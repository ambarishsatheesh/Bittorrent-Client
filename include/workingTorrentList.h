#ifndef WORKINGTORRENTLIST_H
#define WORKINGTORRENTLIST_H

#endif // WORKINGTORRENTLIST_H

#include "Torrent.h"

#include <vector>

namespace Bittorrent
{
    class workingTorrentList
    {
    public:
        std::vector<std::shared_ptr<Torrent>> torrentList;

        void addNewTorrent(std::string fileName, std::string buffer);
        void removeTorrent(int rowNum);

        workingTorrentList();

    };
}
