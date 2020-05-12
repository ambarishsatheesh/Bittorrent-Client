#include "Peer.h"

#include <iostream>
#include <boost/algorithm/string/join.hpp>

namespace Bittorrent
{
	Peer::Peer(std::shared_ptr<Torrent> torrent, std::string& localID, 
		boost::asio::io_context& io_context, tcp::resolver::results_type& results)
		: localID{ localID }, peerID{ "" }, 
		peerTorrent{ torrent->getPtr() }, key{ "" },
		isPieceDownloaded(peerTorrent.get()->piecesData.pieceCount),  isDisconnected{},
		isPositionSent{}, isChokeSent{ true },
		isInterestSent{ false }, isHandshakeReceived{}, IsChokeReceived{ true },
		IsInterestedReceived{ false },  
		lastActive{ boost::posix_time::second_clock::local_time() },
		lastKeepAlive{ boost::posix_time::min_date_time }, uploaded{ 0 }, 
		downloaded{ 0 }, socket(io_context), peerResults(results),
		endpoint(), deadline{io_context}, heartbeatTimer{io_context}
	{
		isBlockRequested.resize(peerTorrent.get()->piecesData.pieceCount);
		for (size_t i = 0; i < peerTorrent.get()->piecesData.pieceCount; ++i)
		{
			isBlockRequested.at(i).resize(peerTorrent->piecesData.setBlockCount(i));
		}

		//begin connecting
		connectToNewPeer(results.begin());

		// Start the deadline actor. The connect and input actors will
		// update the deadline prior to each asynchronous operation.
		deadline.async_wait(boost::bind(&Peer::check_deadline, this));
	}

	//construct peer from accepted connection started by another peer
	Peer::Peer(std::shared_ptr<Torrent> torrent, std::string& localID, 
		boost::asio::io_context& io_context, tcp::socket tcpClient)
		: localID{ localID }, peerID{ "" }, 
		peerTorrent{ torrent->getPtr() }, key{ "" },
		isPieceDownloaded(peerTorrent.get()->piecesData.pieceCount), isDisconnected{},
		isPositionSent{}, isChokeSent{ true },
		isInterestSent{ false }, isHandshakeReceived{}, IsChokeReceived{ true },
		IsInterestedReceived{ false }, 
		lastActive{ boost::posix_time::second_clock::local_time() },
		lastKeepAlive{ boost::posix_time::min_date_time }, uploaded{ 0 },
		downloaded{ 0 }, socket(std::move(tcpClient)), endpoint(), 
		deadline{ io_context }, heartbeatTimer{ io_context }
	{
		isBlockRequested.resize(peerTorrent.get()->piecesData.pieceCount);
		for (size_t i = 0; i < peerTorrent.get()->piecesData.pieceCount; ++i)
		{
			isBlockRequested.at(i).resize(peerTorrent.get()->
				piecesData.setBlockCount(i));
		}

		//get endpoint from accepted connection
		endpoint = socket.remote_endpoint();
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
		//custom comparator (!false verified && true downloaded)
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

	void Peer::connectToNewPeer(tcp::resolver::results_type::iterator endpointItr)
	{
		if (endpointItr != peerResults.end())
		{
			std::cout << "Trying to connect to: " << endpointItr->endpoint() 
				<< "...\n";

			// Set a deadline for the connect operation (10s)
			deadline.expires_after(boost::asio::chrono::seconds(10));

			// Start the asynchronous connect operation.
			socket.async_connect(endpointItr->endpoint(),
				boost::bind(&Peer::handleNewConnect, this,
					boost::placeholders::_1, endpointItr));
		}
		else
		{
			// There are no more endpoints to try. Shut down the client.
			disconnect();
		}
	}

	void Peer::handleNewConnect(const boost::system::error_code& ec,
		tcp::resolver::results_type::iterator endpointItr)
	{


	}

	void Peer::disconnect()
	{
		std::cout << "\n" << "Disconnecting..." << "\n";

		isDisconnected = true;
		boost::system::error_code ignored_ec;
		socket.close(ignored_ec);
		deadline.cancel();
		heartbeatTimer.cancel();

		std::cout << "\n" << "Disconnected from peer, downloaded: " << downloaded
			<< ", uploaded: " << uploaded << "\n";

		//call slot
		disconnected();
	}

	void Peer::check_deadline()
	{
		if (isDisconnected)
			return;

		// Check whether the deadline has passed. We compare the deadline against
		// the current time since a new asynchronous operation may have moved the
		// deadline before this actor had a chance to run.
		if (deadline.expiry() <= boost::asio::steady_timer::clock_type::now())
		{
			// The deadline has passed. The socket is closed so that any outstanding
			// asynchronous operations are cancelled.
			socket.close();

			// There is no longer an active deadline. The expiry is set to the
			// maximum time point so that the actor takes no action until a new
			// deadline is set.
			deadline.expires_at(boost::asio::steady_timer::time_point::max());
		}

		// Put the actor back to sleep.
		deadline.async_wait(boost::bind(&Peer::check_deadline, this));
	}
}
