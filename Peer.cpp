#include "Peer.h"

#include <iostream>
#include <boost/algorithm/string/join.hpp>

namespace Bittorrent
{
	Peer::Peer(Torrent& torrent, std::string& localID, tcp::endpoint& endpoint)
		: localID{ localID }, peerID{ "" }, 
		peerTorrent{ std::make_shared<Torrent>(torrent) }, key{ "" },
		isPieceDownloaded(torrent.piecesData.pieceCount),  isDisconnected{}, 
		isPositionSent{}, isChokeSent{ true },
		isInterestSent{ false }, isHandshakeReceived{}, IsChokeReceived{ true },
		IsInterestedReceived{ false }, blocksRequested{ 0 }, 
		lastActive{ boost::posix_time::second_clock::local_time() },
		lastKeepAlive{ boost::posix_time::min_date_time }, uploaded{ 0 }, 
		downloaded{ 0 }, io_context(), socket(io_context), endpoint(endpoint)
	{
		isBlockRequested.resize(peerTorrent.get()->piecesData.pieceCount);
		for (size_t i = 0; i < peerTorrent.get()->piecesData.pieceCount; ++i)
		{
			isBlockRequested.at(i).resize(peerTorrent->piecesData.setBlockCount(i));
		}
	}

	Peer::Peer(Torrent& torrent, std::string& localID)
		: localID{ localID }, peerID{ "" }, 
		peerTorrent{ std::make_shared<Torrent>(torrent) }, key{ "" },
		isPieceDownloaded(torrent.piecesData.pieceCount), isDisconnected{}, 
		isPositionSent{}, isChokeSent{ true },
		isInterestSent{ false }, isHandshakeReceived{}, IsChokeReceived{ true },
		IsInterestedReceived{ false }, blocksRequested{ 0 }, 
		lastActive{ boost::posix_time::second_clock::local_time() },
		lastKeepAlive{ boost::posix_time::min_date_time }, uploaded{ 0 },
		downloaded{ 0 }, io_context(), socket(io_context), endpoint()
	{
		isBlockRequested.resize(peerTorrent.get()->piecesData.pieceCount);
		for (size_t i = 0; i < peerTorrent.get()->piecesData.pieceCount; ++i)
		{
			isBlockRequested.at(i).resize(peerTorrent.get()->piecesData.setBlockCount(i));
		}
	}

	Peer::Peer(Torrent& torrent, std::string& localID, tcp::socket* tcpClient)
		: localID{ localID }, peerID{ "" }, 
		peerTorrent{ std::make_shared<Torrent>(torrent) }, key{ "" },
		isPieceDownloaded(torrent.piecesData.pieceCount), isDisconnected{}, 
		isPositionSent{}, isChokeSent{ true },
		isInterestSent{ false }, isHandshakeReceived{}, IsChokeReceived{ true },
		IsInterestedReceived{ false }, blocksRequested{ 0 }, 
		lastActive{ boost::posix_time::second_clock::local_time() },
		lastKeepAlive{ boost::posix_time::min_date_time }, uploaded{ 0 },
		downloaded{ 0 }, io_context(), socket(io_context), endpoint()
	{
		isBlockRequested.resize(peerTorrent.get()->piecesData.pieceCount);
		for (size_t i = 0; i < peerTorrent.get()->piecesData.pieceCount; ++i)
		{
			isBlockRequested.at(i).resize(peerTorrent.get()->
				piecesData.setBlockCount(i));
		}

		//not sure if this will be enough to keep connection started by peer?
		endpoint = tcpClient->remote_endpoint();
	}

	std::string Peer::piecesDownloaded()
	{
		//create temp vec to store string bool, then return all as single string
		std::vector<std::string> tempVec(isPieceDownloaded.size());
		for (auto i : isPieceDownloaded)
		{
			if (i == 1)
			{
				tempVec.at(i) = "1";
			}
			else
			{
				tempVec.at(i) = "0";
			}
		}
		return boost::algorithm::join(tempVec, "");
	}

	//check if pieces that are not verified (i.e. required) are downloaded
	int Peer::piecesRequiredAvailable()
	{
		//custom c
		/*auto lambda = [&](bool i) {return isPieceDownloaded[i] &&
			!peerTorrent->statusData.isPieceVerified[i]; };

		return std::count(isPieceDownloaded.begin(),
			isPieceDownloaded.end(), lambda);*/
	}

	int Peer::piecesDownloadedCount()
	{
		return std::count(isPieceDownloaded.begin(), 
			isPieceDownloaded.end(), true);
	}

	bool Peer::isCompleted()
	{
		return piecesDownloadedCount() == peerTorrent.get()->piecesData.pieceCount;
	}


}