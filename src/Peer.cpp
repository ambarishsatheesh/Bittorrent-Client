#include "Peer.h"
#include "sha1.h"
#include "Utility.h"

#include <iostream>
#include <boost/algorithm/string/join.hpp>
#include <boost/dynamic_bitset.hpp>
#include <bitset>

namespace Bittorrent
{
using tcp = boost::asio::ip::tcp;
using namespace utility;

Peer::Peer(std::shared_ptr<Torrent> torrent, std::vector<byte>& localID,
    boost::asio::io_context& io_context, int tcpPort)
    : sig_disconnected{std::make_shared<boost::signals2::signal<void(
                       Peer*)>>()},
    sig_stateChanged{std::make_shared<boost::signals2::signal<void(
                       Peer*)>>()},
    sig_blockRequested{std::make_shared<boost::signals2::signal<void(
                       dataRequest)>>()},
    sig_blockCancelled{std::make_shared<boost::signals2::signal<void(
                       dataRequest)>>()},
    sig_blockReceived{std::make_shared<boost::signals2::signal<void(
                           dataPackage)>>()},
    peerHost{""}, peerPort{""}, localID{ localID }, peerID{ "" },
    torrent{ torrent }, endpointKey(),
    isPieceDownloaded(torrent->piecesData.pieceCount),
    isDisconnected{}, isHandshakeSent{}, isPositionSent{},
    isChokeSent{ true }, isInterestedSent{ false }, isHandshakeReceived{},
    isChokeReceived{ true }, isInterestedReceived{ false },
    lastActive{ std::chrono::high_resolution_clock::time_point::min() },
    lastKeepAlive{ std::chrono::high_resolution_clock::time_point::min() },
    uploaded{ 0 }, downloaded{ 0 },
    strand_(io_context), socket_(io_context),
    recBuffer(68), isAccepted{false}
{
    setSocketOptions(tcpPort);

    isBlockRequested.resize(torrent->piecesData.pieceCount);
    for (size_t i = 0; i < torrent->piecesData.pieceCount; ++i)
    {
        isBlockRequested.at(i).resize(
            torrent->piecesData.getBlockCount(i));
    }

    //add message types to map
    messageType.insert({ "unknown", -3 });
    messageType.insert({ "handshake", -2 });
    messageType.insert({ "keepAlive", -1 });
    messageType.insert({ "choke", 0 });
    messageType.insert({ "unchoke", 1 });
    messageType.insert({ "interested", 2 });
    messageType.insert({ "notInterested", 3 });
    messageType.insert({ "have", 4 });
    messageType.insert({ "bitfield", 5 });
    messageType.insert({ "request", 6 });
    messageType.insert({ "piece", 7 });
    messageType.insert({ "cancel", 8 });
    messageType.insert({ "port", 9 });
}

//construct peer from accepted connection started by another peer
Peer::Peer(std::vector<std::shared_ptr<Torrent>>* torrentList,
           std::vector<byte>& localID, boost::asio::io_context& io_context,
           int tcpPort)
    : sig_disconnected{std::make_shared<boost::signals2::signal<void(
                       Peer*)>>()},
    sig_stateChanged{std::make_shared<boost::signals2::signal<void(
                           Peer*)>>()},
    sig_blockRequested{std::make_shared<boost::signals2::signal<void(
                           dataRequest)>>()},
    sig_blockCancelled{std::make_shared<boost::signals2::signal<void(
                       dataRequest)>>()},
    sig_blockReceived{std::make_shared<boost::signals2::signal<void(
                       dataPackage)>>()},
    peerHost{""}, peerPort{""}, localID{ localID }, peerID{ "" },
    torrent{ std::make_shared<Torrent>() }, endpointKey(),
    isPieceDownloaded(torrent->piecesData.pieceCount),
    isDisconnected{}, isHandshakeSent{}, isPositionSent{},
    isChokeSent{ true }, isInterestedSent{ false }, isHandshakeReceived{},
    isChokeReceived{ true }, isInterestedReceived{ false },
    lastActive{ std::chrono::high_resolution_clock::time_point::min() },
    lastKeepAlive{ std::chrono::high_resolution_clock::time_point::min() },
    uploaded{ 0 }, downloaded{ 0 },
    strand_(io_context), socket_(io_context), recBuffer(68), isAccepted{true},
    ptr_torrentList{torrentList}
{
    setSocketOptions(tcpPort);

    isBlockRequested.resize(torrent->piecesData.pieceCount);
    for (size_t i = 0; i < torrent->piecesData.pieceCount; ++i)
    {
        isBlockRequested.at(i).resize(
            torrent->piecesData.getBlockCount(i));
    }

    //add message types to map
    messageType.insert({ "unknown", -3 });
    messageType.insert({ "handshake", -2 });
    messageType.insert({ "keepAlive", -1 });
    messageType.insert({ "choke", 0 });
    messageType.insert({ "unchoke", 0 });
    messageType.insert({ "interested", 2 });
    messageType.insert({ "notInterested", 3 });
    messageType.insert({ "have", 4 });
    messageType.insert({ "bitfield", 5 });
    messageType.insert({ "request", 6 });
    messageType.insert({ "piece", 7 });
    messageType.insert({ "cancel", 8 });
    messageType.insert({ "port", 9 });
}

//also used when resuming to reopen socket and set options
void Peer::setSocketOptions(int tcpPort)
{
    try
    {
       //open socket_, allow reuse of address+port and bind
       socket_.open(tcp::v4());
       socket_.set_option(tcp::socket::reuse_address(true));
       socket_.bind(tcp::endpoint(tcp::v4(), tcpPort));
    }
    catch (const boost::system::system_error& error)
    {
       LOG_F(ERROR, "Failed to bind TCP socket_ to port %d", tcpPort);

       disconnect();
    }
}

boost::asio::ip::tcp::socket& Peer::socket()
{
    return socket_;
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
//are available from Peer
int Peer::piecesRequiredAvailable()
{
    int pos = 0;
    //custom comparator (!verified && downloaded)
    const auto lambda = [&](bool i) {return i &&
        !torrent->statusData.isPieceVerified.at(pos++); };

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
    return piecesDownloadedCount() == torrent->piecesData.pieceCount;
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

void Peer::readFromAcceptedPeer()
{
    if (peerHost.empty() || peerPort.empty())
    {
        //get endpoint info from accepted connection
        endpointKey = socket_.remote_endpoint();
        peerHost = socket_.remote_endpoint().address().to_string();
        peerHost = std::to_string(socket_.remote_endpoint().port());
    }

    socket_.async_read_some(boost::asio::buffer(recBuffer),
        boost::asio::bind_executor(strand_,
        [self = shared_from_this()](boost::system::error_code ec, std::size_t receivedBytes){
            if (!ec)
            {
                self->handleRead(ec, receivedBytes);
            }
            else
            {
                LOG_F(ERROR, "(%s:%s) Error on receive: %s.",
                      self->peerHost.c_str(), self->peerPort.c_str(),
                      ec.message().c_str());

                self->disconnect();
            }
        }));
}

void Peer::connectToNewPeer(boost::system::error_code const& ec,
    std::shared_ptr<tcp::resolver> presolver, tcp::resolver::iterator iter)
{
    if (ec)
    {
        LOG_F(ERROR, "(%s:%s) Error resolving peer: %s.",
              peerHost.c_str(), peerPort.c_str(), ec.message().c_str());

        disconnect();
    }
    else
    {
        boost::asio::async_connect(socket_, iter,
            boost::asio::bind_executor(strand_,
            [self = shared_from_this(), presolver]
            (const boost::system::error_code& ec,
             tcp::resolver::results_type::iterator endpointItr)
            {
                self->handleNewConnect(ec, endpointItr);
            }));
    }
}

void Peer::handleNewConnect(const boost::system::error_code& ec,
    tcp::resolver::results_type::iterator endpointItr)
{
    if (!ec)
    {
        LOG_F(INFO, "(%s:%s) Connected.", peerHost.c_str(), peerPort.c_str());

        endpointKey = endpointItr->endpoint();

        startNewRead();
        sendHandShake();
    }
    // else connection successful
    else
    {
        LOG_F(ERROR, "(%s:%s) Connect error: %s.",
              peerHost.c_str(), peerPort.c_str(), ec.message().c_str());

        disconnect();
    }
}

void Peer::startNewRead()
{
    boost::asio::async_read(socket_, boost::asio::buffer(recBuffer),
        boost::asio::bind_executor(strand_,
        [self = shared_from_this()]
        (const boost::system::error_code& ec, std::size_t receivedBytes)
        {
            self->handleRead(ec, receivedBytes);
        }));
}

void Peer::handleRead(const boost::system::error_code& ec,
    std::size_t receivedBytes)
{
    if (isDisconnected)
    {
        return;
    }

    if (!ec)
    {
        //handshake message
        if (!isHandshakeReceived)
        {
            //process handshake
            processBuffer = recBuffer;
            handleMessage();

            //clear and resize buffer to receive new header packet
            recBuffer.clear();
            recBuffer.resize(4);

            //differentiate between accepted connected and initiated connection
            if (isAccepted)
            {
                readFromAcceptedPeer();
            }
            else
            {
                startNewRead();
            }
        }
        //if header, copy data, clear recBuffer and wait for entire message
        //use header data to find remaining message length
        else if (receivedBytes == 4)
        {
            LOG_F(INFO, "(%s:%s) Received message header.",
                  peerHost.c_str(), peerPort.c_str());

            processBuffer = recBuffer;
            int messageLength = getMessageLength();

            //4 byte header packet with value 0 is a keep alive message
            if (messageLength == 0)
            {
                //clear and resize buffer to receive new header packet
                recBuffer.clear();
                recBuffer.resize(4);

                if (isAccepted)
                {
                    readFromAcceptedPeer();
                }
                else
                {
                    startNewRead();
                }

                //process keep alive message
                handleMessage();
            }
            //clear and resize buffer to receive rest of the message
            else
            {
                recBuffer.clear();
                recBuffer.resize(messageLength);

                if (isAccepted)
                {
                    readFromAcceptedPeer();
                }
                else
                {
                    startNewRead();
                }
            }
        }
        //rest of the message
        else
        {
            LOG_F(INFO, "(%s:%s) Received message body.",
                  peerHost.c_str(), peerPort.c_str());

            //insert rest of message after stored header
            processBuffer.insert(processBuffer.begin() + 4,
                recBuffer.begin(), recBuffer.end());

            //process complete message
            handleMessage();

            //clear and resize buffer to receive new header packet
            recBuffer.clear();
            recBuffer.resize(4);

            if (isAccepted)
            {
                readFromAcceptedPeer();
            }
            else
            {
                startNewRead();
            }
        }
    }
    else
    {
        LOG_F(ERROR, "(%s:%s) Error on receive: %s.",
              peerHost.c_str(), peerPort.c_str(), ec.message().c_str());

        disconnect();
    }
}

void Peer::acc_sendNewBytes(std::vector<byte> sendBuffer)
{
    if (isDisconnected)
    {
        return;
    }

    boost::asio::async_write(socket_, boost::asio::buffer(sendBuffer),
        boost::asio::bind_executor(strand_,
        [self = shared_from_this()]
        (boost::system::error_code ec, std::size_t sentBytes)
        {
            self->handleNewSend(ec, sentBytes);
        }));
}

void Peer::sendNewBytes(std::vector<byte> sendBuffer)
{
    if (isDisconnected)
    {
        return;
    }

    //capture the payload so it continues to exist during async send
    auto payload = std::make_shared<std::vector<byte>>(sendBuffer);

    // start async send and immediately call the handler
    boost::asio::async_write(socket_, boost::asio::buffer(*payload),
        boost::asio::bind_executor(strand_,
        [self = shared_from_this(), payload]
        (const boost::system::error_code& ec, std::size_t sentBytes)
        {
            self->handleNewSend(ec, sentBytes);
        }));
}

void Peer::handleNewSend(const boost::system::error_code& ec,
    std::size_t sentBytes)
{
    if (isDisconnected)
    {
        return;
    }

    if (!ec)
    {
        LOG_F(INFO, "(%s:%s) Sent %zd bytes.",
              peerHost.c_str(), peerPort.c_str(),
              sentBytes);
    }
    else
    {
        LOG_F(ERROR, "(%s:%s) Error on send: %s.",
              peerHost.c_str(), peerPort.c_str(),
              ec.message().c_str());

        disconnect();
    }
}

void Peer::disconnect()
{
    isDisconnected = true;
    boost::system::error_code ignored_ec;
    socket_.close(ignored_ec);

    LOG_F(INFO, "(%s:%s) Disconnecting from peer; "
                "Downloaded %lld, uploaded %lld.",
          peerHost.c_str(), peerPort.c_str(),
          downloaded, uploaded);

    //call slot
    sig_disconnected->operator()(this);
}


int Peer::getMessageLength()
{
    //header packet (4 bytes) gives message length
    //convert bytes to int
    int length = 0;
    for (size_t i = 0; i < 4; ++i)
    {
        length <<= 8;
        length |= recBuffer.at(i);
    }
    return length;
}

void Peer::handleMessage()
{
    LOG_F(INFO, "(%s:%s) Handling message...",
          peerHost.c_str(), peerPort.c_str());

    //update clock
    lastActive = std::chrono::high_resolution_clock::now();

    int deducedtype = getMessageType(processBuffer);

    LOG_F(INFO, "deducedtype: %d", deducedtype);

    if (deducedtype == messageType.left.at("unknown"))
    {
        return;
    }
    else if (deducedtype == messageType.left.at("handshake"))
    {
        LOG_F(INFO, "(%s:%s) Received 'handshake' message.",
              peerHost.c_str(), peerPort.c_str());

        std::vector<byte> hash;
        std::string id;
        if (decodeHandshake(hash, id))
        {
            handleHandshake(hash, id);
            return;
        }
    }
    else if (deducedtype == messageType.left.at("keepAlive")
        && decodeKeepAlive())
    {
        LOG_F(INFO, "(%s:%s) Received 'keep-alive' message.",
              peerHost.c_str(), peerPort.c_str());

        handleKeepAlive();
        return;
    }
    else if (deducedtype == messageType.left.at("choke")
        && decodeChoke())
    {
        LOG_F(INFO, "(%s:%s) Received 'choke' message.",
              peerHost.c_str(), peerPort.c_str());

        handleChoke();
        return;
    }
    else if (deducedtype == messageType.left.at("unchoke")
        && decodeUnchoke())
    {
        LOG_F(INFO, "(%s:%s) Received 'unchoke' message.",
              peerHost.c_str(), peerPort.c_str());

        handleUnchoke();
        return;
    }
    else if (deducedtype == messageType.left.at("interested")
        && decodeInterested())
    {
        LOG_F(INFO, "(%s:%s) Received 'interested' message.",
              peerHost.c_str(), peerPort.c_str());

        handleInterested();
        return;
    }
    else if (deducedtype == messageType.left.at("notInterested")
        && decodeNotInterested())
    {
        LOG_F(INFO, "(%s:%s) Received 'not interested' message.",
              peerHost.c_str(), peerPort.c_str());

        handleNotInterested();
        return;
    }
    else if (deducedtype == messageType.left.at("have"))
    {
        LOG_F(INFO, "(%s:%s) Received 'have' message.",
              peerHost.c_str(), peerPort.c_str());

        int index;
        if (decodeHave(index))
        {
            handleHave(index);
            return;
        }
    }
    else if (deducedtype == messageType.left.at("bitfield"))
    {
        std::vector<bool> recIsPieceDownloaded;
        if (decodeBitfield(isPieceDownloaded.size(), recIsPieceDownloaded))
        {
            LOG_F(INFO, "(%s:%s) Received 'bitfield' message.",
                  peerHost.c_str(), peerPort.c_str());

            handleBitfield(recIsPieceDownloaded);
            return;
        }
    }
    else if (deducedtype == messageType.left.at("request"))
    {
        LOG_F(INFO, "(%s:%s) Received 'request' message.",
              peerHost.c_str(), peerPort.c_str());

        int index;
        int offset;
        int dataSize;
        if (decodeDataRequest(index, offset, dataSize))
        {
            handleDataRequest(index, offset, dataSize);
            return;
        }
    }
    else if (deducedtype == messageType.left.at("cancel"))
    {
        LOG_F(INFO, "(%s:%s) Received 'cancel' message.",
              peerHost.c_str(), peerPort.c_str());

        int index;
        int offset;
        int dataSize;
        if (decodeCancel(index, offset, dataSize))
        {
            handleCancel(index, offset, dataSize);
            return;
        }
    }
    else if (deducedtype == messageType.left.at("piece"))
    {
        LOG_F(INFO, "(%s:%s) Received 'piece' message.",
              peerHost.c_str(), peerPort.c_str());

        int index;
        int offset;
        std::vector<byte> data;
        if (decodePiece(index, offset, data))
        {
            handlePiece(index, offset, data);
            return;
        }
    }
    //this is the listen port for peer's DHT node
    else if (deducedtype == messageType.left.at("port"))
    {
        return;
    }

    //if unhandled message, disconnect from peer
    LOG_F(ERROR, "(%s:%s) Received an unhandled message: %s.",
          peerHost.c_str(), peerPort.c_str(), toHex(processBuffer).c_str());

    disconnect();
}

bool Peer::decodeHandshake(std::vector<byte>& hash, std::string& id)
{
    hash.resize(20);
    id = "";

    if (processBuffer.size() != 68 || processBuffer.at(0) != 19)
    {

        LOG_F(ERROR, "(%s:%s) Invalid handshake! Must be 68 bytes long and the "
                     "byte value must equal 19.",
              peerHost.c_str(), peerPort.c_str());

        LOG_F(ERROR, "(%s:%s) Handshake size: %zd, length value: %hhu",
              peerHost.c_str(), peerPort.c_str(),
              processBuffer.size(), processBuffer.at(0));

        return false;
    }

    //get first 19 byte string (UTF-8) after length byte
    std::string protocolStr(processBuffer.begin() + 1,
        processBuffer.begin() + 20);

    if (protocolStr != "BitTorrent protocol")
    {
        LOG_F(ERROR, "(%s:%s) Invalid handshake! Protocol value must equal "
                     "'Bittorrent protocol'. Received: %s",
              peerHost.c_str(), peerPort.c_str(), protocolStr.c_str());

        return false;
    }

    //byte 21-28 represent flags (all 0) - can be ignored
    hash = { processBuffer.begin() + 28, processBuffer.begin() + 48 };
    id = { processBuffer.begin() + 48, processBuffer.end()};

    LOG_F(INFO, "(%s:%s) Successfully decoded 'handshake' message.",
          peerHost.c_str(), peerPort.c_str());

    return true;
}

bool Peer::decodeKeepAlive()
{
    //convert bytes to int
    int lengthVal = 0;

    for (size_t i = 0; i < 4; ++i)
    {
        lengthVal <<= 8;
        lengthVal |= processBuffer.at(i);
    }

    if (processBuffer.size() != 4 || lengthVal != 0)
    {
        LOG_F(ERROR, "(%s:%s) Invalid 'keep-alive' message!",
              peerHost.c_str(), peerPort.c_str());

        return false;
    }

    LOG_F(INFO, "(%s:%s) Successfully decoded 'keep-alive' message.",
          peerHost.c_str(), peerPort.c_str());

    return true;
}

bool Peer::decodeChoke()
{
    return decodeState(messageType.left.at("choke"));
}

bool Peer::decodeUnchoke()
{
    return decodeState(messageType.left.at("unchoke"));
}

bool Peer::decodeInterested()
{
    return decodeState(messageType.left.at("interested"));
}

bool Peer::decodeNotInterested()
{
    return decodeState(messageType.left.at("notInterested"));
}

//all state messages consist of 4 byte header with value 1,
//and then a 1 byte type value
bool Peer::decodeState(int typeVal)
{
    //convert bytes to int
    int lengthVal = 0;

    for (size_t i = 0; i < 4; ++i)
    {
        lengthVal <<= 8;
        lengthVal |= processBuffer.at(i);
    }

    if (processBuffer.size() != 5 || lengthVal != 1 ||
        processBuffer.at(4) != static_cast<byte>(typeVal))
    {
        LOG_F(ERROR, "(%s:%s) Invalid 'state' message of type %s.",
              peerHost.c_str(), peerPort.c_str(),
              messageType.right.at(typeVal).c_str());

        return false;
    }

    LOG_F(INFO, "(%s:%s) Successfully decoded 'state' message of type %s.",
          peerHost.c_str(), peerPort.c_str(),
          messageType.right.at(typeVal).c_str());

    return true;
}

bool Peer::decodeHave(int& index)
{
    //convert bytes to int
    int lengthVal = 0;

    for (size_t i = 0; i < 4; ++i)
    {
        lengthVal <<= 8;
        lengthVal |= processBuffer.at(i);
    }

    if (processBuffer.size() != 9 || lengthVal != 5)
    {
        LOG_F(ERROR, "(%s:%s) Invalid 'have' message!",
              peerHost.c_str(), peerPort.c_str());

        return false;
    }

    //get index from last 4 bytes
    index = 0;
    for (size_t i = 5; i < 9; ++i)
    {
        index <<= 8;
        index |= processBuffer.at(i);
    }

    LOG_F(INFO, "(%s:%s) Successfully decoded 'have' message.",
          peerHost.c_str(), peerPort.c_str());

    return index;
}

bool Peer::decodeBitfield(int pieces,
    std::vector<bool>& recIsPieceDownloaded)
{
    recIsPieceDownloaded.reserve(pieces);

    //get length of data after header packet
    //if number of pieces is not divisible by 8, the end of the bitfield is
    //padded with 0s
    const int expectedLength = std::ceil(pieces / 8.0) + 1;

    //convert first 4 bytes to int
    int lengthVal = 0;

    for (size_t i = 0; i < 4; ++i)
    {
        lengthVal <<= 8;
        lengthVal |= processBuffer.at(i);
    }

    if (processBuffer.size() != expectedLength + 4 ||
        lengthVal != expectedLength)
    {
        LOG_F(ERROR, "(%s:%s) Invalid 'bitfield' message! "
                     "Expected length: %d. Received length: %d",
              peerHost.c_str(), peerPort.c_str(), expectedLength, lengthVal);

        return false;
    }

    LOG_F(INFO, "(%s:%s) Decoded step A.",
          peerHost.c_str(), peerPort.c_str());

    std::string binaryAsString = "";
    for (size_t i = 5; i < processBuffer.size(); ++i)
    {
        std::bitset<8> bit(processBuffer.at(i));
        binaryAsString += bit.to_string();
    }

    LOG_F(INFO, "(%s:%s) Decoded step B.",
          peerHost.c_str(), peerPort.c_str());

    for (int i = 0; i < pieces; ++i)
    {
        recIsPieceDownloaded.push_back(binaryAsString.at(i) == '1');
    }

    LOG_F(INFO, "(%s:%s) Successfully decoded 'bitfield' message.",
          peerHost.c_str(), peerPort.c_str());

    return true;
}

bool Peer::decodeDataRequest(int& index, int& offset, int& dataSize)
{
    //convert first 4 bytes to int
    int lengthVal = 0;

    for (size_t i = 0; i < 4; ++i)
    {
        lengthVal <<= 8;
        lengthVal |= processBuffer.at(i);
    }

    if (processBuffer.size() != 17 || lengthVal != 13)
    {
        LOG_F(ERROR, "(%s:%s) Invalid 'request' message!",
              peerHost.c_str(), peerPort.c_str());

        return false;
    }

    //get index val
    index = 0;
    for (size_t i = 5; i < 9; ++i)
    {
        index <<= 8;
        index |= processBuffer.at(i);
    }

    //get byte offset val
    offset = 0;
    for (size_t i = 9; i < 13; ++i)
    {
        offset <<= 8;
        offset |= processBuffer.at(i);
    }

    //get data length (number of bytes to send) val
    dataSize = 0;
    for (size_t i = 13; i < 17; ++i)
    {
        dataSize <<= 8;
        dataSize |= processBuffer.at(i);
    }

    LOG_F(INFO, "(%s:%s) Successfully decoded 'request' message.",
          peerHost.c_str(), peerPort.c_str());

    return true;
}

bool Peer::decodeCancel(int& index, int& offset, int& dataSize)
{
    //convert first 4 bytes to int
    int lengthVal = 0;

    for (size_t i = 0; i < 4; ++i)
    {
        lengthVal <<= 8;
        lengthVal |= processBuffer.at(i);
    }

    if (processBuffer.size() != 17 || lengthVal != 13)
    {
        LOG_F(ERROR, "(%s:%s) Invalid Cancel message!",
              peerHost.c_str(), peerPort.c_str());

        return false;
    }

    //get index val
    index = 0;
    for (size_t i = 5; i < 9; ++i)
    {
        index <<= 8;
        index |= processBuffer.at(i);
    }

    //get byte offset val
    offset = 0;
    for (size_t i = 9; i < 13; ++i)
    {
        offset <<= 8;
        offset |= processBuffer.at(i);
    }

    //get data length (number of bytes to send) val
    dataSize = 0;
    for (size_t i = 13; i < 17; ++i)
    {
        dataSize <<= 8;
        dataSize |= processBuffer.at(i);
    }

    LOG_F(INFO, "(%s:%s) Successfully decoded 'cancel' message.",
          peerHost.c_str(), peerPort.c_str());

    return true;
}

bool Peer::decodePiece(int& index, int& offset, std::vector<byte>& data)
{
    if (processBuffer.size() < 13)
    {
        LOG_F(ERROR, "(%s:%s) Invalid piece data message! "
                     "Minimum length is 13 bytes.",
              peerHost.c_str(), peerPort.c_str());

        return false;
    }

    //get index val
    index = 0;
    for (size_t i = 5; i < 9; ++i)
    {
        index <<= 8;
        index |= processBuffer.at(i);
    }

    //get byte offset val
    offset = 0;
    for (size_t i = 9; i < 13; ++i)
    {
        offset <<= 8;
        offset |= processBuffer.at(i);
    }

    //convert first 4 bytes to int
    int lengthVal = 0;

    for (size_t i = 0; i < 4; ++i)
    {
        lengthVal <<= 8;
        lengthVal |= processBuffer.at(i);
    }

    //ignore bytes before piece data
    lengthVal -= 9;

    //get piece data
    data.resize(lengthVal);
    std::copy(processBuffer.begin() + 13, processBuffer.end(),
        data.begin());

    LOG_F(INFO, "(%s:%s) Successfully decoded piece data message.",
          peerHost.c_str(), peerPort.c_str());

    return true;
}

std::vector<byte> Peer::encodeHandshake(std::vector<byte>& hash,
    std::vector<byte>& id)
{

    std::vector<byte> message(68);
    std::string protocolStr = "BitTorrent protocol";

    message.at(0) = static_cast<byte>(19);

    auto last = std::copy(protocolStr.begin(), protocolStr.end(),
        message.begin() + 1);

    //bytes 21-28 are already 0 due to initialisation, can skip to byte 29
    last = std::copy(hash.begin(), hash.end(), message.begin() + 28);

    last = std::copy(id.begin(), id.end(),  message.begin() + 48);

    LOG_F(ERROR, "(%s:%s) infohash: %s", peerHost.c_str(), peerPort.c_str(), toHex(hash).c_str());
    LOG_F(ERROR, "(%s:%s) id: %s", peerHost.c_str(), peerPort.c_str(), toHex(id).c_str());
    LOG_F(ERROR, "(%s:%s) Sent handshake: %s", peerHost.c_str(), peerPort.c_str(), toHex(message).c_str());

    return message;
}

std::vector<byte> Peer::encodeKeepAlive()
{
    std::vector<byte> newKeepAlive(4, 0);

    return newKeepAlive;
}

std::vector<byte> Peer::encodeChoke()
{
    return encodeState(messageType.left.at("choke"));
}

std::vector<byte> Peer::encodeUnchoke()
{
    return encodeState(messageType.left.at("unchoke"));
}

std::vector<byte> Peer::encodeInterested()
{
    return encodeState(messageType.left.at("interested"));
}

std::vector<byte> Peer::encodeNotInterested()
{
    return encodeState(messageType.left.at("notInterested"));
}

std::vector<byte> Peer::encodeState(int typeVal)
{
    std::vector<byte> newMessage(5, 0);

    //4 byte length
    newMessage.at(3) = static_cast<byte>(1);

    //1 byte type value
    newMessage.at(4) = static_cast<byte>(typeVal);

    return newMessage;
}

std::vector<byte> Peer::encodeHave(int index)
{
    std::vector<byte> newHave(9, 0);

    //first 4 bytes representing length
    newHave.at(3) = static_cast<byte>(5);

    //type byte
    newHave.at(4) = static_cast<byte>(messageType.left.at("have"));

    //last 4 index bytes (big endian)
    newHave.at(5) = (index >> 24) & 0xFF;
    newHave.at(6) = (index >> 16) & 0xFF;
    newHave.at(7) = (index >> 8) & 0xFF;
    newHave.at(8) = index & 0xFF;

    return newHave;
}

std::vector<byte> Peer::encodeBitfield(
    std::vector<bool> isPieceVerified)
{
    const int numPieces = isPieceVerified.size();
    const int numBytes = std::ceil(numPieces / 8.0);
    const int numBits = numBytes * 8;
    const int length = numBytes + 1;

    std::vector<byte> newBitfield(length + 4);

    //first 4 length bytes (big endian)
    newBitfield.at(0) = (length >> 24) & 0xFF;
    newBitfield.at(1) = (length >> 16) & 0xFF;
    newBitfield.at(2) = (length >> 8) & 0xFF;
    newBitfield.at(3) = length & 0xFF;

    //type byte
    newBitfield.at(4) = static_cast<byte>(messageType.left.at("bitfield"));

    LOG_F(INFO, "(%s:%s) Encoded first four bytes of 'bitfield' message.",
          peerHost.c_str(), peerPort.c_str());

    //convert bools to binary in string representation
    std::string binaryAsString = "";
    for (int i = 0; i < numPieces; ++i)
    {
        binaryAsString += isPieceVerified.at(i) ? '1' : '0';
    }

    LOG_F(INFO, "(%s:%s) Encoded step A.",
          peerHost.c_str(), peerPort.c_str());

    //add trailing 0s (as per spec)
    const int extraBits = (numBits) - numPieces;
    for (int i = 0; i < extraBits; ++i)
    {
        binaryAsString += '0';
    }

    LOG_F(INFO, "(%s:%s) Encoded step B.",
          peerHost.c_str(), peerPort.c_str());

    //covert to byte vector
    std::vector<byte> tempBitfieldVec;
    tempBitfieldVec.reserve(numBytes);
    for (int i = 0; i < binaryAsString.size(); i+=8)
    {
        std::bitset<8> x(binaryAsString.substr(i, 8));
        tempBitfieldVec.push_back(static_cast<int>(x.to_ulong() & 0xFF));
    }

    LOG_F(INFO, "(%s:%s) Encoded step C.",
          peerHost.c_str(), peerPort.c_str());

    //copy vector to complete bitfield vector
    std::copy(tempBitfieldVec.begin(), tempBitfieldVec.end(),
        newBitfield.begin() + 5);

    LOG_F(INFO, "(%s:%s) Successfully encoded 'bitfield' message.",
          peerHost.c_str(), peerPort.c_str());

    return newBitfield;
}

std::vector<byte> Peer::encodeDataRequest(int index, int offset,
    int dataSize)
{
    std::vector<byte> newDataRequest(17);

    //length byte
    newDataRequest.at(3) = static_cast<byte>(13);

    //type byte
    newDataRequest.at(4) = static_cast<byte>(
        messageType.left.at("request"));

    //index bytes (big endian)
    newDataRequest.at(5) = (index >> 24) & 0xFF;
    newDataRequest.at(6) = (index >> 16) & 0xFF;
    newDataRequest.at(7) = (index >> 8) & 0xFF;
    newDataRequest.at(8) = index & 0xFF;

    //offset bytes (big endian)
    newDataRequest.at(9) = (offset >> 24) & 0xFF;
    newDataRequest.at(10) = (offset >> 16) & 0xFF;
    newDataRequest.at(11) = (offset >> 8) & 0xFF;
    newDataRequest.at(12) = offset & 0xFF;

    //data size bytes (big endian)
    newDataRequest.at(13) = (dataSize >> 24) & 0xFF;
    newDataRequest.at(14) = (dataSize >> 16) & 0xFF;
    newDataRequest.at(15) = (dataSize >> 8) & 0xFF;
    newDataRequest.at(16) = dataSize & 0xFF;

    return newDataRequest;
}

std::vector<byte> Peer::encodeCancel(int index, int offset, int dataSize)
{
    std::vector<byte> newCancelRequest(17);

    //length byte
    newCancelRequest.at(3) = static_cast<byte>(13);

    //type byte
    newCancelRequest.at(4) = static_cast<byte>(
        messageType.left.at("cancel"));

    //index bytes (big endian)
    newCancelRequest.at(5) = (index >> 24) & 0xFF;
    newCancelRequest.at(6) = (index >> 16) & 0xFF;
    newCancelRequest.at(7) = (index >> 8) & 0xFF;
    newCancelRequest.at(8) = index & 0xFF;

    //offset bytes (big endian)
    newCancelRequest.at(9) = (offset >> 24) & 0xFF;
    newCancelRequest.at(10) = (offset >> 16) & 0xFF;
    newCancelRequest.at(11) = (offset >> 8) & 0xFF;
    newCancelRequest.at(12) = offset & 0xFF;

    //data size bytes (big endian)
    newCancelRequest.at(13) = (dataSize >> 24) & 0xFF;
    newCancelRequest.at(14) = (dataSize >> 16) & 0xFF;
    newCancelRequest.at(15) = (dataSize >> 8) & 0xFF;
    newCancelRequest.at(16) = dataSize & 0xFF;

    return newCancelRequest;
}

std::vector<byte> Peer::encodePiece(int index, int offset,
    std::vector<byte> data)
{
    //set appropriate message size (including index, offset and length bytes)
    const auto dataSize = data.size() + 9;
    std::vector<byte> newPiece(dataSize + 4);

    //first 4 length bytes (big endian)
    newPiece.at(0) = (dataSize >> 24) & 0xFF;
    newPiece.at(1) = (dataSize >> 16) & 0xFF;
    newPiece.at(2) = (dataSize >> 8) & 0xFF;
    newPiece.at(3) = dataSize & 0xFF;

    //type byte
    newPiece.at(4) = static_cast<byte>(messageType.left.at("piece"));

    //index bytes (big endian)
    newPiece.at(5) = (index >> 24) & 0xFF;
    newPiece.at(6) = (index >> 16) & 0xFF;
    newPiece.at(7) = (index >> 8) & 0xFF;
    newPiece.at(8) = index & 0xFF;

    //offset bytes (big endian)
    newPiece.at(9) = (offset >> 24) & 0xFF;
    newPiece.at(10) = (offset >> 16) & 0xFF;
    newPiece.at(11) = (offset >> 8) & 0xFF;
    newPiece.at(12) = offset & 0xFF;

    //copy piece bytes to rest of message
    std::copy(data.begin(), data.end(), newPiece.begin() + 13);

    return newPiece;
}

void Peer::sendHandShake()
{
    if (isHandshakeSent)
    {
        return;
    }

    LOG_F(INFO, "(%s:%s) Sending 'handshake' message.",
          peerHost.c_str(), peerPort.c_str());

    //create buffer and send
    if (isAccepted)
    {
        acc_sendNewBytes(
            (encodeHandshake(torrent->hashesData.infoHash, localID)));
    }
    else
    {
        sendNewBytes(encodeHandshake(torrent->hashesData.infoHash, localID));
    }

    isHandshakeSent = true;
}

void Peer::sendKeepAlive()
{
    if (std::chrono::high_resolution_clock::now() <
            lastKeepAlive + std::chrono::seconds{30})
    {
        return;
    }

    LOG_F(INFO, "(%s:%s) Sending 'keep-alive' message.",
          peerHost.c_str(), peerPort.c_str());

    //create buffer and send
    if (isAccepted)
    {
        acc_sendNewBytes((encodeKeepAlive()));
    }
    else
    {
        sendNewBytes(encodeKeepAlive());
    }

    lastKeepAlive = std::chrono::high_resolution_clock::now();
}

void Peer::sendChoke()
{
    if (isChokeSent)
    {
        return;
    }

    LOG_F(INFO, "(%s:%s) Sending 'choke' message.",
          peerHost.c_str(), peerPort.c_str());

    //create buffer and send
    if (isAccepted)
    {
        acc_sendNewBytes((encodeChoke()));
    }
    else
    {
        sendNewBytes(encodeChoke());
    }

    isChokeSent = true;
}

void Peer::sendUnchoke()
{
    if (!isChokeSent)
    {
        return;
    }

    LOG_F(INFO, "(%s:%s) Sending 'unchoke' message.",
          peerHost.c_str(), peerPort.c_str());

    //create buffer and send
    if (isAccepted)
    {
        acc_sendNewBytes((encodeUnchoke()));
    }
    else
    {
        sendNewBytes(encodeUnchoke());
    }

    isChokeSent = false;
}

void Peer::sendInterested()
{
    if (isInterestedSent)
    {
        return;
    }

    LOG_F(INFO, "(%s:%s) Sending 'interested' message.",
          peerHost.c_str(), peerPort.c_str());

    //create buffer and send
    if (isAccepted)
    {
        acc_sendNewBytes((encodeInterested()));
    }
    else
    {
        sendNewBytes(encodeInterested());
    }

    isInterestedSent = true;
}

void Peer::sendNotInterested()
{
    if (!isInterestedSent)
    {
        return;
    }

    LOG_F(INFO, "(%s:%s) Sending 'not interested' message.",
          peerHost.c_str(), peerPort.c_str());

    //create buffer and send
    if (isAccepted)
    {
        acc_sendNewBytes((encodeNotInterested()));
    }
    else
    {
        sendNewBytes(encodeNotInterested());
    }

    isInterestedSent = false;
}

void Peer::sendHave(int index)
{
    LOG_F(INFO, "(%s:%s) Sending 'have' message.",
          peerHost.c_str(), peerPort.c_str());

    //create buffer and send
    if (isAccepted)
    {
        acc_sendNewBytes((encodeHave(index)));
    }
    else
    {
        sendNewBytes(encodeHave(index));
    }
}

void Peer::sendBitfield(std::vector<bool> isPieceVerified)
{
    //create temp vec to store string bool, then return all as single string
    std::vector<std::string> tempVec(isPieceVerified.size());
    for (auto i : isPieceVerified)
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
    std::string bitfieldStr = boost::algorithm::join(tempVec, "");

    LOG_F(INFO, "(%s:%s) Sending 'bitfield' message.",
          peerHost.c_str(), peerPort.c_str());

    //create buffer and send
    if (isAccepted)
    {
        acc_sendNewBytes((encodeBitfield(isPieceVerified)));
    }
    else
    {
        sendNewBytes(encodeBitfield(isPieceVerified));
    }
}

void Peer::sendDataRequest(int index, int offset, int dataSize)
{
    LOG_F(INFO, "(%s:%s) Sending 'request' message.",
          peerHost.c_str(), peerPort.c_str());

    //create buffer and send
    if (isAccepted)
    {
        acc_sendNewBytes((encodeDataRequest(index, offset, dataSize)));
    }
    else
    {
        sendNewBytes(encodeDataRequest(index, offset, dataSize));
    }
}

void Peer::sendCancel(int index, int offset, int dataSize)
{
    LOG_F(INFO, "(%s:%s) Sending 'cancel' message.",
          peerHost.c_str(), peerPort.c_str());

    //create buffer and send
    if (isAccepted)
    {
        acc_sendNewBytes((encodeCancel(index, offset, dataSize)));
    }
    else
    {
        sendNewBytes(encodeCancel(index, offset, dataSize));
    }
}

void Peer::sendPiece(int index, int offset, std::vector<byte> data)
{
    LOG_F(INFO, "(%s:%s) Sending piece data... Index: %d, Offset: %d, "
                "data size: %zd.",
          peerHost.c_str(), peerPort.c_str(), index, offset, data.size());


    //create buffer and send
    if (isAccepted)
    {
        acc_sendNewBytes((encodePiece(index, offset, data)));
    }
    else
    {
        sendNewBytes(encodePiece(index, offset, data));
    }

    uploaded += data.size();
}

int Peer::getMessageType(std::vector<byte> data)
{
    if (!isHandshakeReceived)
    {
        return messageType.left.at("handshake");
    }

    //keep alive message is handled by handleRead()

    //check if message type exists in map
    if (data.size() > 4 && messageType.right.find(
        static_cast<int>(data.at(4))) != messageType.right.end())
    {
        return static_cast<int>(data.at(4));
    }

    return messageType.left.at("unknown");
}

void Peer::handleHandshake(std::vector<byte> hash, std::string id)
{
    //check which torrent the accepted peer connection is referencing
    if (isAccepted)
    {
        for (auto& ptr_torrent : *ptr_torrentList)
        {
            if (!std::equal(hash.begin(), hash.end(),
                            torrent->hashesData.infoHash.begin()))
            {
                LOG_F(ERROR, "(%s:%s) Setting peer torrent...",
                      peerHost.c_str(), peerPort.c_str());

                //set this Peer object's associated Torrent object
                //by assigning the shared ptr to that Torrent object
                torrent = ptr_torrent;
                break;
            }
        }
        if (!torrent)
        {
            LOG_F(ERROR, "(%s:%s) Local torrent not found!",
                  peerHost.c_str(), peerPort.c_str());

            disconnect();
            return;
        }
        else
        {
            LOG_F(ERROR, "(%s:%s) Invalid handshake! No local torrent info hash "
                        "matches received info hash.",
                  peerHost.c_str(), peerPort.c_str());
        }
    }
    //operator== seems to give incorrect result with vector<byte>
    else if(!std::equal(hash.begin(), hash.end(),
                        torrent->hashesData.infoHash.begin()))
    {
        LOG_F(ERROR, "(%s:%s) Invalid handshake! Incorrect infohash. "
                     "Expected: %s; "
                     "Received %s.",
              peerHost.c_str(), peerPort.c_str(),
              torrent->hashesData.hexStringInfoHash.c_str(),
              toHex(hash).c_str());

        disconnect();
        return;
    }

    peerID = id;
    isHandshakeReceived = true;

    //if the peer opened the connection and sent a handshake, we send a
    //reply handshake. Otherwise, we sent the initial handshake and so we
    //send the bitfield
    if (isAccepted)
    {
        sendHandShake();
    }
    else
    {
        sendBitfield(torrent->statusData.isPieceVerified);
    }
}

void Peer::handleKeepAlive()
{
    //don't need to do anything since lastActive variable is updated on
    //any message received
}

void Peer::handleChoke()
{
    isChokeReceived = true;

    //call slot
    if (!sig_stateChanged->empty())
    {
        sig_stateChanged->operator()(this);
    }
}

void Peer::handleUnchoke()
{
    isChokeReceived = false;

    //call slot
    if (!sig_stateChanged->empty())
    {
        sig_stateChanged->operator()(this);
    }
}

void Peer::handleInterested()
{
    isInterestedReceived = true;

    //call slot
    if (!sig_stateChanged->empty())
    {
        sig_stateChanged->operator()(this);
    }
}

void Peer::handleNotInterested()
{
    isInterestedReceived = false;

    //call slot
    if (!sig_stateChanged->empty())
    {
        sig_stateChanged->operator()(this);
    }
}

void Peer::handleHave(int index)
{
    isPieceDownloaded.at(index) = true;

    //call slot
    if (!sig_stateChanged->empty())
    {
        sig_stateChanged->operator()(this);
    }
}

void Peer::handleBitfield(std::vector<bool> recIsPieceDownloaded)
{
    //set true if we have either just been told that the peer has the
    //piece downloaded or already set to true
    for (size_t i = 0; i < torrent->piecesData.pieceCount; ++i)
    {
        isPieceDownloaded.at(i) = isPieceDownloaded.at(i) ||
            recIsPieceDownloaded.at(i);
    }

    //call slot
    if (!sig_stateChanged->empty())
    {
        sig_stateChanged->operator()(this);
    }
}

void Peer::handleDataRequest(int index, int offset, int dataSize)
{
    LOG_F(INFO, "(%s:%s) Handling 'request' message... Index: %d; "
                "offset: %d; data size: %d",
          peerHost.c_str(), peerPort.c_str(), index, offset, dataSize);

    dataRequest newDataRequest = { this, index, offset, dataSize, false };

    //pass struct and this peer's data to slot
    if (!sig_blockRequested->empty())
    {
        sig_blockRequested->operator()(newDataRequest);
    }
}

void Peer::handleCancel(int index, int offset, int dataSize)
{
    dataRequest newDataRequest = { this, index, offset, dataSize, false };

    //pass struct and this peer's data to slot
    if (!sig_blockCancelled->empty())
    {
        sig_blockCancelled->operator()(newDataRequest);
    }
}

void Peer::handlePiece(int index, int offset, std::vector<byte> data)
{
    dataPackage newPackage = { this, index,
                               offset/torrent->piecesData.blockSize, data};

    //pass struct and this peer's data to slot
    if (!sig_blockReceived->empty())
    {
        sig_blockReceived->operator()(newPackage);
    }
}

std::shared_ptr<Peer> Peer::getPtr()
{
    return shared_from_this();
}

}
