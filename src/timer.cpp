#include "timer.h"

namespace Bittorrent {


TrackerTimer::TrackerTimer(std::vector<byte> clientID)
    : clientID{clientID}, stopThread{false}
{
}

TrackerTimer::~TrackerTimer()
{
    if (wait_thread.joinable())
    {
        wait_thread.join();
    }
}

void TrackerTimer::start(pSet* trackerSet)
{
    wait_thread = std::thread(&TrackerTimer::wait_then_call, this, trackerSet);
}

void TrackerTimer::stop()
{
    //join thread if set becomes empty
    if (wait_thread.joinable())
    {
        stopThread = true;
        cv.notify_all();
        wait_thread.join();
        LOG_F(INFO, "Ended tracker timer.");
    }

    stopThread = false;
}

bool TrackerTimer::isRunning()
{
    return wait_thread.joinable();
}

void TrackerTimer::wait_then_call(pSet* trackerSet)
{
    std::chrono::seconds tempTime{0};

    while (!trackerSet->empty() && !stopThread)
    {
        loguru::set_thread_name("Timer Thread");
        LOG_F(INFO, "Started tracker timer.");

        auto nextInterval =
                trackerSet->begin()->first->peerRequestInterval;

        std::unique_lock<std::mutex> lck{ mtx };
        cv.wait_for(lck, nextInterval - tempTime);

        if (stopThread)
        {
            return;
        }

        //check if current element has not been removed and then
        //update first tracker in set
        if (nextInterval == trackerSet->begin()->first->peerRequestInterval)
        {
            trackerSet->begin()->first->
                    update(TorrentStatus::currentStatus::started,
                           clientID, 0,
                           trackerSet->begin()->second->hashesData.urlEncodedInfoHash,
                           trackerSet->begin()->second->hashesData.infoHash,
                           trackerSet->begin()->second->statusData.uploaded(),
                           trackerSet->begin()->second->statusData.downloaded(),
                           trackerSet->begin()->second->statusData.remaining());

            //re-insert element (with updated peer interval)
            trackerSet->emplace(trackerSet->begin()->first, trackerSet->begin()->second);

            //set temp time for use in next set element
            tempTime = trackerSet->begin()->first->peerRequestInterval;

            //remove first element so next tracker can update
            trackerSet->erase(trackerSet->begin());
        }
        else
        {
            //update temp time
            tempTime = nextInterval;
        }
    }

    LOG_F(INFO, "Ended tracker timer.");

    //join thread if set becomes empty
    if (wait_thread.joinable())
    {
        wait_thread.join();
    }
}

}
