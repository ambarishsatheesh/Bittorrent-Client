﻿#include "trackerObj.h"
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
        leechers(std::numeric_limits<int>::min()), errMessage{""},
        isWorking{false}, peerListUpdated{ std::make_shared<sigPeer>() }
    {
    }

    //create required url for GET request
    void trackerObj::update(TorrentStatus::currentStatus currentState,
        std::vector<byte> clientID, int httpPort, int udpPort,
        std::string urlEncodedInfoHash, std::vector<byte> infoHash,
        long long uploaded, long long downloaded, long long remaining)
    {
        //switch case to get enumerator string
        std::string stringEvent;
        int intEvent = 0;
        switch (currentState)
        {
        case TorrentStatus::currentStatus::completed:
                stringEvent = "completed";
                intEvent = 1;
                break;
        case TorrentStatus::currentStatus::started:
                stringEvent = "started";
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

        //handle tracker url without announce
        std::string sep = "";
        if (trackerAddress.find("announce") == std::string::npos)
        {
            sep = "/";
        }

        //this is for http requests
        //compact will be 1 but client will also support non-compact response
        std::string url = trackerAddress + sep + "?info_hash=" + urlEncodedInfoHash +
            "&peer_id=" + urlEncodedClientID + "&port=" + std::to_string(httpPort)
            + "&uploaded=" + std::to_string(uploaded) + "&downloaded=" +
            std::to_string(downloaded) + "&left=" + std::to_string(remaining) +
            "&event=" + stringEvent + "&compact=1";

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
            if (seeders == std::numeric_limits<int>::min() &&
                leechers == std::numeric_limits<int>::min())
            {
                HTTPClient httpAnnounce(parsedUrl, httpPort, 1);
                isWorking = !httpAnnounce.isFail;
                errMessage = httpAnnounce.errMessage;

                if (isWorking)
                {
                    seeders = httpAnnounce.seeders;
                    leechers = httpAnnounce.leechers;
                    peerRequestInterval = httpAnnounce.peerRequestInterval;

                    //call signal to fire peerListUpdated event
                    //not thread safe?
                    (*peerListUpdated)(httpAnnounce.peerList);

                    //replace peer vector with new vector
                    peerList = httpAnnounce.peerList;
                }
            }
            else
            {
                //scrape
                HTTPClient httpGen(parsedUrl, httpPort, 0);
                isWorking = !httpGen.isFail;
                errMessage = httpGen.errMessage;

                //announce and update if seeders/leechers values change
                if (httpGen.seeders != seeders ||
                        httpGen.leechers != leechers)
                {
                    LOG_F(INFO,
                        "Tracker (%s:%s) info changed since last scrape. "
                        "Switching to HTTP announce request.",
                        parsedUrl.hostname.c_str(), parsedUrl.port.c_str());

                    //reset tracker url target (scrape changes it)
                    httpGen.target = parsedUrl.target;

                    httpGen.dataTransmission(1);
                    isWorking = !httpGen.isFail;
                    errMessage = httpGen.errMessage;

                    if (errMessage.empty())
                    {
                        seeders = httpGen.seeders;
                        leechers = httpGen.leechers;
                        peerRequestInterval = httpGen.peerRequestInterval;

                        //call signal to fire peerListUpdated event
                        //not thread safe?
                        (*peerListUpdated)(httpGen.peerList);

                        //replace peer vector with new vector
                        peerList = httpGen.peerList;
                    }
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
                    downloaded, remaining, intEvent, udpPort, 1);

                isWorking = !udpAnnounce.isFail;

                if (isWorking)
                {
                    seeders = udpAnnounce.seeders;
                    leechers = udpAnnounce.leechers;
                    peerRequestInterval = udpAnnounce.peerRequestInterval;

                    //call signal to fire peerListUpdated event
                    //not thread safe?
                    (*peerListUpdated)(udpAnnounce.peerList);

                    //replace peer vector with new vector
                    peerList = udpAnnounce.peerList;
                }
            }
            else
            {
                //scrape
                UDPClient udpGen(parsedUrl, clientID, infoHash, uploaded,
                    downloaded, remaining, intEvent, udpPort, 0);
                isWorking = !udpGen.isFail;
                errMessage = udpGen.errMessage;

                //announce and update if seeders/leechers values change
                if (udpGen.seeders != seeders || udpGen.leechers != leechers)
                {
                    LOG_F(INFO,
                        "Tracker (%s:%s) info changed since last scrape. "
                        "Switching to UDP announce request.",
                        parsedUrl.hostname.c_str(), parsedUrl.port.c_str());

                    udpGen.dataTransmission(1);
                    isWorking = !udpGen.isFail;
                    errMessage = udpGen.errMessage;

                    if (isWorking)
                    {
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
            }

            //handle peers, seeders, leechers, interval
            //need some event handling to scrape/announce after interval time has passed
        }
        else
        {
            return;
        }
    }

    void trackerObj::resetLastRequest()
    {
        lastPeerRequest =
        {std::chrono::high_resolution_clock::time_point::min()};
    }
}
