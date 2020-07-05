#include "throttle.h"

namespace Bittorrent{

Throttle::Throttle(int maxDataSize, std::chrono::duration<int> maxTimeWindow)
    : maxDataSize{maxDataSize}, maxTimeWindow{maxTimeWindow}
{
}

void Throttle::add(int size)
{
    transferPacketVec.emplace_back(size,
                                   std::chrono::high_resolution_clock::now());
}

bool Throttle::isThrottled()
{
    highResClock::time_point cutoff = highResClock::now() - maxTimeWindow;

    //remove all transferPacket objects created before cutoff time
    transferPacketVec.erase(
                std::remove_if(
                    transferPacketVec.begin(), transferPacketVec.end(),
                    [&]
                    (const transferPacket& p){return p.dataTime < cutoff;}),
                transferPacketVec.end());

        return std::accumulate(
                    transferPacketVec.begin(), transferPacketVec.end(), 0,
                        [&]
                    (int sum, const transferPacket& p)
        {return sum + p.dataSize;}) > maxDataSize;
}

}
