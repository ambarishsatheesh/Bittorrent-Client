#ifndef TORRENT_H
#define TORRENT_H

#include <string>
#include <vector>
#include <memory>
#include <QtConcurrent>

#include "fileObj.h"
#include "TorrentGeneral.h"
#include "TorrentPieces.h"
#include "TorrentHashes.h"
#include "TorrentStatus.h"


namespace Bittorrent
{
	class Torrent
		: public std::enable_shared_from_this<Torrent>
	{
    public:
        //ranking
        int clientRank;

		//general info
		TorrentGeneral generalData;

		//file info
		std::vector<fileObj> fileList;

		//pieces & blocks
		TorrentPieces piecesData;

		//hashes
		TorrentHashes hashesData;

		//Status
		TorrentStatus statusData;

        //constructor
        Torrent();

		//set file info
        void setFileList(const valueDictionary& torrent);
		valueDictionary filesToDictionary(valueDictionary& dict);

        //update peer list from tracker response
        void handlePeerListUpdated();

        //signals
        std::shared_ptr<boost::signals2::signal<void(
                peer, std::shared_ptr<Torrent>)>> sig_addPeer;

        //piece verification signal
        std::shared_ptr<boost::signals2::signal<void(
                Torrent*, int)>> sig_pieceVerified;

		std::shared_ptr<Torrent> getPtr();

	private:

	};
}

#endif // TORRENT_H
