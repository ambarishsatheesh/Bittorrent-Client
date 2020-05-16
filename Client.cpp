#include "Client.h"

namespace Bittorrent
{
	Client::Client()
        : port{ 0 }, localID{""}, acc_io_context(), 
        acceptor(acc_io_context, tcp::endpoint(tcp::v4(), port))
	{
        //generate 20 byte client ID
        std::random_device dev;
        std::mt19937 rng(dev());
        const std::uniform_int_distribution<int32_t>
            dist6(0, std::numeric_limits<int32_t>::max());

        localID.push_back('-');
        localID.push_back('A');
        localID.push_back('S');
        localID.push_back('0');
        localID.push_back('5');
        localID.push_back('0');
        localID.push_back('0');
        localID.push_back('-');

        for (size_t i = 8; i < 20; ++i)
        {
            localID.push_back((dist6(rng) >> 8) & 0xff);
        }

	}

	void Client::handlePeerListUpdated(std::vector<peer> peerList)
	{
        //TCP setup
        boost::asio::io_context io_context;
        tcp::resolver resolver(io_context);

        //new peer instance using peer data from vector
        for (auto peer : peerList)
        {
            //check if peer already in (thread-safe?) Peer list 
            //(client class member, not this peerList function parameter)
            //if (!tryAddtoDict)
            //{
                // Look up the domain name
                tcp::resolver::results_type results = resolver.resolve(
                    peer.ipAddress, peer.port);

                //create new peer instance
                Peer peer(torrent, localID, io_context, results);

                //can be shared among threads if needed - not a good idea until
                //everything is thread safe
                io_context.run();
            //}
        }
	}

    void Client::enablePeerConnections()
    {
        doAccept();

       acc_io_context.run();
    }

    void Client::doAccept()
    {
        acceptor.async_accept(
            [this](boost::system::error_code ec, tcp::socket socket)
            {
                if (!ec)
                {
                    //if (!tryAddtoDict socket.endpoint)
                    //{
                        std::make_shared<Peer>(torrent, localID, acc_io_context,
                            std::move(socket));
                    //}
                    //else
                    //{
                        //socket.cancel();
                    //}
                        
                }
                doAccept();
            });
    }
}