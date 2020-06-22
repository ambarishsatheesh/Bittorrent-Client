#include "HTTPClient.h"
#include "Decoder.h"
#include "Utility.h"
#include "loguru.h"

#include <iomanip>

#include <boost/optional/optional_io.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

namespace Bittorrent
{
using namespace utility;

HTTPClient::HTTPClient(trackerUrl& parsedUrl, bool isAnnounce)
    : peerHost{ parsedUrl.hostname }, peerPort{ parsedUrl.port },
    target{ parsedUrl.target }, version{ 10 }, m_isAnnounce{ isAnnounce },
    peerRequestInterval{ 0 }, complete{ 0 }, incomplete{ 0 }, errMessage{""},
    io_context(), resolver( io_context ), socket( io_context ), remoteEndpoint()
{
    LOG_F(INFO, "Resolving HTTP tracker (%s:%s)...",
        peerHost.c_str(), peerPort.c_str());

    dataTransmission(isAnnounce);
}

void HTTPClient::run(std::chrono::steady_clock::duration timeout)
{
    //restart io_context in case there have been previous run invocations
    io_context.restart();

    io_context.run_for(timeout);

    //If not stopped, then the io_context::run_for call must have timed out.
    if (!io_context.stopped())
    {
        LOG_F(ERROR, "Asynchronous operation timed out! (Tracker %s:%s).",
            peerHost.c_str(), peerPort.c_str());

        peerRequestInterval = std::chrono::seconds(1800);

        // Close the socket to cancel the outstanding asynchronous operation.
        socket.close();
    }
}

HTTPClient::~HTTPClient()
{
    close();
}

void HTTPClient::close()
{
    socket.close();

    LOG_F(INFO,
        "Closed HTTP socket used for tracker update (%s:%s).",
        peerHost.c_str(), peerPort.c_str());
}

void HTTPClient::dataTransmission(bool isAnnounce)
{
    //assign new empty http::response for next request (asio methods are additive)
    http::response<http::dynamic_body> res2;
    res = res2;

    m_isAnnounce = isAnnounce;

    tcp::resolver resolver{ io_context };
    auto results = resolver.resolve(peerHost, peerPort);

    //need to bind object context using "this" for class member functions
    boost::asio::async_connect(socket, results,
        boost::bind(&HTTPClient::handleConnect, this,
            boost::asio::placeholders::error));

    //set 10 second total deadline timer for all asynchronous operations
    run(std::chrono::seconds(10));
}

void HTTPClient::handleConnect(const boost::system::error_code& error)
{
    if (!error)
    {
        /*remoteEndpoint = endpoint_itr->endpoint();*/
        remoteEndpoint = socket.remote_endpoint();

        LOG_F(INFO, "Resolved HTTP tracker endpoint! Endpoint: %s:%hu (%s:%s).",
            remoteEndpoint.address().to_string().c_str(), remoteEndpoint.port(),
            peerHost.c_str(), peerPort.c_str());

        //use interval value to check if we have announced before
        if (m_isAnnounce)
        {
            LOG_F(INFO, "Starting HTTP announce request...");

            announceRequest();
        }
        else if (!m_isAnnounce)
        {
            //if target doesn't contain "announce" immediately after the slash,
            //it doesn't support scraping
            auto slashPos = target.find_last_of("\\/");
            auto ancPos = target.find("announce");
            if (ancPos == 1)
            {
                LOG_F(INFO, "Starting HTTP scrape request...");

                target.replace(ancPos, 8, "scrape");
                scrapeRequest();
            }
            else if (ancPos - slashPos != 1)
            {
                LOG_F(WARNING,
                    "Scraping is not supported for this tracker (%s:%s)",
                    peerHost.c_str(), peerPort.c_str());
                LOG_F(INFO, "Starting HTTP announce request...");

                announceRequest();
            }
        }
    }
    else
    {
        LOG_F(ERROR,
            "Failed to resolve HTTP tracker %s:%s! Error msg: \"%s\".",
            peerHost.c_str(), peerPort.c_str(), error.message().c_str());

        peerRequestInterval = std::chrono::seconds(1800);

        close();
    }
}

void HTTPClient::scrapeRequest()
{
    // Set up a HTTP GET request message
    req = { http::verb::get, target, version };
    req.set(http::field::host, peerHost);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

    http::async_write(socket, req, boost::bind(&HTTPClient::handleScrapeSend,
        this, boost::asio::placeholders::error));
}

void HTTPClient::handleScrapeSend(const boost::system::error_code& error)
{
    if (!error)
    {
        LOG_F(INFO,
            "Sent HTTP scrape request to tracker %s:%hu (%s:%s); "
            "Status: %s; Scrape URL: %s.",
            remoteEndpoint.address().to_string().c_str(), remoteEndpoint.port(),
            peerHost.c_str(), peerPort.c_str(),
            error.message().c_str(), target.c_str());

        http::async_read(socket, recBuffer, res,
            boost::bind(&HTTPClient::handleScrapeReceive, this,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    }
    else
    {
        LOG_F(ERROR,
            "Failed to send HTTP scrape request to tracker %s:%hu (%s:%s) "
            "from %s:%hu! Error msg: \"%s\".",
            remoteEndpoint.address().to_string().c_str(), remoteEndpoint.port(),
            peerHost.c_str(), peerPort.c_str(),
            socket.local_endpoint().address().to_string().c_str(),
            socket.local_endpoint().port(), error.message().c_str());

        close();
    }
}

void HTTPClient::handleScrapeReceive(const boost::system::error_code& error,
    const size_t& bytesTransferred)
{
    if (!error)
    {
        LOG_F(INFO,
            "Received HTTP scrape response from tracker %s:%hu (%s:%s); "
            "Status: %s; Bytes received: %d.",
            remoteEndpoint.address().to_string().c_str(), remoteEndpoint.port(),
            peerHost.c_str(), peerPort.c_str(),
            error.message().c_str(), static_cast<int>(bytesTransferred));

        handleScrapeResp();
    }
    else
    {
        LOG_F(ERROR,
            "Failed to receive HTTP scrape response from tracker %s:%hu (%s:%s)! "
            "Error msg: \"%s\".",
            remoteEndpoint.address().to_string().c_str(), remoteEndpoint.port(),
            peerHost.c_str(), peerPort.c_str(), error.message().c_str());

        peerRequestInterval = std::chrono::seconds(1800);

        close();
    }
}

void HTTPClient::handleScrapeResp()
{
    //stream response result object, extract status code and store in string
    std::stringstream ss;
    ss << res.base();
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
            "Scrape GET request error (tracker %s:%hu - %s:%s)! Status code: %s.",
            remoteEndpoint.address().to_string().c_str(),
            remoteEndpoint.port(), peerHost.c_str(), peerPort.c_str(),
            result.c_str());

        peerRequestInterval = std::chrono::seconds(1800);

        return;
    }

    LOG_F(INFO,
        "Successful scrape GET request to tracker %s:%hu (%s:%s)! Status code:"
        "%s.",
        remoteEndpoint.address().to_string().c_str(),
        remoteEndpoint.port(),
        peerHost.c_str(), peerPort.c_str(),
        result.c_str());

    //get and store body
    std::string body{ boost::asio::buffers_begin(res.body().data()),
                boost::asio::buffers_end(res.body().data()) };

    LOG_F(INFO,
        "Tracker (%s:%hu - %s:%s) scrape response body: %s.",
        remoteEndpoint.address().to_string().c_str(),
        remoteEndpoint.port(), peerHost.c_str(), peerPort.c_str(),
        body.c_str());

    valueDictionary info = boost::get<valueDictionary>(Decoder::decode(body));

    if (info.empty())
    {
        LOG_F(ERROR, "Unable to decode tracker announce response!");

        peerRequestInterval = std::chrono::seconds(1800);

        return;
    }

    if (info.count("failure reason"))
    {
        auto failReason = boost::get<std::string>(info.at("failure reason"));

        LOG_F(ERROR,
            "Tracker (%s:%hu - %s:%s) scrape response: "
            "Failure Reason %s.",
            remoteEndpoint.address().to_string().c_str(),
            remoteEndpoint.port(), peerHost.c_str(), peerPort.c_str(),
            failReason.c_str());

        peerRequestInterval = std::chrono::seconds(1800);

        return;
    }
    else
    {
        if (info.count("files") )
        {
            peerRequestInterval = std::chrono::seconds(1800);

            valueDictionary files = boost::get<valueDictionary>(info.at("files"));
            valueDictionary hash;
            for (valueDictionary::iterator it = files.begin(); it != files.end();
                    ++it)
            {
                hash = boost::get<valueDictionary>(it->second);
            }

            if (hash.count("complete") && hash.count("incomplete"))
            {
                complete = static_cast<int>(boost::get<long long>(
                    hash.at("complete")));
                incomplete = static_cast<int>(boost::get<long long>(
                    hash.at("incomplete")));

                LOG_F(INFO,
                    "Updated peer info using tracker (%s:%hu - %s%s) scrape response!",
                    remoteEndpoint.address().to_string().c_str(),
                    remoteEndpoint.port(), peerHost.c_str(), peerPort.c_str());
            }
            else
            {
                LOG_F(ERROR,
                    "Invalid scrape response from tracker %s:%hu (%s:%s)",
                    remoteEndpoint.address().to_string().c_str(),
                    remoteEndpoint.port(), peerHost.c_str(), peerPort.c_str());
            }
        }
        else
        {
            LOG_F(ERROR,
                "Invalid scrape response from tracker %s:%hu (%s:%s)",
                remoteEndpoint.address().to_string().c_str(),
                remoteEndpoint.port(), peerHost.c_str(), peerPort.c_str());

            peerRequestInterval = std::chrono::seconds(1800);
        }
    }
}


void HTTPClient::announceRequest()
{
    // Set up a HTTP GET request message
    req = { http::verb::get, target, version };
    req.set(http::field::host, peerHost);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

    // Send the HTTP request to the remote host
    http::async_write(socket, req, boost::bind(&HTTPClient::handleAnnounceSend,
        this, boost::asio::placeholders::error));
}

void HTTPClient::handleAnnounceSend(const boost::system::error_code& error)
{
    if (!error)
    {
        LOG_F(INFO,
            "Sent HTTP announce request to tracker %s:%hu (%s:%s); "
            "Status: %s; Announce URL: %s.",
            remoteEndpoint.address().to_string().c_str(), remoteEndpoint.port(),
            peerHost.c_str(), peerPort.c_str(),
            error.message().c_str(), target.c_str());

        http::async_read(socket, recBuffer, res,
            boost::bind(&HTTPClient::handleAnnounceReceive, this,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    }
    else
    {
        LOG_F(ERROR,
            "Failed to send HTTP announce request to tracker %s:%hu (%s:%s) "
            "from %s:%hu! Error msg: \"%s\".",
            remoteEndpoint.address().to_string().c_str(), remoteEndpoint.port(),
            peerHost.c_str(), peerPort.c_str(),
            socket.local_endpoint().address().to_string().c_str(),
            socket.local_endpoint().port(), error.message().c_str());

        peerRequestInterval = std::chrono::seconds(1800);

        close();
    }
}

void HTTPClient::handleAnnounceReceive(const boost::system::error_code& error,
    const size_t& bytesTransferred)
{
    if (!error)
    {
        LOG_F(INFO,
            "Received HTTP announce response from tracker %s:%hu (%s:%s); "
            "Status: %s; Bytes received: %d.",
            remoteEndpoint.address().to_string().c_str(), remoteEndpoint.port(),
            peerHost.c_str(), peerPort.c_str(),
            error.message().c_str(), static_cast<int>(bytesTransferred));

        handleAnnounceResp();
    }
    else
    {
        LOG_F(ERROR,
            "Failed to receive HTTP announce response from tracker %s:%hu (%s:%s)! "
            "Error msg: \"%s\".",
            remoteEndpoint.address().to_string().c_str(), remoteEndpoint.port(),
            peerHost.c_str(), peerPort.c_str(), error.message().c_str());

        peerRequestInterval = std::chrono::seconds(1800);

        close();
    }
}

void HTTPClient::handleAnnounceResp()
{
    //stream response result object, extract status code and store in string
    std::stringstream ss;
    ss << res.base();
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
            "Announce GET request error (tracker %s:%hu - %s:%s)! Status code: "
            "%s.",
            remoteEndpoint.address().to_string().c_str(),
            remoteEndpoint.port(), peerHost.c_str(), peerPort.c_str(),
            result.c_str());

        peerRequestInterval = std::chrono::seconds(1800);

        return;
    }

    LOG_F(INFO,
        "Successful announce GET request to tracker %s:%hu (%s:%s)! "
        "Status code: %s.",
        remoteEndpoint.address().to_string().c_str(),
        remoteEndpoint.port(), peerHost.c_str(), peerPort.c_str(),
        result.c_str());

    //get and store body
    std::string body{ boost::asio::buffers_begin(res.body().data()),
                boost::asio::buffers_end(res.body().data()) };

    LOG_F(INFO,
        "Tracker (%s:%hu - %s:%s) announce response body: %s.",
        remoteEndpoint.address().to_string().c_str(),
        remoteEndpoint.port(), peerHost.c_str(), peerPort.c_str(),
        body.c_str());

    valueDictionary info = boost::get<valueDictionary>(Decoder::decode(body));

    if (info.empty())
    {
        LOG_F(ERROR, "Unable to decode tracker (%s:%hu - %s:%s) announce response!",
              remoteEndpoint.address().to_string().c_str(),
              remoteEndpoint.port(), peerHost.c_str(), peerPort.c_str());

        peerRequestInterval = std::chrono::seconds(1800);

        return;
    }

    if (info.count("failure reason"))
    {
        errMessage = boost::get<std::string>(info.at("failure reason"));

        LOG_F(ERROR,
            "Tracker (%s:%hu - %s:%s) announce response: "
            "Failure Reason %s.",
            remoteEndpoint.address().to_string().c_str(),
            remoteEndpoint.port(), peerHost.c_str(), peerPort.c_str(),
            errMessage.c_str());

        peerRequestInterval = std::chrono::seconds(1800);

        return;
    }
    else
    {
        peerRequestInterval =
            std::chrono::seconds(static_cast<int>(boost::get<long long>(
                info.at("interval"))));

        complete = static_cast<int>(
            boost::get<long long>(info.at("complete")));
        incomplete = static_cast<int>(
            boost::get<long long>(info.at("incomplete")));

        if (!info.count("peers"))
        {
            LOG_F(ERROR, "No peers data found in tracker (%s:%hu - %s:%s)"
                    " announce response!",
                  remoteEndpoint.address().to_string().c_str(),
                  remoteEndpoint.port(), peerHost.c_str(), peerPort.c_str());
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
                    "Updated peer info using tracker (%s:%hu - %s:%s) announce "
                    "response (compact)!",
                    remoteEndpoint.address().to_string().c_str(),
                    remoteEndpoint.port(), peerHost.c_str(), peerPort.c_str());
            }
            //non-compact uses a list of dictionaries
            else
            {
                valueList peerInfoList = boost::get<valueList>(
                    info.at("peers"));

                for (size_t i = 0; i < peerInfoList.size(); ++i)
                {
                    //get and add to peers list
                    peer singlePeer;

                    valueDictionary peerInfoDict =
                        boost::get<valueDictionary>(peerInfoList.at(i));

                    singlePeer.ipAddress =
                        boost::get<std::string>(peerInfoDict.at("ip"));

                    singlePeer.port = static_cast<int>(
                        boost::get<long long>(peerInfoDict.at("port")));

                    const std::string peerID =
                        boost::get<std::string>(peerInfoDict.at("peer id"));
                    singlePeer.peerID.assign(peerID.begin(), peerID.end());

                    peerList.push_back(singlePeer);
                }
                LOG_F(INFO,
                    "Updated peer info using tracker (%s:%hu - %s:%s) announce "
                    "response (non-compact)!",
                    remoteEndpoint.address().to_string().c_str(),
                    remoteEndpoint.port(), peerHost.c_str(), peerPort.c_str());
            }
        }
    }
}

}
