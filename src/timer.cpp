#include "timer.h"

namespace Bittorrent {


TrackerTimer::TrackerTimer(std::vector<byte> clientID)
    : clientID{clientID}, tempTime{0}
{
}

TrackerTimer::~TrackerTimer()
{
    if (wait_thread.joinable())
    {
        wait_thread.join();
    }
}

void TrackerTimer::startTimer(pSet* trackerSet)
{
    wait_thread = std::thread(&TrackerTimer::wait_then_call, this, trackerSet);
}

bool TrackerTimer::isRunning()
{
    return wait_thread.joinable();
}

void TrackerTimer::wait_then_call(pSet* trackerSet)
{
    while (!trackerSet->empty())
    {
        loguru::set_thread_name("Timer Thread");
        LOG_F(INFO, "Beginning tracker update...");

        std::unique_lock<std::mutex> lck{ mtx };
        cv.wait_for(lck, trackerSet->begin()->
                    first->peerRequestInterval - tempTime);

        //update first tracker in set
        trackerSet->begin()->first->
                update(TorrentStatus::currentStatus::started,
                       clientID, 0,
                       trackerSet->begin()->second->hashesData.urlEncodedInfoHash,
                       trackerSet->begin()->second->hashesData.infoHash,
                       trackerSet->begin()->second->statusData.uploaded(),
                       trackerSet->begin()->second->statusData.downloaded(),
                       trackerSet->begin()->second->statusData.remaining());

        //re-insert element (with updated peer interval)
        trackerSet->emplace(std::make_pair(trackerSet->begin()->first, trackerSet->begin()->second));

        tempTime = trackerSet->begin()->first->peerRequestInterval;

        //remove first element so next tracker can update
        trackerSet->erase(trackerSet->begin());
    }

    //join thread if set becomes empty
    if (wait_thread.joinable())
    {
        wait_thread.join();
    }
}

}
