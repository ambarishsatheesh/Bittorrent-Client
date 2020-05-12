#include "Client.h"

namespace Bittorrent
{
	Client::Client()
	{

	}

	void Client::handlePeerListUpdated(std::vector<peer> peerList)
	{
        //TCP setup
        boost::asio::io_context io_context;
        tcp::resolver resolver(io_context);

        //new peer instance using peer data from vector
        for (auto peer : peerList)
        {
            // Look up the domain name
            tcp::resolver::results_type results = resolver.resolve(
                peer.ipAddress, peer.port);

            //create new peer instance
            Peer peer();

        }
	}
}