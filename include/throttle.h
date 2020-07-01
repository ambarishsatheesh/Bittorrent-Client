#ifndef THROTTLE_H
#define THROTTLE_H

#include <mutex>
#include <chrono>
#include <vector>
#include <algorithm>
#include <numeric>

namespace Bittorrent{

class Throttle
{
    using highResClock = std::chrono::high_resolution_clock;

public:
    int maxDataSize;
    std::chrono::duration<int> maxTimeWindow;

    struct transferPacket
    {
        int dataSize;
        highResClock::time_point dataTime;

        transferPacket(int size, highResClock::time_point time)
            : dataSize{size}, dataTime{time}
        {}
    };

    std::vector<transferPacket> transferPacketVec;

    void add(int size);
    bool isThrottled();

    Throttle(int maxDataSize, std::chrono::duration<int> maxTimeWindow);

private:
    std::mutex mtx_throttle;
};

}

#endif // THROTTLE_H
