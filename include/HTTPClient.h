#pragma once
#include "ValueTypes.h"
#include "trackerUrl.h"

#include <unordered_map>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/buffers_iterator.hpp>

namespace Bittorrent
{
    using tcp = boost::asio::ip::tcp;
    namespace http = boost::beast::http;

    class HTTPClient
    {
    public:
        //general variables
        std::string peerHost;
        std::string peerPort;
        std::string target;
        int version;
        bool m_isAnnounce;

        //response data
        std::chrono::seconds peerRequestInterval;
        int seeders;
        int leechers;
        std::string errMessage;
        bool isFail;
        std::vector<peer> peerList;

        void dataTransmission(bool isAnnounce);

        //constructor
        HTTPClient(trackerUrl& parsedUrl, bool isAnnounce);
        ~HTTPClient();

    private:
        boost::asio::io_context io_context;
        tcp::resolver resolver;
        tcp::socket socket;
        tcp::endpoint remoteEndpoint;
        http::request<boost::beast::http::string_body> req;
        //parser to read and response into
        http::response<http::dynamic_body> res;
        //buffer to hold additional data that lies past the end of the message
        //being read
        boost::beast::flat_buffer recBuffer;

        void close();

        void handleConnect(const boost::system::error_code& error);
        void handleScrapeSend(const boost::system::error_code& error);
        void handleScrapeReceive(const boost::system::error_code& error,
            const size_t& bytesTransferred);
        void handleAnnounceSend(const boost::system::error_code& error);
        void handleAnnounceReceive(const boost::system::error_code& error,
            const size_t& bytesTransferred);


        void scrapeRequest();
        void announceRequest();

        void handleScrapeResp();
        void handleAnnounceResp();

        void run(std::chrono::steady_clock::duration timeout);
    };


}

