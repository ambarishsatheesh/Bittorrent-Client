#include "throttle.h"

namespace Bittorrent{

Throttle::Throttle(int maxDataSize, std::chrono::duration<int> maxTimeWindow)
    : maxDataSize{maxDataSize}, maxTimeWindow{maxTimeWindow}
{
}

void Throttle::add(int size)
{
    std::lock_guard<std::mutex> throttleGuard(mtx_throttle);

    transferPacketVec.emplace_back(size,
                                   std::chrono::high_resolution_clock::now());
}

bool Throttle::isThrottled()
{
    std::lock_guard<std::mutex> throttleGuard(mtx_throttle);

    highResClock::time_point cutoff = highResClock::now() - maxTimeWindow;

    //remove all transferPacket objects created before cutoff time
    transferPacketVec.erase(
                std::remove_if(
                    transferPacketVec.begin(), transferPacketVec.end(),
                    [&]
                    (const transferPacket& p){return p.dataTime < cutoff;}));

    return std::accumulate(
                transferPacketVec.begin(), transferPacketVec.end(), 0,
                    []
                (int sum, const transferPacket& p){return sum + p.dataSize;});
}

}
