#include "HTTPClient.h"
#include "Decoder.h"
#include "Utility.h"
#include <iomanip>

#include <boost/optional/optional_io.hpp>

namespace Bittorrent
{
	using namespace utility;

	HTTPClient::HTTPClient(trackerUrl& parsedUrl, bool isAnnounce)
		: peerHost{ parsedUrl.hostname }, peerPort{ parsedUrl.port }, 
		target{ parsedUrl.target }, version{ 10 }, peerRequestInterval{ 0 }, 
		complete{ 0 }, incomplete{ 0 }, io_context(), resolver( io_context ), 
		socket( io_context ), remoteEndpoint()
	{
		try
		{
			//socket.close();
			socket.open(tcp::v4());

			// Look up the domain name
			auto const results = resolver.resolve(peerHost, peerPort);

			// Make the connection on the IP address we get from a lookup
			remoteEndpoint = boost::asio::connect(socket, results.begin(),
				results.end())->endpoint();
		}
		catch(const boost::system::system_error& e)
		{
			std::cout << "\n" << "Error occured! Error code = " << e.code()
				<< ". Message: " << e.what();
		}

		dataTransmission(parsedUrl, isAnnounce);
	}

	HTTPClient::~HTTPClient()
	{
		socket.close();
	}

	void HTTPClient::scrapeRequest(trackerUrl& parsedUrl,
		boost::system::error_code& err)
	{
		try
		{
			// Set up an HTTP GET request message
			http::request<boost::beast::http::string_body> req{ http::verb::get,
				target, version };
			req.set(http::field::host, peerHost);
			req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

			std::cout << "Sending..." << "\n";

			// Send the HTTP request to the remote host
			boost::beast::http::write(socket, req);

			std::cout << "Sent HTTP scrape request to " << remoteEndpoint
				<< ": " << target << " (" << err.message()
				<< ") \n";

			std::cout << "Receiving..." << "\n";

			// This buffer is used for reading and must be persisted
			boost::beast::flat_buffer buffer;

			// Declare a container to hold the response
			http::response<http::dynamic_body> res;

			// Receive the HTTP response
			http::read(socket, buffer, res);

			std::cout << "Received HTTP scrape response from " 
				<< remoteEndpoint << ": " << res.payload_size() << " bytes" 
				<< " (" << err.message() << ") \n";

			// Gracefully close the socket
			boost::system::error_code ec;
			socket.shutdown(tcp::socket::shutdown_both, ec);
			socket.close();

			// not_connected happens sometimes
			// so don't bother reporting it.
			if (ec && ec != boost::system::errc::not_connected)
				throw boost::system::system_error{ ec };

			handleScrapeResp(res);
		}
		catch (const boost::system::system_error& e)
		{
			std::cout << "\n" << "Error occured! Error code = " << e.code()
				<< ". Message: " << e.what();
		}
	}

	void HTTPClient::announceRequest(trackerUrl& parsedUrl,
		boost::system::error_code& err)
	{
		try
		{
			// Set up an HTTP GET request message
			http::request<boost::beast::http::string_body> req{ http::verb::get,
				target, version };
			req.set(http::field::host, peerHost);
			req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

			std::cout << "Sending..." << "\n";

			// Send the HTTP request to the remote host
			boost::beast::http::write(socket, req);

			std::cout << "Sent HTTP announce request to " << remoteEndpoint 
				<< ": " << std::quoted(req.body()) << " (" << err.message() 
				<< ") \n";

			std::cout << "Receiving..." << "\n";

			// This buffer is used for reading and must be persisted
			boost::beast::flat_buffer buffer;

			// Declare a container to hold the response
			http::response<http::dynamic_body> res;

			// Receive the HTTP response
			http::read(socket, buffer, res);

			std::cout << "Received HTTP announce response from " 
				<< remoteEndpoint << ": " << res.payload_size() << " bytes" 
				<< " (" << err.message() << ") \n";

			// Gracefully close the socket
			boost::system::error_code ec;
			socket.shutdown(tcp::socket::shutdown_both, ec);
			socket.close();

			// not_connected happens sometimes
			// so don't bother reporting it.
			if (ec && ec != boost::system::errc::not_connected)
				throw boost::system::system_error{ ec };

			handleAnnounceResp(res);
		}
		catch (const boost::system::system_error& e)
		{
			std::cout << "\n" << "Error occured! Error code = " << e.code()
				<< ". Message: " << e.what();
		}
	}

	// TODO: needs more testing
	void HTTPClient::handleScrapeResp(
		http::response<http::dynamic_body>& response)
	{
		//stream response result object, extract status code and store in string
		std::stringstream ss;
		ss << response.base();
		std::string result;
		std::getline(ss, result);
		const auto space1 = result.find_first_of(" ");
		const auto space2 = result.find(" ", space1 + 1);
		if (space2 != std::string::npos)
		{
			result = result.substr(space1 + 1, space2 - space1);
		}

		if (stoi(result) != 200)
		{
			std::cout << "\n" << "Error reaching tracker \"" + peerHost + "\": "
				+ "status code " + result << std::endl;
			return;
		}

		//get and store body
		std::string body{ boost::asio::buffers_begin(response.body().data()),
				   boost::asio::buffers_end(response.body().data()) };

		std::cout << body << "\n";

		valueDictionary info = boost::get<valueDictionary>(Decoder::decode(body));

		if (info.empty())
		{
			std::cout << "Unable to decode tracker announce response!" <<
				std::endl;
			return;
		}

		if (info.count("failure reason"))
		{
			std::cout << boost::get<std::string>(info.at("failure reason"));
			return;
		}
		else
		{
			valueDictionary files = boost::get<valueDictionary>(info.at("files"));
			valueDictionary hash;
			for (valueDictionary::iterator it = files.begin(); it != files.end();
				++it)
			{
				hash = boost::get<valueDictionary>(it->second);
			}

			complete = static_cast<int>(boost::get<long long>(hash.at("complete")));
			incomplete = static_cast<int>(boost::get<long long>(hash.at("complete")));

			std::cout << "complete: " << complete << "\n";
			std::cout << "incomplete: " << complete << "\n";
		}
	}

