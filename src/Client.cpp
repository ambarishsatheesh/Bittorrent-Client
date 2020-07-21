#include "Client.h"
#include "loguru.h"
#include <random>

namespace Bittorrent
{
    Client::Client()
        : port{ 0 }, localID()
	{
        LOG_SCOPE_F(INFO, "ClientID");
        LOG_F(INFO, "Generating new Client ID...");

        //generate 20 byte client ID
        std::random_device dev;
        std::mt19937 rng(dev());
        std::uniform_int_distribution<int32_t>
            dist6(0, std::numeric_limits<int32_t>::max());

        localID.push_back('-');
        localID.push_back('A');
        localID.push_back('S');
        localID.push_back('0');
        localID.push_back('5');
        localID.push_back('0');
        localID.push_back('0');
        localID.push_back('-');

        for (size_t i = 8; i < 20; ++i)
        {
            localID.push_back((dist6(rng) >> 8) & 0xff);
        }

        LOG_F(INFO, "Client ID generated!");

        auto clientLog = toHex(localID);
        LOG_F(INFO, "Client ID (hex): %s", clientLog.c_str());

        WorkingTorrents.clientID = localID;
	}
}
