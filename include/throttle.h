#ifndef THROTTLE_H
#define THROTTLE_H

#include <mutex>

class Throttle
{
public:
    static constexpr int maxDownloadBytesPerSecond = 4194300;
    static constexpr int maxUploadBytesPerSecond = 512000;

    std::mutex mtx_throttle;


    Throttle();
};

#endif // THROTTLE_H
