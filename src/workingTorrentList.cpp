#include "workingTorrentList.h"
#include "Decoder.h"
#include "TorrentManipulation.h"
#include "loguru.h"

namespace Bittorrent
{
    using namespace torrentManipulation;

    workingTorrentList::workingTorrentList()
    {

    }

    void workingTorrentList::addNewTorrent(std::string fileName,
                                           std::string buffer)
    {
        valueDictionary decodedTorrent =
                boost::get<valueDictionary>(Decoder::decode(buffer));
        Torrent loadedTorrent = toTorrentObj(fileName.c_str(), decodedTorrent);

        auto loadedTorrent_ptr = std::make_shared<Torrent>(loadedTorrent);
        torrentList.push_back(loadedTorrent_ptr);

          LOG_F(INFO, "Added Torrent %s to client!", loadedTorrent.generalData.fileName.c_str());
    }


}


