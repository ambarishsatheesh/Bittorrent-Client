#include "Peer.h"

#include <iostream>
#include <boost/algorithm/string/join.hpp>

namespace Bittorrent
{
	Peer::Peer(std::shared_ptr<Torrent> torrent, std::string& localID, tcp::endpoint& endpoint)
		: localID{ localID }, peerID{ "" }, 
		peerTorrent{ torrent->getPtr() }, key{ "" },
		isPieceDownloaded(peerTorrent.get()->piecesData.pieceCount),  isDisconnected{},
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

	Peer::Peer(std::shared_ptr<Torrent> torrent, std::string& localID)
		: localID{ localID }, peerID{ "" }, 
		peerTorrent{ torrent->getPtr() }, key{ "" },
		isPieceDownloaded(peerTorrent.get()->piecesData.pieceCount), isDisconnected{},
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

	//construct peer from accepted connection started by another peer
	Peer::Peer(std::shared_ptr<Torrent> torrent, std::string& localID, tcp::socket tcpClient)
		: localID{ localID }, peerID{ "" }, 
		peerTorrent{ torrent->getPtr() }, key{ "" },
		isPieceDownloaded(peerTorrent.get()->piecesData.pieceCount), isDisconnected{},
		isPositionSent{}, isChokeSent{ true },
		isInterestSent{ false }, isHandshakeReceived{}, IsChokeReceived{ true },
		IsInterestedReceived{ false }, 
		lastActive{ boost::posix_time::second_clock::local_time() },
		lastKeepAlive{ boost::posix_time::min_date_time }, uploaded{ 0 },
		downloaded{ 0 }, io_context(), socket(std::move(tcpClient)), endpoint()
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

