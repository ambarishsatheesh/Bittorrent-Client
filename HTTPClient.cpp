#include "HTTPClient.h"
#include "Decoder.h"
#include "Utility.h"
#include "loguru.h"

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
			std::cout << peerHost << "\n";
			LOG_F(INFO, "Resolving HTTP tracker (%s:%s)...", 
				peerHost, peerPort);

			//socket.close();
			socket.open(tcp::v4());

			// Look up the domain name
			auto const results = resolver.resolve(peerHost, peerPort);

			// Make the connection on the IP address we get from a lookup
			remoteEndpoint = boost::asio::connect(socket, results.begin(),
				results.end())->endpoint();

			LOG_F(INFO, "Resolved HTTP tracker endpoint! Endpoint: %s:%hu (%s:%s).",
				remoteEndpoint.address().to_string(), remoteEndpoint.port(),
				peerHost, peerPort);

			dataTransmission(isAnnounce);
		}
		catch(const boost::system::system_error& e)
		{
			LOG_F(ERROR, 
				"Failed to resolve HTTP tracker %s:%s! Error msg: \"%s\".", 
				peerHost, peerPort, e.what());
		}
	}

	HTTPClient::~HTTPClient()
	{
		socket.close();

		LOG_F(INFO,
			"Closed HTTP socket used for tracker update (%s:%s).",
			peerHost, peerPort);
	}

	void HTTPClient::scrapeRequest(boost::system::error_code& err)
	{
		try
		{
			// Set up an HTTP GET request message
			http::request<boost::beast::http::string_body> req{ http::verb::get,
				target, version };
			req.set(http::field::host, peerHost);
			req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

			// Send the HTTP request to the remote host
			boost::beast::http::write(socket, req);

			LOG_F(INFO,
				"Sent HTTP scrape request to tracker %s:%d; "
				"Status: %s; Scrape URL: %s." ,
				remoteEndpoint.address().to_string(), remoteEndpoint.port(),
				err.message().c_str(), target.c_str());

			// This buffer is used for reading and must be persisted
			boost::beast::flat_buffer buffer;

			// Declare a container to hold the response
			http::response<http::dynamic_body> res;

			// Receive the HTTP response
			http::read(socket, buffer, res);

			LOG_F(INFO,
				"Received HTTP scrape response from tracker %s:%hu; "
				"Status: %s; Bytes received: %d.",
				remoteEndpoint.address().to_string(), remoteEndpoint.port(),
				err.message().c_str(), res.payload_size().get());

			// Gracefully close the socket
			boost::system::error_code ec;
			socket.shutdown(tcp::socket::shutdown_both, ec);
			socket.close();

			LOG_F(INFO,
				"Closed HTTP connection with tracker %s:%hu.",
				remoteEndpoint.address().to_string(), remoteEndpoint.port());

			if (ec && ec != boost::system::errc::not_connected)
			{
				LOG_F(ERROR, "Error shutting down socket: %s.", 
					ec.message().c_str());
			}

			handleScrapeResp(res);
		}
		catch (const boost::system::system_error& e)
		{
			LOG_F(ERROR,
				"HTTP scrape request failure (tracker %s:%hu): %s.",
				remoteEndpoint.address().to_string(), remoteEndpoint.port(), 
				e.what());
		}
	}

	void HTTPClient::announceRequest(boost::system::error_code& err)
	{
		try
		{
			// Set up an HTTP GET request message
			http::request<boost::beast::http::string_body> req{ http::verb::get,
				target, version };
			req.set(http::field::host, peerHost);
			req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

			// Send the HTTP request to the remote host
			boost::beast::http::write(socket, req);

			LOG_F(INFO,
				"Sent HTTP announce request to tracker %s:%d; "
				"Status: %s; announce URL: %s.",
				remoteEndpoint.address().to_string(), remoteEndpoint.port(),
				err.message().c_str(), target.c_str());

			// This buffer is used for reading and must be persisted
			boost::beast::flat_buffer buffer;

			// Declare a container to hold the response
			http::response<http::dynamic_body> res;

			// Receive the HTTP response
			http::read(socket, buffer, res);

			LOG_F(INFO,
				"Received HTTP announce response from tracker %s:%hu; "
				"Status: %s; Bytes received: %d.",
				remoteEndpoint.address().to_string(), remoteEndpoint.port(),
				err.message().c_str(), res.payload_size().get());


			// Gracefully close the socket
			boost::system::error_code ec;
			socket.shutdown(tcp::socket::shutdown_both, ec);
			socket.close();

			LOG_F(INFO,
				"Closed HTTP connection with tracker %s:%hu.",
				remoteEndpoint.address().to_string(), remoteEndpoint.port());

			if (ec && ec != boost::system::errc::not_connected)
			{
				LOG_F(ERROR, "Error shutting down socket: %s",
					ec.message().c_str());
			}

			handleAnnounceResp(res);
		}
		catch (const boost::system::system_error& e)
		{
			LOG_F(ERROR,
				"HTTP announce request failure (tracker %s:%hu): %s.",
				remoteEndpoint.address().to_string(), remoteEndpoint.port(),
				e.what());
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
			LOG_F(ERROR,
				"Scrape GET request error (tracker %s:%hu)! Status code: %s.",
				remoteEndpoint.address().to_string(), remoteEndpoint.port(),
				result.c_str());

			return;
		}

		LOG_F(INFO,
			"Successful scrape GET request to tracker %s:%hu! Status code: %s.",
			remoteEndpoint.address().to_string(), remoteEndpoint.port(),
			result.c_str());

		//get and store body
		std::string body{ boost::asio::buffers_begin(response.body().data()),
				   boost::asio::buffers_end(response.body().data()) };

		LOG_F(INFO,
			"Tracker (%s:%hu) scrape response body: %s.",
			remoteEndpoint.address().to_string(), remoteEndpoint.port(),
			body.c_str());

		valueDictionary info = boost::get<valueDictionary>(Decoder::decode(body));

		if (info.empty())
		{
			LOG_F(ERROR, "Unable to decode tracker announce response!");
			return;
		}

		if (info.count("failure reason"))
		{
			auto failReason = boost::get<std::string>(info.at("failure reason"));

			LOG_F(INFO,
				"Tracker (%s:%hu) scrape response: "
				"Failure Reason %s.",
				remoteEndpoint.address().to_string(), remoteEndpoint.port(),
				failReason.c_str());

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

			LOG_F(INFO, 
				"Updated peer info using tracker (%s:%hu) scrape response!",
				remoteEndpoint.address().to_string(), remoteEndpoint.port());
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
			LOG_F(ERROR,
				"Announce GET request error (tracker %s:%hu)! Status code: %s.",
				remoteEndpoint.address().to_string(), remoteEndpoint.port(),
				result.c_str());

			return;
		}

		LOG_F(INFO,
			"Successful announce GET request to tracker %s:%hu! Status code: %s.",
			remoteEndpoint.address().to_string(), remoteEndpoint.port(),
			result.c_str());

		//get and store body
		std::string body{ boost::asio::buffers_begin(response.body().data()),
				   boost::asio::buffers_end(response.body().data()) };

		LOG_F(INFO,
			"Tracker (%s:%hu) announce response body: %s.",
			remoteEndpoint.address().to_string(), remoteEndpoint.port(),
			body.c_str());

		valueDictionary info = boost::get<valueDictionary>(Decoder::decode(body));

		if (info.empty())
		{
			LOG_F(ERROR, "Unable to decode tracker announce response!");
			return;
		}

		peerRequestInterval =
			boost::posix_time::seconds(static_cast<int>(boost::get<long long>(
				info.at("interval"))));

		if (info.count("failure reason"))
		{
			auto failReason = boost::get<std::string>(info.at("failure reason"));

			LOG_F(INFO,
				"Tracker (%s:%hu) announce response: "
				"Failure Reason %s.",
				remoteEndpoint.address().to_string(), remoteEndpoint.port(),
				failReason.c_str());

			return;
		}
		else
		{
			complete = static_cast<int>(
				boost::get<long long>(info.at("complete")));
			incomplete = static_cast<int>(
				boost::get<long long>(info.at("incomplete")));

			if (!info.count("peers"))
			{
				LOG_F(ERROR, "No peers data found in tracker announce response!");
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
					LOG_F(INFO,
						"Updated peer info using tracker (%s:%hu) announce "
						"response (compact)!",
						remoteEndpoint.address().to_string(), 
						remoteEndpoint.port());
				}
				//non-compact uses a list of dictionaries
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
							boost::get<long long>(peerInfoDict.at("port")));
						//add to peers list
						peer singlePeer;
						singlePeer.ipAddress = ipAddress;
						singlePeer.port = peerPort;
						peerList.push_back(singlePeer);
					}
					LOG_F(INFO,
						"Updated peer info using tracker (%s:%hu) announce "
						"response (non-compact)!",
						remoteEndpoint.address().to_string(),
						remoteEndpoint.port());
				}
			}
		}
	}

	void HTTPClient::dataTransmission(bool isAnnounce)
	{
		boost::system::error_code err;

		//use interval value to check if we have announced before
		if (isAnnounce)
		{
			LOG_F(INFO, "Starting HTTP announce request...");

			announceRequest(err);
		}
		else if (!isAnnounce)
		{
			//if target doesn't contain "announce" immediately after the slash,
			//it doesn't support scraping
			auto slashPos = target.find_last_of("\\/");
			auto ancPos = target.find("announce");
			if (ancPos == 1)
			{
				LOG_F(INFO, "Starting HTTP scrape request...");

				target.replace(ancPos, 8, "scrape");
				scrapeRequest(err);
			}
			else if (ancPos - slashPos != 1)
			{
				LOG_F(WARNING,
					"Scraping is not supported for this tracker (%s:%hu)",
					remoteEndpoint.address().to_string(), remoteEndpoint.port());
				LOG_F(INFO, "Starting HTTP announce request...");

				announceRequest(err);
			}
		}
	}
}
