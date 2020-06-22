#pragma once
#include <boost/array.hpp>
#include <boost/asio.hpp>

#include "ValueTypes.h"
#include "trackerUrl.h"

#include <unordered_map>


namespace Bittorrent
{
    using boost::asio::ip::udp;
    using boost::asio::ip::address;

    class UDPClient
    {
    public:
        boost::posix_time::ptime connIDReceivedTime;
        boost::posix_time::ptime lastRequestTime;

        //announce response data
        std::chrono::seconds peerRequestInterval;
        int leechers;
        int seeders;
        int completed;
        std::string errMessage;
        std::vector<peer> peerList;

        void dataTransmission(bool isAnnounce);

        //default constructor & destructor
        UDPClient(trackerUrl& parsedUrl, std::vector<byte>& clientID,
            std::vector<byte>& infoHash, long long& uploaded,
            long long& downloaded, long long& remaining, int& intEvent,
            int& port, bool isAnnounce);
        ~UDPClient();

    private:
        //general variables
        std::string peerHost;
        std::string peerPort;
        int localPort;
        std::vector<byte> errorAction;
        bool isFail;
        std::string logBuffer;
        bool m_isAnnounce;

        //sent variables
        std::vector<byte> protocolID;
        std::vector<byte> sentTransactionID;
        std::vector<byte> connectAction;

        //received variables
        std::vector<byte> connectionID;

        //scrape variables
        std::vector<byte> scrapeAction;
        std::vector<byte> byteInfoHash;

        //announce (send) variables
        std::vector<byte> ancAction;
        std::vector<byte> ancClientID;
        long long ancDownloaded;
        long long ancUploaded;
        long long ancRemaining;
        int ancIntEvent;

        //send/receive buffers
        std::vector<byte> recConnBuffer;
        std::vector<byte> recScrapeBuffer;
        std::vector<byte> recAncBuffer;

        boost::asio::io_context io_context;
        udp::socket socket;
        udp::endpoint remoteEndpoint;
        udp::endpoint localEndpoint;

        void handleConnect(const boost::system::error_code& error);
        void handleConnectSend(const boost::system::error_code& error,
            const size_t& bytesTransferred);
        void handleConnectReceive(const boost::system::error_code& error,
            const size_t& bytesTransferred);
        void handleScrapeSend(const boost::system::error_code& error,
            const size_t& bytesTransferred);
        void handleScrapeReceive(const boost::system::error_code& error,
            const size_t& bytesTransferred);
        void handleAnnounceSend(const boost::system::error_code& error,
            const size_t& bytesTransferred);
        void handleAnnounceReceive(const boost::system::error_code& error,
            const size_t& bytesTransferred);

        std::vector<byte> buildScrapeReq();
        std::vector<byte> buildConnectReq();
        std::vector<byte> buildAnnounceReq();

        void handleConnectResp(const std::size_t& connBytesRec);
        void handleScrapeResp(const std::size_t& scrapeBytesRec);
        void handleAnnounceResp(const std::size_t& AncBytesRec);

        void scrapeRequest();
        void announceRequest();

        void close();

        void run(std::chrono::steady_clock::duration timeout);
    };
}