	void Peer::connectToCreatedPeer()
	{


	}
}
//
//class client
//{
//public:
//    client(boost::asio::io_context& io_context)
//        :
//        socket_(io_context),
//        deadline_(io_context),
//        heartbeat_timer_(io_context)
//    {
//    }
//
//    // Called by the user of the client class to initiate the connection process.
//    // The endpoints will have been obtained using a tcp::resolver.
//    void start(tcp::resolver::results_type endpoints)
//    {
//        // Start the connect actor.
//        endpoints_ = endpoints;
//        start_connect(endpoints_.begin());
//
//        // Start the deadline actor. You will note that we're not setting any
//        // particular deadline here. Instead, the connect and input actors will
//        // update the deadline prior to each asynchronous operation.
//        deadline_.async_wait(boost::bind(&client::check_deadline, this));
//    }
//
//    // This function terminates all the actors to shut down the connection. It
//    // may be called by the user of the client class, or by the class itself in
//    // response to graceful termination or an unrecoverable error.
//    void stop()
//    {
//        stopped_ = true;
//        boost::system::error_code ignored_ec;
//        socket_.close(ignored_ec);
//        deadline_.cancel();
//        heartbeat_timer_.cancel();
//    }
//
//private:
//    void start_connect(tcp::resolver::results_type::iterator endpoint_iter)
//    {
//        if (endpoint_iter != endpoints_.end())
//        {
//            std::cout << "Trying " << endpoint_iter->endpoint() << "...\n";
//
//            // Set a deadline for the connect operation.
//            deadline_.expires_after(boost::asio::chrono::seconds(60));
//
//            // Start the asynchronous connect operation.
//            socket_.async_connect(endpoint_iter->endpoint(),
//                boost::bind(&client::handle_connect, this,
//                    boost::placeholders::_1, endpoint_iter));
//        }
//        else
//        {
//            // There are no more endpoints to try. Shut down the client.
//            stop();
//        }
//    }
//
//    void handle_connect(const boost::system::error_code& ec,
//        tcp::resolver::results_type::iterator endpoint_iter)
//    {
//        if (stopped_)
//            return;
//
//        // The async_connect() function automatically opens the socket at the start
//        // of the asynchronous operation. If the socket is closed at this time then
//        // the timeout handler must have run first.
//        if (!socket_.is_open())
//        {
//            std::cout << "Connect timed out\n";
//
//            // Try the next available endpoint.
//            start_connect(++endpoint_iter);
//        }
//
//        // Check if the connect operation failed before the deadline expired.
//        else if (ec)
//        {
//            std::cout << "Connect error: " << ec.message() << "\n";
//
//            // We need to close the socket used in the previous connection attempt
//            // before starting a new one.
//            socket_.close();
//
//            // Try the next available endpoint.
//            start_connect(++endpoint_iter);
//        }
//
//        // Otherwise we have successfully established a connection.
//        else
//        {
//            std::cout << "Connected to " << endpoint_iter->endpoint() << "\n";
//
//            // Start the input actor.
//            start_read();
//
//            // Start the heartbeat actor.
//            start_write();
//        }
//    }
//
//    void start_read()
//    {
//        // Set a deadline for the read operation.
//        deadline_.expires_after(boost::asio::chrono::seconds(30));
//
//        // Start an asynchronous operation to read a newline-delimited message.
//        boost::asio::async_read_until(socket_,
//            boost::asio::dynamic_buffer(input_buffer_), '\n',
//            boost::bind(&client::handle_read, this,
//                boost::placeholders::_1, boost::placeholders::_2));
//    }
//
//    void handle_read(const boost::system::error_code& ec, std::size_t n)
//    {
//        if (stopped_)
//            return;
//
//        if (!ec)
//        {
//            // Extract the newline-delimited message from the buffer.
//            std::string line(input_buffer_.substr(0, n - 1));
//            input_buffer_.erase(0, n);
//
//            // Empty messages are heartbeats and so ignored.
//            if (!line.empty())
//            {
//                std::cout << "Received: " << line << "\n";
//            }
//
//            start_read();
//        }
//        else
//        {
//            std::cout << "Error on receive: " << ec.message() << "\n";
//
//            stop();
//        }
//    }
//
//    void start_write()
//    {
//        if (stopped_)
//            return;
//
//        // Start an asynchronous operation to send a heartbeat message.
//        boost::asio::async_write(socket_, boost::asio::buffer("\n", 1),
//            boost::bind(&client::handle_write, this, boost::placeholders::_1));
//    }
//
//    void handle_write(const boost::system::error_code& ec)
//    {
//        if (stopped_)
//            return;
//
//        if (!ec)
//        {
//            // Wait 10 seconds before sending the next heartbeat.
//            heartbeat_timer_.expires_after(boost::asio::chrono::seconds(10));
//            heartbeat_timer_.async_wait(boost::bind(&client::start_write, this));
//        }
//        else
//        {
//            std::cout << "Error on heartbeat: " << ec.message() << "\n";
//
//            stop();
//        }
//    }
//
//    void check_deadline()
//    {
//        if (stopped_)
//            return;
//
//        // Check whether the deadline has passed. We compare the deadline against
//        // the current time since a new asynchronous operation may have moved the
//        // deadline before this actor had a chance to run.
//        if (deadline_.expiry() <= steady_timer::clock_type::now())
//        {
//            // The deadline has passed. The socket is closed so that any outstanding
//            // asynchronous operations are cancelled.
//            socket_.close();
//
//            // There is no longer an active deadline. The expiry is set to the
//            // maximum time point so that the actor takes no action until a new
//            // deadline is set.
//            deadline_.expires_at(steady_timer::time_point::max());
//        }
//
//        // Put the actor back to sleep.
//        deadline_.async_wait(boost::bind(&client::check_deadline, this));
//    }
//
//private:
//    tcp::resolver::results_type endpoints_;
//    tcp::socket socket_;
//    std::string input_buffer_;
//    steady_timer deadline_;
//    steady_timer heartbeat_timer_;
//};