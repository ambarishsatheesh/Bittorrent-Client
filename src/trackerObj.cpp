#include "trackerObj.h"
#include "Decoder.h"
#include "Utility.h"
#include "loguru.h"


namespace Bittorrent
{
    using namespace utility;

    trackerObj::trackerObj()
        : trackerAddress{ "" },
        lastPeerRequest{ std::chrono::high_resolution_clock::time_point::min()},
        peerRequestInterval{ 1800 }, seeders(std::numeric_limits<int>::min()),
        leechers(std::numeric_limits<int>::min()),
        complete{ std::numeric_limits<int>::min() },
        incomplete{ std::numeric_limits<int>::min() },
        peerListUpdated{ std::make_shared<sigPeer>() }
    {
    }

    //create required url for GET request
    void trackerObj::update(TorrentStatus::currentStatus currentState, std::vector<byte> clientID,
        int port, std::string urlEncodedInfoHash, std::vector<byte> infoHash,
        long long uploaded, long long downloaded, long long remaining)
    {
        //switch case to get enumerator string
        std::string stringEvent;
        int intEvent = 0;
        switch (currentState)
        {
        case TorrentStatus::currentStatus::started:
                stringEvent = "started";
                intEvent = 1;
                break;
        case TorrentStatus::currentStatus::paused:
                stringEvent = "paused";
                intEvent = 2;
                break;
        case TorrentStatus::currentStatus::stopped:
                stringEvent = "stopped";
                intEvent = 3;
                break;
        }

        //URL encode clientID
        std::string clientIDString(clientID.begin(), clientID.end());
        auto urlEncodedClientID = urlEncode(clientIDString);

        //compact will be 1 but client will also support non-compact response
        std::string url = trackerAddress + "?info_hash=" + urlEncodedInfoHash +
            "&peer_id=" + urlEncodedClientID + "&port=" + std::to_string(port)
            + "&uploaded=" + std::to_string(uploaded) + "&downloaded=" +
            std::to_string(downloaded) + "&left=" + std::to_string(remaining) +
            "&event=" + std::to_string(intEvent) + "&compact=1";

        //only requesting new peers if request interval has passed
        if (currentState == TorrentStatus::currentStatus::started &&
            std::chrono::high_resolution_clock::now() <
            (lastPeerRequest + peerRequestInterval))
        {
            return;
        }

        //parse url and check if valid
        trackerUrl parsedUrl(url);
        parsedUrl.parse();
        if (parsedUrl.isInvalidURL)
        {
            LOG_F(ERROR, "Tracker (%s) is invalid!",
                trackerAddress.c_str());
            return;
        }

        //temp vector for printing peers
        //std::vector<peer> tempPeerList;

        //request url using appropriate protocol
        if (parsedUrl.protocol == trackerUrl::protocolType::http)
        {
            if (complete == std::numeric_limits<int>::min() &&
                incomplete == std::numeric_limits<int>::min())
            {
                HTTPClient httpAnnounce(parsedUrl, 0);
                complete = httpAnnounce.complete;
                incomplete = httpAnnounce.incomplete;

                //call signal to fire peerListUpdated event
                //not thread safe?
                (*peerListUpdated)(httpAnnounce.peerList);
                //replace peer vector with new vector
                peerList = httpAnnounce.peerList;
            }
            else
            {
                //scrape
                HTTPClient httpGen(parsedUrl, 0);
                //announce and update if seeders/leechers values change
                if (httpGen.complete != complete ||
                    httpGen.incomplete != incomplete)
                {
                    LOG_F(INFO,
                        "Tracker (%s:%s) info changed since last scrape. "
                        "Switching to HTTP announce request.",
                        parsedUrl.hostname.c_str(), parsedUrl.port.c_str());

                    //reset tracker url target (scrape changes it)
                    httpGen.target = parsedUrl.target;

                    httpGen.dataTransmission(1);
                    complete = httpGen.complete;
                    incomplete = httpGen.incomplete;
                    //call signal to fire peerListUpdated event
                    //not thread safe?
                    (*peerListUpdated)(httpGen.peerList);
                    //replace peer vector with new vector
                    peerList = httpGen.peerList;
                }
            }
            //handle peers, seeders, leechers, interval
            //need some event handling to scrape/announce after interval time has passed
        }
        else if (parsedUrl.protocol == trackerUrl::protocolType::udp)
        {
            //announce if first time, otherwise scrape
            if (seeders == std::numeric_limits<int>::min() &&
                leechers == std::numeric_limits<int>::min())
            {
                UDPClient udpAnnounce(parsedUrl, clientID, infoHash, uploaded,
                    downloaded, remaining, intEvent, port, 0);
                seeders = udpAnnounce.seeders;
                leechers = udpAnnounce.leechers;
                peerRequestInterval = udpAnnounce.peerRequestInterval;
                //call signal to fire peerListUpdated event
                //not thread safe?
                (*peerListUpdated)(udpAnnounce.peerList);
                //replace peer vector with new vector
                peerList = udpAnnounce.peerList;
            }
            else
            {
                UDPClient udpGen(parsedUrl, clientID, infoHash, uploaded,
                    downloaded, remaining, intEvent, port, 0);
                //announce and update if seeders/leechers values change
                if (udpGen.seeders != seeders || udpGen.leechers != leechers)
                {
                    LOG_F(INFO,
                        "Tracker (%s:%s) info changed since last scrape. "
                        "Switching to UDP announce request.",
                        parsedUrl.hostname.c_str(), parsedUrl.port.c_str());

                    udpGen.dataTransmission(1);
                    seeders = udpGen.seeders;
                    leechers = udpGen.leechers;
                    peerRequestInterval = udpGen.peerRequestInterval;
                    //call signal to fire peerListUpdated event
                    //not thread safe?
                    (*peerListUpdated)(udpGen.peerList);
                    //replace peer vector with new vector
                    peerList = udpGen.peerList;
                }
            }

            //handle peers, seeders, leechers, interval
            //need some event handling to scrape/announce after interval time has passed
        }
        else
        {
            return;
        }

        //update request time
        lastPeerRequest = std::chrono::high_resolution_clock::now();

        std::cout << "\n" << "Received peer information from " << trackerAddress << "\n";
        std::cout << "Peer count:  " << peerList.size() << "\n";
        std::cout << "Peers: ";
        for (auto peer : peerList)
        {
            std::cout << "IP Address: " << peer.ipAddress << ", Port: " <<
                peer.port << "\n";
        }
    }

    void trackerObj::resetLastRequest()
    {
        lastPeerRequest =
        {std::chrono::high_resolution_clock::time_point::min()};
    }
}