	void HTTPClient::handleAnnounceResp(
		http::response<http::dynamic_body>& response)
	{
		//stream response result object, extract status code and store in string
		std::stringstream ss;
		ss << response.base();
		std::string result;
		std::getline(ss, result);
		const auto space1 = result.find_first_of(" ");
		const auto space2 = result.find(" ", space1 + 1);
		if (space2 != std::string::npos)
		{
			result = result.substr(space1 + 1, space2 - space1);
		}

		if (stoi(result) != 200)
		{
			std::cout << "\n" << "Error reaching tracker \"" + peerHost + "\": "
				+ "status code " + result << std::endl;
			return;
		}

		//get and store body
		std::string body{ boost::asio::buffers_begin(response.body().data()),
				   boost::asio::buffers_end(response.body().data()) };

		valueDictionary info = boost::get<valueDictionary>(Decoder::decode(body));

		std::cout << body << "\n";

		if (info.empty())
		{
			std::cout << "Unable to decode tracker announce response!" <<
				std::endl;
			return;
		}

		peerRequestInterval =
			boost::posix_time::seconds(static_cast<int>(boost::get<long long>(
				info.at("interval"))));

		if (info.count("failure reason"))
		{
			std::cout << boost::get<std::string>(info.at("failure reason"));
				return;
		}
		else
		{
			complete = static_cast<int>(boost::get<long long>(info.at("complete")));
			incomplete = static_cast<int>(boost::get<long long>(info.at("incomplete")));

			std::cout << "complete: " << complete << "\n";
			std::cout << "incomplete: " << incomplete << "\n";

			if (!info.count("peers"))
			{
				std::cout << "\n" << "No peers data found in tracker response!" 
					<<std::endl;
				return;
			}
			else
			{
				//compact version is a string of (multiple) 6 bytes 
				//first 4 are IP address, last 2 are port number in network notation
				if (info.at("peers").type() == typeid(std::string))
				{
					std::string peerInfoString = boost::get<std::string>(
						info.at("peers"));
					std::vector<byte> peerInfo(peerInfoString.begin(),
						peerInfoString.end());
					//info is split into chunks of 6 bytes
					for (size_t i = 0; i < peerInfo.size(); i += 6)
					{
						//first four bytes of each chunk form the ip address
						std::string ipAddress = std::to_string(peerInfo.at(i))
							+ "." + std::to_string(peerInfo.at(i + 1)) + "." +
							std::to_string(peerInfo.at(i + 2)) + "."
							+ std::to_string(peerInfo.at(i + 3));
						//the two bytes representing port are in big endian
						//read in correct order directly using bit shifting
						std::string peerPort = std::to_string(
							(peerInfo.at(i + 4) << 8) |
							(peerInfo.at(i + 5)));
						//add to peers list
						peer singlePeer;
						singlePeer.ipAddress = ipAddress;
						singlePeer.port = peerPort;
						peerList.push_back(singlePeer);
					}
				}
				//non compact uses a list of dictionaries
				else
				{
					valueList peerInfoList = boost::get<valueList>(
						info.at("peers"));
					for (size_t i = 0; i < peerInfoList.size(); ++i)
					{
						valueDictionary peerInfoDict =
							boost::get<valueDictionary>(peerInfoList.at(i));
						const std::string ipAddress =
							boost::get<std::string>(peerInfoDict.at("ip"));
						const int peerPort = static_cast<int>(
							boost::get<long long>(peerInfoDict.at("ip")));
						//add to peers list
						peer singlePeer;
						singlePeer.ipAddress = ipAddress;
						singlePeer.port = peerPort;
						peerList.push_back(singlePeer);
					}
				}
			}
		}
	}

	void HTTPClient::dataTransmission(trackerUrl& parsedUrl, bool isAnnounce)
	{
		boost::system::error_code err;

		//use interval value to check if we have announced before
		if (isAnnounce)
		{
			announceRequest(parsedUrl, err);
		}
		else if (!isAnnounce && target.find("scrape") != std::string::npos)
		{
			scrapeRequest(parsedUrl, err);
		}
		else
		{		
			//if target doesn't contain "announce" immediately after the slash,
			//it doesn't support scraping
			auto slashPos = parsedUrl.target.find_last_of("\\/");
			auto ancPos = parsedUrl.target.find("announce");
			if (ancPos - slashPos != 1)
			{
				announceRequest(parsedUrl, err);
			}
			else if (ancPos == 1)
			{
				target.replace(ancPos, 8, "scrape");
				scrapeRequest(parsedUrl, err);
			}
		}
	}
}
