#include "Peer.h"

namespace Bittorrent
{
	Peer::Peer(Torrent& torrent, std::string& localID, tcp::endpoint& endpoint)
		: localID{ localID }, peerID{ "" }, 
		torrent{ std::make_unique<Torrent>(torrent) }, key{ "" }, 
		isPieceDownloaded(torrent.piecesData.pieceCount), piecesDownloaded{ "" },
		piecesRequiredAvailable{ 0 }, piecesDownloadedCount{ 0 },
		isCompleted{}, isDisconnected{}, isPositionSent{}, isChokeSent{ true },
		isInterestSent{ false }, isHandshakeReceived{}, IsChokeReceived{ true },
		IsInterestedReceived{ false }, blocksRequested{ 0 }, 
		lastActive{ boost::posix_time::second_clock::local_time() },
		lastKeepAlive{ boost::posix_time::min_date_time }, uploaded{ 0 }, 
		downloaded{ 0 }, io_context(), socket(io_context), endpoint(endpoint)
	{
		isBlockRequested.resize(torrent.piecesData.pieceCount);
		for (size_t i = 0; i < torrent.piecesData.pieceCount; ++i)
		{
			isBlockRequested.at(i).resize(torrent.piecesData.setBlockCount(i));
		}
	}

	Peer::Peer(Torrent& torrent, std::string& localID)
		: localID{ localID }, peerID{ "" }, 
		torrent{ std::make_unique<Torrent>(torrent) }, key{ "" }, 
		isPieceDownloaded(torrent.piecesData.pieceCount), piecesDownloaded{ "" },
		piecesRequiredAvailable{ 0 }, piecesDownloadedCount{ 0 },
		isCompleted{}, isDisconnected{}, isPositionSent{}, isChokeSent{ true },
		isInterestSent{ false }, isHandshakeReceived{}, IsChokeReceived{ true },
		IsInterestedReceived{ false }, blocksRequested{ 0 }, 
		lastActive{ boost::posix_time::second_clock::local_time() },
		lastKeepAlive{ boost::posix_time::min_date_time }, uploaded{ 0 },
		downloaded{ 0 }, io_context(), socket(io_context), endpoint()
	{
		isBlockRequested.resize(torrent.piecesData.pieceCount);
		for (size_t i = 0; i < torrent.piecesData.pieceCount; ++i)
		{
			isBlockRequested.at(i).resize(torrent.piecesData.setBlockCount(i));
		}
	}

	Peer::Peer(Torrent& torrent, std::string& localID, tcp::socket* tcpClient)
		: localID{ localID }, peerID{ "" }, 
		torrent{ std::make_unique<Torrent>(torrent) }, key{ "" }, 
		isPieceDownloaded(torrent.piecesData.pieceCount), piecesDownloaded{ "" },
		piecesRequiredAvailable{ 0 }, piecesDownloadedCount{ 0 },
		isCompleted{}, isDisconnected{}, isPositionSent{}, isChokeSent{ true },
		isInterestSent{ false }, isHandshakeReceived{}, IsChokeReceived{ true },
		IsInterestedReceived{ false }, blocksRequested{ 0 }, 
		lastActive{ boost::posix_time::second_clock::local_time() },
		lastKeepAlive{ boost::posix_time::min_date_time }, uploaded{ 0 },
		downloaded{ 0 }, io_context(), socket(io_context), endpoint()
	{
		isBlockRequested.resize(torrent.piecesData.pieceCount);
		for (size_t i = 0; i < torrent.piecesData.pieceCount; ++i)
		{
			isBlockRequested.at(i).resize(torrent.piecesData.setBlockCount(i));
		}

		//not sure if this will be enough to keep connection started by peer?
		endpoint = tcpClient->remote_endpoint();
	}
}