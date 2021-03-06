#ifndef PEER_H
#define PEER_H

#include <map>
#include <string>
#include <memory>
#include <boost/asio.hpp>
#include <boost/signals2.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/bimap.hpp>
#include <chrono>

#include "Torrent.h"
#include "ValueTypes.h"

namespace Bittorrent
{
    using boost::asio::ip::tcp;

    class Peer
        : public std::enable_shared_from_this<Peer>
    {
    public:
        struct dataPackage
        {
            Peer* sourcePeer;
            int piece;
            int block;
            std::vector<byte> data;

            bool operator==(const dataPackage& p) const {
                return this->sourcePeer->endpointKey ==
                        p.sourcePeer->endpointKey &&
                        this->piece == p.piece && this->block == p.block &&
                        this->data == p.data;
            }

            bool operator!=(const dataPackage& p) const {
                return !(*this == p);
            }
        };

        struct dataRequest
        {
            Peer* sourcePeer;
            int piece;
            int offset;
            int dataSize;
            bool isCancelled;

            bool operator==(const dataRequest& r) const {
                return this->sourcePeer->endpointKey ==
                        r.sourcePeer->endpointKey &&
                        this->piece == r.piece && this->offset == r.offset &&
                        this->dataSize == r.dataSize;
            }

            bool operator!=(const dataRequest& r) const {
                return !(*this == r);
            }
        };

        //signals
        std::shared_ptr<boost::signals2::signal<void(Peer*)>> sig_disconnected;

        std::shared_ptr<boost::signals2::signal<void(Peer*)>> sig_stateChanged;

        std::shared_ptr<boost::signals2::signal<void(
                dataRequest)>> sig_blockRequested;

        std::shared_ptr<boost::signals2::signal<void(
                dataRequest)>> sig_blockCancelled;

        std::shared_ptr<boost::signals2::signal<void(
                dataPackage)>> sig_blockReceived;

        //general variables
        std::string peerHost;
        std::string peerPort;
        boost::bimap<std::string, int> messageType;

        std::vector<byte> localID;
        std::string peerID;

        std::shared_ptr<Torrent> torrent;

        tcp::endpoint endpointKey;

        //how many pieces the peer has downloaded
        std::vector<bool> isPieceDownloaded;

        //status info
        std::chrono::high_resolution_clock::time_point disconnectTime;
        bool isDisconnected;
        bool isHandshakeSent;
        bool isBitfieldSent;
        bool isChokeSent;
        bool isInterestedSent;
        bool isHandshakeReceived;
        bool isChokeReceived;
        bool isInterestedReceived;
        std::vector<std::vector<bool>> isBlockRequested;

        std::chrono::high_resolution_clock::time_point lastActive;
        std::chrono::high_resolution_clock::time_point lastKeepAlive;

        long long uploaded;
        long long downloaded;

        //asio context
        //boost::asio::io_context& context;

        //delete default constructor
        Peer() = delete;
        //client-opened connection constructors
        Peer(std::shared_ptr<Torrent> torrent, std::vector<byte>& localID,
            boost::asio::io_context& io_context, int tcpPort);
        //peer-opened connection constructor
        //need io_context here to initialise timers
        Peer(std::vector<std::shared_ptr<Torrent>>* torrentList,
             std::vector<byte>& localID, boost::asio::io_context& io_context,
             int tcpPort);

        std::shared_ptr<Peer> getPtr();

        //status functions
        std::string piecesDownloaded();
        int piecesRequiredAvailable();
        int piecesDownloadedCount();
        bool isCompleted();
        int blocksRequested();
        void disconnect();

        //sending
        void sendKeepAlive();
        void sendInterested();
        void sendNotInterested();
        void sendUnchoke();
        void sendHave(int index);
        void sendDataRequest(int index, int offset, int dataSize);
        void sendCancel(int index, int offset, int dataSize);
        void sendPiece(int index, int offset, std::vector<byte> data);


        //ASIO
        boost::asio::ip::tcp::socket& socket();
        //restart connection with peer
        void restartResolve(std::shared_ptr<tcp::resolver> presolver);
        void handleRestartResolve(const boost::system::error_code& ec,
                                 const tcp::resolver::results_type results);

        //start connection to resolved peer endpoints
        void connectToPeer(const boost::system::error_code& ec,
                              tcp::resolver::results_type results);

        void readFromAcceptedPeer();
        void startNewRead();
        void setSocketOptions(int tcpPort);

    private:
        // Strand to ensure the connection's handlers are not called concurrently.
        boost::asio::io_context::strand strand_;
        //tcp data
        tcp::socket socket_;

        //TCP transmission buffers
        std::vector<byte> processBuffer;
        std::vector<byte> recBuffer;

        //ptr to working torrent list so accepted peer connection can
        //associate handshake infohash with torrent from the list
        std::vector<std::shared_ptr<Torrent>>* ptr_torrentList;

        //established connection functions - maybe separate class?
        bool isAccepted;	//flag to use async funcs with shared_ptr

        //new connection methods
        void handleNewConnect(const boost::system::error_code& ec,
            const tcp::endpoint& endpoint);
        void handleRead(const boost::system::error_code& ec,
            std::size_t receivedBytes);
        int getMessageLength();
        void sendNewBytes(std::vector<byte> sendBuffer);
        void handleNewSend(const boost::system::error_code& ec,
            std::size_t receivedBytes, std::shared_ptr<std::vector<byte>> message);

        //decoding
        bool decodeHandshake(std::vector<byte>& hash, std::string& id);
        bool decodeKeepAlive();
        bool decodeChoke();
        bool decodeUnchoke();
        bool decodeInterested();
        bool decodeNotInterested();
        bool decodeState(int typeVal);
        bool decodeHave(int& index);
        bool decodeBitfield(int pieces,
            std::vector<bool>& recIsPieceDownloaded);
        bool decodeDataRequest(int& index, int& offset, int& dataSize);
        bool decodeCancel(int& index, int& offset, int& dataSize);
        bool decodePiece(int& index, int& offset, std::vector<byte>& data);


        //encoding
        std::vector<byte> encodeHandshake(std::vector<byte>& hash,
            std::vector<byte>& id);
        std::vector<byte> encodeKeepAlive();
        std::vector<byte> encodeChoke();
        std::vector<byte> encodeUnchoke();
        std::vector<byte> encodeInterested();
        std::vector<byte> encodeNotInterested();
        std::vector<byte> encodeState(int typeVal);
        std::vector<byte> encodeHave(int index);
        std::vector<byte> encodeBitfield(
            std::vector<bool> isPieceVerified);
        std::vector<byte> encodeDataRequest(int index, int offset,
            int dataSize);
        std::vector<byte> encodeCancel(int index, int offset, int dataSize);
        std::vector<byte> encodePiece(int index, int offset,
            std::vector<byte> data);

        //sending
        void sendHandShake();
        void sendChoke();
        void sendBitfield(std::vector<bool> isPieceVerified);

        //receiving
        void handleMessage();
        int getMessageType(std::vector<byte> data);
        void handleHandshake(std::vector<byte> hash, std::string id);
        void handleKeepAlive();
        void handleChoke();
        void handleUnchoke();
        void handleInterested();
        void handleNotInterested();
        void handleHave(int index);
        void handleBitfield(std::vector<bool> recIsPieceDownloaded);
        void handleDataRequest(int index, int offset, int dataSize);
        void handleCancel(int index, int offset, int dataSize);
        void handlePiece(int index, int offset, std::vector<byte> data);
        //no DHT port stuff for now
        //void handlePort(int port);
    };
}


#endif // PEER_H
