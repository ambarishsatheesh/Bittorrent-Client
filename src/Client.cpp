#include "Client.h"
#include "loguru.h"
#include <random>

namespace Bittorrent
{
    Client::Client()
        : port{ 0 }, localID(), acc_io_context(), 
        acceptor(acc_io_context, tcp::endpoint(tcp::v4(), port))
	{
        LOG_SCOPE_F(INFO, "ClientID");
        LOG_F(INFO, "Generating new Client ID...");

        //generate 20 byte client ID
        std::random_device dev;
        std::mt19937 rng(dev());
        std::uniform_int_distribution<int32_t>
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

        LOG_F(INFO, "Client ID generated!");

        auto clientLog = toHex(localID);
        LOG_F(INFO, "Client ID (hex): %s", clientLog.c_str());

        WorkingTorrents.clientID = localID;
	}

	void Client::handlePeerListUpdated(std::vector<peer> peerList)
	{
        //TCP setup
        boost::asio::io_context io_context;

        //new peer instance using peer data from vector
        for (auto peer : peerList)
        {
            //check if peer already in (thread-safe?) Peer dict 
            //(client class member, not this peerList function parameter)
            //if (!tryAddtoDict)
            //{
                //create new peer instance
                auto peerConn = 
                    std::make_shared<Peer>(WorkingTorrents.torrentList[0], localID, io_context);   //temp vector element used, needs to be corrected later

                peerConn->startNew(peer.ipAddress, peer.port);

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
        //which torrent??? need to specify in creation of Client class
        acceptor.async_accept(
            [this](boost::system::error_code ec, tcp::socket socket)
            {
                if (!ec)
                {
                    //if (!tryAddtoDict socket.endpoint)
                    //{
                        std::make_shared<Peer>(WorkingTorrents.torrentList[0], localID, acc_io_context,
                            std::move(socket));   //temp vector element used, needs to be corrected later
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
