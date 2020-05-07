#include "trackerObj.h"
#include "Decoder.h"


namespace Bittorrent
{
	trackerObj::trackerObj()
		: trackerAddress{ "" },
		lastPeerRequest{ boost::posix_time::ptime(boost::posix_time::min_date_time) },
		peerRequestInterval{ 1800 }
	{
	}

	//create required url for GET request
	void trackerObj::update(trackerEvent trkEvent, long clientID,
		int port, std::string urlEncodedInfoHash, std::vector<int8_t> infoHash,
		long long uploaded, long long downloaded, long long remaining, bool compact)
	{
		//switch case to get enumerator string
		std::string stringEvent;
		int intEvent;
		switch (trkEvent)
		{
		case trackerEvent::started:
				stringEvent = "started";
				intEvent = 1;
				break;
		case trackerEvent::paused:
				stringEvent = "paused";
				intEvent = 2;
				break;
		case trackerEvent::stopped:
				stringEvent = "stopped";
				intEvent = 3;
				break;
		}

		//wait until time interval has elapsed before requesting new peers
		if (trkEvent == trackerObj::trackerEvent::started &&
			boost::posix_time::second_clock::universal_time() <
			(lastPeerRequest + peerRequestInterval))
		{
			return;
		}

		lastPeerRequest = boost::posix_time::second_clock::universal_time();

		//compact defaulted to 1 for now
		std::string url = trackerAddress + "?info_hash=" + urlEncodedInfoHash +
			"&peer_id=" + std::to_string(clientID) + "&port=" + std::to_string(port) + "&uploaded=" +
			std::to_string(uploaded) + "&downloaded=" +
			std::to_string(downloaded) + "&left=" + std::to_string(remaining) +
			"&event=" + stringEvent + "&compact=" + std::to_string(compact);

		//parse url
		trackerUrl parsedUrl(url);

		//request url using appropriate protocol
		if (parsedUrl.protocol == trackerUrl::protocolType::http)
		{
			//HTTPRequest(parsedUrl, compact);
			UDP udp(parsedUrl, clientID, infoHash, uploaded, downloaded, remaining, intEvent);
		}
		else
		{
			
		}
	}

	void trackerObj::resetLastRequest()
	{
		lastPeerRequest = boost::posix_time::ptime(boost::posix_time::min_date_time);
	}




	void trackerObj::HTTPRequest(trackerUrl parsedUrl, bool compact)
	{
		try
		{
			//if empty use default values
			const auto host = parsedUrl.hostname;
			const auto port = parsedUrl.port.empty() ? "80" : parsedUrl.port;
			const auto target = parsedUrl.target.empty() ? "/" : parsedUrl.target;
			const auto version = 10;

			// The io_context is required for all I/O
			boost::asio::io_context io_context;

			// These objects perform our I/O
			tcp::resolver resolver{ io_context };
			tcp::socket socket{ io_context };

			// Look up the domain name
			auto const results = resolver.resolve(host, port);

			// Make the connection on the IP address we get from a lookup
			boost::asio::connect(socket, results.begin(), results.end());

			// Set up an HTTP GET request message
			http::request<boost::beast::http::string_body> req{ http::verb::get, target, version };
			req.set(http::field::host, host);
			req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

			// Send the HTTP request to the remote host
			boost::beast::http::write(socket, req);

			// This buffer is used for reading and must be persisted
			boost::beast::flat_buffer buffer;

			// Declare a container to hold the response
			http::response<http::dynamic_body> res;

			// Receive the HTTP response
			http::read(socket, buffer, res);

			// Gracefully close the socket
			boost::system::error_code ec;
			socket.shutdown(tcp::socket::shutdown_both, ec);

			// not_connected happens sometimes
			// so don't bother reporting it.
			//
			if (ec && ec != boost::system::errc::not_connected)
				throw boost::system::system_error{ ec };

			// If we get here then the connection is closed gracefully
			handleHTTPResponse(res, compact);
		}
		catch (std::exception const& e)
		{
			std::cerr << "Error: " << e.what() << std::endl;
		}
	}

	void trackerObj::handleHTTPResponse(http::response<http::dynamic_body> response, bool compact)
	{	
		//stream response result object, extract status code and store in string
		std::stringstream ss;
		ss << response.base();
		std::string result;
		std::getline(ss, result);
		const auto space1 = result.find_first_of(" ");
		const auto space2 = result.find(" ", space1+1);
		if (space2 != std::string::npos)
		{
			result = result.substr(space1+1, space2-space1);
		}

		if (stoi(result) != 200)
		{
			std::cout << "Error reaching tracker \"" + trackerAddress + "\": " + 
				"status code " + result << std::endl;
		}

		//print body
		std::string body{ boost::asio::buffers_begin(response.body().data()),
				   boost::asio::buffers_end(response.body().data()) };
		std::cout << "Body: " << std::quoted(body) << "\n";


		std::string data = "d8:completei2e10:downloadedi0e10:incompletei1e8:intervali1769e12:min intervali884e5:peers18:thisisatestetgersce";

		valueDictionary info = boost::get<valueDictionary>(Decoder::decode(data));

		if (info.empty())
		{
			std::cout << "Unable to decode tracker announce response!" << std::endl;
			return;
		}

		peerRequestInterval =
			boost::posix_time::seconds(boost::get<long long>(info.at("interval")));

		// TODO: tracker response needs testing
		// compact version is a string of (multiple) 6 bytes 
		// first 4 are IP address, last 2 are port number in network notation
		// non compact uses a list of dictionaries
		if (compact)
		{
			std::string peerInfoString = boost::get<std::string>(info.at("peers"));
			std::vector<byte> peerInfo(peerInfoString.begin(), peerInfoString.end());

			for (size_t i = 0; i < peerInfo.size(); ++i)
			{
				const int offset = i * 6;
				std::string ipAddress = std::to_string(peerInfo.at(offset)) + "."
					+ std::to_string(peerInfo.at(offset + 1)) + "." +
					std::to_string(peerInfo.at(offset + 2)) + "."
					+ std::to_string(peerInfo.at(offset + 3));
				//the two bytes representing port are in big endian
				//read in correct order directly using bit shifting
				const int peerPort = (peerInfo.at(offset + 4) << 8) |
					(peerInfo.at(offset + 5));

				// TODO: add to peers
			}
		}
		else
		{
			valueList peerInfoList = boost::get<valueList>(info.at("peers"));
			for (size_t i = 0; i < peerInfoList.size(); ++i)
			{
				valueDictionary peerInfoDict =
					boost::get<valueDictionary>(peerInfoList.at(i));
				const std::string ipAddress =
					boost::get<std::string>(peerInfoDict.at("ip"));
				const int peerPort = boost::get<int>(peerInfoDict.at("ip"));

				// TODO: add to peers
			}
		}

	}
}