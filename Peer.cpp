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
		IsInterestedReceived{ false },  
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
		IsInterestedReceived{ false }, 
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
		IsInterestedReceived{ false }, 
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

	//count how many pieces that aren't verified (i.e. are required) 
	//are downloaded
	int Peer::piecesRequiredAvailable()
	{
		//custom comparator (false verified, true downloaded)
		const auto lambda = [this](bool i) {return i &&
			!peerTorrent.get()->statusData.isPieceVerified.at(i); };

		return std::count_if(isPieceDownloaded.cbegin(),
			isPieceDownloaded.cend(), lambda);
	}

	int Peer::piecesDownloadedCount()
	{
		return std::count(isPieceDownloaded.cbegin(), 
			isPieceDownloaded.cend(), true);
	}

	bool Peer::isCompleted()
	{
		return piecesDownloadedCount() == peerTorrent.get()->piecesData.pieceCount;
	}

	//count how many total blocks are requested
	int Peer::blocksRequested()
	{
		int sum = 0;
		for (size_t i = 0; i < isBlockRequested.size(); ++i)
		{
			sum += std::count(isBlockRequested.at(i).cbegin(), 
				isBlockRequested.at(i).cend(), true);
		}

		return sum;
	}
}