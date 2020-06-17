#pragma once
#include "boost/date_time/posix_time/posix_time.hpp"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/buffers_iterator.hpp>
#include <boost/signals2.hpp>

#include "UDPClient.h"
#include "HTTPClient.h"
#include "ValueTypes.h"
#include "TorrentStatus.h"

#include <memory>
#include <string>


namespace Bittorrent
{
    using tcp = boost::asio::ip::tcp;
    namespace http = boost::beast::http;

    class trackerObj
    {
    public:
        std::string trackerAddress;
        boost::posix_time::ptime lastPeerRequest;
        boost::posix_time::seconds peerRequestInterval;

        //udp scrape data
        int seeders;
        int leechers;
        int complete;
        int incomplete;

        //peer list
        std::vector<peer> peerList;

        //need shared ptr to signal so it can be copyable/movable
        using sigPeer = boost::signals2::signal<void(std::vector<peer>)>;
        std::shared_ptr<sigPeer> peerListUpdated;

        void update(TorrentStatus::currentStatus currentState,
                    std::vector<byte> clientID, int port,
                    std::string urlEncodedInfoHash, std::vector<byte> infoHash,
                    long long uploaded, long long downloaded,
                    long long remaining);

        void resetLastRequest();

        //default constructor
        trackerObj();
    };
}

