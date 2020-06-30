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

        void handlePeerListUpdated(peer* singlePeer);

        //signals
        std::shared_ptr<boost::signals2::signal<void(peer*, Torrent*)>> sig_addPeer;
        void addPeer(peer* singlePeer, Torrent* torrent);

		std::shared_ptr<Torrent> getPtr();

	private:

	};
}

#endif // TORRENT_H
