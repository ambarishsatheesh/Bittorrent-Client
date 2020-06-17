#include "UDPClient.h"
#include "Utility.h"
#include "loguru.h"

#include <iostream>
#include <iomanip>
#include <random>
#include <limits>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/bind.hpp>

// TODO: implement time out/retransmit
// TODO: need to populate "remaining"
// TODO: port as argument (randomise?)

namespace Bittorrent
{
using namespace utility;

//port "0" in member initialisation list for socket assigns any
//free port
UDPClient::UDPClient(trackerUrl& parsedUrl, std::vector<byte>& clientID,
    std::vector<byte>& infoHash, long long& uploaded, long long& downloaded,
    long long& remaining, int& intEvent, int& port, bool isAnnounce)
    : connIDReceivedTime{}, lastRequestTime{}, peerRequestInterval{ 0 },
    leechers{ 0 }, seeders{ 0 }, completed{ 0 },
    peerHost{ parsedUrl.hostname }, peerPort{ parsedUrl.port },
    localPort{port}, isFail{ 0 }, logBuffer{""}, m_isAnnounce{isAnnounce},
    byteInfoHash{ infoHash }, ancClientID{ clientID },
    ancDownloaded{ downloaded }, ancUploaded{ uploaded },
    ancRemaining{ remaining }, ancIntEvent{ intEvent },
    recConnBuffer(16), recScrapeBuffer(200), recAncBuffer(320),
    io_context(), socket(io_context, udp::endpoint(udp::v4(), port)),
    remoteEndpoint(), localEndpoint()
{
    errorAction = { 0x0, 0x0, 0x0, 0x3 };
    dataTransmission(isAnnounce);
}


void UDPClient::run(std::chrono::steady_clock::duration timeout)
{
    //restart io_context in case there have been previous run invocations
    io_context.restart();

    io_context.run_for(timeout);

    //If not stopped, then the io_context::run_for call must have timed out.
    if (!io_context.stopped())
    {
        LOG_F(ERROR, "Asynchronous operation timed out! (Tracker %s:%s).",
            peerHost.c_str(), peerPort.c_str());

        // Close the socket to cancel the outstanding asynchronous operation.
        socket.close();
    }
}

UDPClient::~UDPClient()
{
    close();
}

void UDPClient::close()
{
    socket.close();
    isFail = true;

    LOG_F(INFO,
        "Closed UDP socket used for tracker update (%s:%s).",
        peerHost.c_str(), peerPort.c_str());
}

void UDPClient::dataTransmission(bool isAnnounce)
{
    m_isAnnounce = isAnnounce;

    udp::resolver resolver{ io_context };
    auto results = resolver.resolve(peerHost, peerPort);

    //need to bind object context using "this" for class member functions
    boost::asio::async_connect(socket, results,
        boost::bind(&UDPClient::handleConnect, this,
            boost::asio::placeholders::error));

    //set 10 second total deadline timer for all asynchronous operations
    run(std::chrono::seconds(10));
}

//establish connection acccording to Bittorrent spec after establishing
//basic network connection
void UDPClient::handleConnect(const boost::system::error_code& error)
{
    if (!error)
    {
        remoteEndpoint = socket.remote_endpoint();

        LOG_F(INFO, "Resolved UDP tracker endpoint! Endpoint: %s:%hu (%s:%s).",
            remoteEndpoint.address().to_string().c_str(), remoteEndpoint.port(),
            peerHost.c_str(), peerPort.c_str());

        //build connect request buffer
        const auto connectReqBuffer = buildConnectReq();
        logBuffer = toHex(connectReqBuffer);

        //udp is a datagram oriented socket so datagram functions need to be used
        //socket is connected so no need to use async_send_to
        socket.async_send(boost::asio::buffer(connectReqBuffer,
            connectReqBuffer.size()),
            boost::bind(&UDPClient::handleConnectSend, this,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    }
    else
    {
        LOG_F(ERROR,
            "Failed to resolve UDP tracker %s:%s! Error msg: \"%s\".",
            peerHost.c_str(), peerPort.c_str(), error.message().c_str());

        close();
    }
}

std::vector<byte> UDPClient::buildConnectReq()
{
    LOG_F(INFO, "Building UDP connect request to tracker %s:%hu (%s:%s)...",
        remoteEndpoint.address().to_string().c_str(), remoteEndpoint.port(),
        peerHost.c_str(), peerPort.c_str());

    //build connect request buffer
    protocolID = { 0x0, 0x0, 0x04, 0x17, 0x27, 0x10, 0x19, 0x80 };
    connectAction = { 0x0, 0x0, 0x0, 0x0 };

    //generate random int32 and pass into transactionID array
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<int32_t>
        dist6(0, std::numeric_limits<int32_t>::max());
    auto rndInt = dist6(rng);

    //Big endian - but endianness doesn't matter since it's random anyway
    sentTransactionID.clear();
    sentTransactionID.resize(4);
    for (int i = 0; i <= 3; ++i) {
        sentTransactionID.at(i) = ((rndInt >> (8 * (3 - i))) & 0xff);
    }

    //copy to buffer
    std::vector<byte> connectVec(16);
    auto last = std::copy(protocolID.begin(), protocolID.end(),
        connectVec.begin());
    last = std::copy(connectAction.begin(), connectAction.end(), last);
    last = std::copy(sentTransactionID.begin(), sentTransactionID.end(), last);

    //convert to appropriate buffer for sending via UDP and return
    return connectVec;
}

void UDPClient::handleConnectSend(const boost::system::error_code& error,
    const size_t& bytesTransferred)
{
    if (!error)
    {
        LOG_F(INFO, "Sent UDP connect request from %s:%hu to tracker %s:%hu "
            "(%s:%s); Status: %s; Sent bytes: %d; Sent payload (hex): %s.",
            socket.local_endpoint().address().to_string().c_str(),
            socket.local_endpoint().port(),
            remoteEndpoint.address().to_string().c_str(), remoteEndpoint.port(),
            peerHost.c_str(), peerPort.c_str(),
            error.message().c_str(), static_cast<int>(bytesTransferred),
              logBuffer.c_str());

        socket.async_receive(
            boost::asio::buffer(recConnBuffer, recConnBuffer.size()),
            boost::bind(&UDPClient::handleConnectReceive, this,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    }
    else
    {
        LOG_F(ERROR,
            "Failed to send UDP connect request to tracker %s:%hu (%s:%s)! "
            "Error msg: \"%s\".",
            remoteEndpoint.address().to_string().c_str(), remoteEndpoint.port(),
            peerHost.c_str(), peerPort.c_str(), error.message().c_str());

        close();
    }
}

void UDPClient::handleConnectReceive(const boost::system::error_code& error,
    const size_t& bytesTransferred)
{
    if (!error)
    {
        logBuffer = toHex(recConnBuffer);

        LOG_F(INFO, "Received UDP connect response from tracker %s:%hu (%s:%s); "
            "Status: %s; Received bytes: %d; Received payload (hex): %s.",
            remoteEndpoint.address().to_string().c_str(), remoteEndpoint.port(),
            peerHost.c_str(), peerPort.c_str(),
            error.message().c_str(), static_cast<int>(bytesTransferred),
              logBuffer.c_str());

        //update relevant timekeeping
        connIDReceivedTime = boost::posix_time::second_clock::local_time();
        lastRequestTime = boost::posix_time::second_clock::local_time();

        LOG_F(INFO, "Received UDP connect response at %s; "
            "Updated last request time. Tracker: %s:%hu (%s:%s).",
            boost::posix_time::to_simple_string(
                boost::posix_time::ptime(connIDReceivedTime)).c_str(),
            remoteEndpoint.address().to_string().c_str(),remoteEndpoint.port(),
            peerHost.c_str(), peerPort.c_str());

        //handle connect response
        handleConnectResp(bytesTransferred);
    }
    else
    {
        LOG_F(ERROR,
            "Failed to receive UDP connect response from tracker %s:%hu (%s:%s)! "
            "Error msg: \"%s\".",
            remoteEndpoint.address().to_string().c_str(), remoteEndpoint.port(),
            peerHost.c_str(), peerPort.c_str(), error.message().c_str());

        close();
    }
}

void UDPClient::handleConnectResp(const std::size_t& connBytesRec)
{
    //validate size
    if (connBytesRec < 16)
    {
        isFail = true;
        LOG_F(ERROR,
            "UDP Connect response packet is less than the expected 16 bytes! "
            "Tracker: %s:%hu (%s:%s).",
            remoteEndpoint.address().to_string().c_str(), remoteEndpoint.port(),
            peerHost.c_str(), peerPort.c_str());

        return;
    }

    //validate action
    std::vector<byte> receivedAction = { recConnBuffer.begin(),
        recConnBuffer.begin() + 4 };
    if (receivedAction != connectAction)
    {
        isFail = true;
        LOG_F(ERROR,
            "Received UDP connect response action is not \"connect\"! "
            "Tracker: %s:%hu (%s:%s).",
            remoteEndpoint.address().to_string().c_str(), remoteEndpoint.port(),
            peerHost.c_str(), peerPort.c_str());

        return;
    }

    //validate transaction id
    std::vector<byte> receivedTransactionID = { recConnBuffer.begin() + 4,
        recConnBuffer.begin() + 8 };
    if (receivedTransactionID != sentTransactionID)
    {
        isFail = true;
        LOG_F(ERROR,
            "UDP connect response transaction ID is not equal to sent "
            "transaction ID! Tracker: %s:%hu (%s:%s).",
            remoteEndpoint.address().to_string().c_str(),
            remoteEndpoint.port(), peerHost.c_str(), peerPort.c_str());

        return;
    }

    //store connection id
    connectionID = { recConnBuffer.begin() + 8,
        recConnBuffer.end() };

    std::string ConnIDStr = toHex(connectionID);

    LOG_F(INFO, "Handled UDP connect response from tracker %s:%hu (%s:%s); "
        "Connection ID (hex): %s.",
        remoteEndpoint.address().to_string().c_str(),
        remoteEndpoint.port(), peerHost.c_str(), peerPort.c_str(),
        ConnIDStr.c_str());

    //check if we have announced before and if connect has failed
    if (m_isAnnounce && !isFail)
    {
        announceRequest();
    }
    else if (!isFail)
    {
        scrapeRequest();
    }
}

void UDPClient::scrapeRequest()
{
    //build scrape request buffer
    const auto scrapeReqBuffer = buildScrapeReq();
    logBuffer = toHex(scrapeReqBuffer);

    socket.async_send(
        boost::asio::buffer(scrapeReqBuffer, scrapeReqBuffer.size()),
        boost::bind(&UDPClient::handleScrapeSend, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
}

std::vector<byte> UDPClient::buildScrapeReq()
{
    LOG_F(INFO, "Building UDP scrape request to tracker %s:%hu (%s:%s)...",
        remoteEndpoint.address().to_string().c_str(),
        remoteEndpoint.port(), peerHost.c_str(), peerPort.c_str());

    //build scrape request buffer
    scrapeAction = { 0x0, 0x0, 0x0, 0x2 };

    //generate random int32 and pass into transactionID array
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<int32_t>
        dist6(0, std::numeric_limits<int32_t>::max());
    auto rndInt = dist6(rng);

    //Big endian - but endianness doesn't matter since it's random anyway
    sentTransactionID.clear();
    sentTransactionID.resize(4);
    for (int i = 0; i <= 3; ++i) {
        sentTransactionID.at(i) = ((rndInt >> (8 * (3 - i))) & 0xff);
    }

    //copy to buffer
    std::vector<byte> scrapeVec(36);
    auto last = std::copy(connectionID.begin(), connectionID.end(),
        scrapeVec.begin());
    last = std::copy(scrapeAction.begin(), scrapeAction.end(), last);
    last = std::copy(sentTransactionID.begin(), sentTransactionID.end(),
        last);
    last = std::copy(byteInfoHash.begin(), byteInfoHash.end(), last);

    //convert to appropriate buffer for sending via UDP and return
    return scrapeVec;
}

void UDPClient::handleScrapeSend(const boost::system::error_code& error,
    const size_t& bytesTransferred)
{
    if (!error)
    {
        LOG_F(INFO, "Sent UDP scrape request from %s:%hu to tracker %s:%hu "
            "(%s:%s); Status: %s; Sent bytes: %d; Sent payload (hex): %s.",
            socket.local_endpoint().address().to_string().c_str(),
            socket.local_endpoint().port(),
            remoteEndpoint.address().to_string().c_str(), remoteEndpoint.port(),
            peerHost.c_str(), peerPort.c_str(),
            error.message().c_str(), static_cast<int>(bytesTransferred),
              logBuffer.c_str());

        socket.async_receive(
            boost::asio::buffer(recScrapeBuffer, recScrapeBuffer.size()),
            boost::bind(&UDPClient::handleScrapeReceive, this,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    }
    else
    {
        LOG_F(ERROR,
            "Failed to send UDP scrape request to tracker %s:%hu (%s:%s) "
            "from %s:%hu! Error msg: \"%s\".",
            remoteEndpoint.address().to_string().c_str(), remoteEndpoint.port(),
            peerHost.c_str(), peerPort.c_str(),
            socket.local_endpoint().address().to_string().c_str(),
            socket.local_endpoint().port(), error.message().c_str());

        close();
    }
}

void UDPClient::handleScrapeReceive(const boost::system::error_code& error,
    const size_t& bytesTransferred)
{
    if (!error)
    {
        //remove redundant data at end
        recScrapeBuffer.erase(recScrapeBuffer.begin() + bytesTransferred,
            recScrapeBuffer.end());

        logBuffer = toHex(recScrapeBuffer);

        LOG_F(INFO, "Received UDP scrape response from tracker %s:%hu (%s:%s); "
            "Status: %s; Received bytes: %d; Received payload (hex): %s.",
            remoteEndpoint.address().to_string().c_str(), remoteEndpoint.port(),
            peerHost.c_str(), peerPort.c_str(),
            error.message().c_str(), static_cast<int>(bytesTransferred),
              logBuffer.c_str());

        //update relevant timekeeping
        connIDReceivedTime = boost::posix_time::second_clock::local_time();
        lastRequestTime = boost::posix_time::second_clock::local_time();

        LOG_F(INFO, "Received UDP scrape response at %s; "
            "Updated last request time. Tracker: %s:%hu (%s:%s).",
            boost::posix_time::to_simple_string(
                boost::posix_time::ptime(connIDReceivedTime)).c_str(),
            remoteEndpoint.address().to_string().c_str(),
            remoteEndpoint.port(), peerHost.c_str(), peerPort.c_str());

        //handle connect response
        handleScrapeResp(bytesTransferred);
    }
    else
    {
        LOG_F(ERROR,
            "Failed to receive UDP scrape response from tracker %s:%s (%s:%s)! "
            "Error msg: \"%s\".",
            peerHost.c_str(), peerPort.c_str(), error.message().c_str(),
            peerHost.c_str(), peerPort.c_str());

        close();
    }
}

void UDPClient::handleScrapeResp(const std::size_t& scrapeBytesRec)
{
    //validate size
    if (scrapeBytesRec < 8)
    {
        isFail = true;
        LOG_F(ERROR,
            "UDP connect response packet is less than the expected 8 bytes! "
            "Tracker: %s:%hu (%s:%s).",
            remoteEndpoint.address().to_string().c_str(),
            remoteEndpoint.port(), peerHost.c_str(), peerPort.c_str());
        return;
    }

    //validate transaction id
    std::vector<byte> receivedTransactionID = { recScrapeBuffer.begin() + 4,
        recScrapeBuffer.begin() + 8 };
    if (receivedTransactionID != receivedTransactionID)
    {
        isFail = true;
        LOG_F(ERROR,
            "UDP scrape response transaction ID is not equal to sent "
            "transaction ID! Tracker: %s:%hu (%s:%s).",
            remoteEndpoint.address().to_string().c_str(),
            remoteEndpoint.port(), peerHost.c_str(), peerPort.c_str());
        return;
    }

    //validate action
    std::vector<byte> receivedAction = { recScrapeBuffer.begin(),
        recScrapeBuffer.begin() + 4 };

    if (receivedAction == scrapeAction)
    {
        //convert seeders bytes to int
        for (size_t i = 8; i < 12; ++i)
        {
            seeders <<= 8;
            seeders |= recScrapeBuffer.at(i);
        }

        //convert completed bytes to int
        for (size_t i = 12; i < 16; ++i)
        {
            completed <<= 8;
            completed |= recScrapeBuffer.at(i);
        }

        //convert leechers bytes to int
        for (size_t i = 16; i < 20; ++i)
        {
            leechers <<= 8;
            leechers |= recScrapeBuffer.at(i);
        }

        LOG_F(INFO, "Handled UDP scrape response from tracker %s:%hu (%s:%s); "
            "Updated peer data.",
            remoteEndpoint.address().to_string().c_str(), remoteEndpoint.port(),
            peerHost.c_str(), peerPort.c_str());

    }
    else if (receivedAction == errorAction)
    {
        //get error message
        std::string scrapeErrorMsg(recScrapeBuffer.begin() + 8,
            recScrapeBuffer.begin() + scrapeBytesRec);

        isFail = true;
        LOG_F(ERROR,
            "Tracker UDP scrape error (tracker %s:%hu - %s:%s): %s.",
            remoteEndpoint.address().to_string().c_str(), remoteEndpoint.port(),
            peerHost.c_str(), peerPort.c_str(),
            scrapeErrorMsg.c_str());
    }
}

void UDPClient::announceRequest()
{
    //build connect request buffer
    const auto announceReqBuffer = buildAnnounceReq();

    socket.async_send(
        boost::asio::buffer(announceReqBuffer, announceReqBuffer.size()),
        boost::bind(&UDPClient::handleAnnounceSend, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
}

std::vector<byte> UDPClient::buildAnnounceReq()
{
    LOG_F(INFO, "Building UDP announce request to tracker %s:%hu (%s:%s)...",
        remoteEndpoint.address().to_string().c_str(), remoteEndpoint.port(),
        peerHost.c_str(), peerPort.c_str());

    //generate announce action (1)
    ancAction = { 0x0, 0x0, 0x0, 0x01 };

    //generate random int32 and pass into transactionID array
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<int32_t>
        dist6(0, std::numeric_limits<int32_t>::max());
    auto rndInt = dist6(rng);

    //Big endian - but endianness doesn't matter since it's random anyway
    sentTransactionID.clear();
    sentTransactionID.resize(4);
    for (int i = 0; i <= 3; ++i) {
        sentTransactionID.at(i) = ((rndInt >> (8 * (3 - i))) & 0xff);
    }

    //downloaded bytes
    std::vector<byte> downloadedVec;
    downloadedVec.resize(8);
    for (int i = 0; i <= 7; ++i) {
        downloadedVec.at(i) = ((ancDownloaded >> (8 * (7 - i))) & 0xff);
    }

    //remaining bytes
    std::vector<byte> remainingVec;
    remainingVec.resize(8);
    for (int i = 0; i <= 7; ++i) {
        remainingVec.at(i) = ((ancRemaining >> (8 * (7 - i))) & 0xff);
    }

    //uploaded bytes
    std::vector<byte> uploadedVec;
    uploadedVec.resize(8);
    for (int i = 0; i <= 7; ++i) {
        uploadedVec.at(i) = ((ancUploaded >> (8 * (7 - i))) & 0xff);
    }

    //event
    std::vector<byte> eventVec;
    eventVec.resize(4);
    for (int i = 0; i <= 3; ++i) {
        eventVec.at(i) = ((ancIntEvent >> (8 * (3 - i))) & 0xff);
    }

    //default 0 - optional - can be determined from udp request
    std::vector<byte> ipVec = { 0x0, 0x0, 0x0, 0x0 };

    //generate random int32 for key value and store in vec
    //only in case ip changes
    rndInt = dist6(rng);
    std::vector<byte> keyVec;
    keyVec.resize(4);
    for (int i = 0; i <= 3; ++i) {
        keyVec.at(i) = ((rndInt >> (8 * (3 - i))) & 0xff);
    }

    //number of peers that the client would like to receive from the tracker
    //this client will have max 10 (better for performance)
    std::vector<byte> numWantVec;
    numWantVec.resize(4);
    int32_t numWantInt = 10;
    for (int i = 0; i <= 3; ++i) {
        numWantVec.at(i) = ((numWantInt >> (8 * (3 - i))) & 0xff);
    }

    //port
    std::vector<byte> portVec;
    portVec.resize(2);
    int32_t intPort = localPort;
    for (int i = 0; i <= 1; ++i) {
        portVec.at(i) = ((intPort >> (8 * (1 - i))) & 0xff);
    }

    //copy each section to buffer
    std::vector<byte> announceVec(98);
    auto last = std::copy(connectionID.begin(), connectionID.end(), announceVec.begin());
    last = std::copy(ancAction.begin(), ancAction.end(), last);
    last = std::copy(sentTransactionID.begin(), sentTransactionID.end(), last);
    last = std::copy(byteInfoHash.begin(), byteInfoHash.end(), last);
    last = std::copy(ancClientID.begin(), ancClientID.end(), last);
    last = std::copy(downloadedVec.begin(), downloadedVec.end(), last);
    last = std::copy(remainingVec.begin(), remainingVec.end(), last);
    last = std::copy(uploadedVec.begin(), uploadedVec.end(), last);
    last = std::copy(eventVec.begin(), eventVec.end(), last);
    last = std::copy(ipVec.begin(), ipVec.end(), last);
    last = std::copy(keyVec.begin(), keyVec.end(), last);
    last = std::copy(numWantVec.begin(), numWantVec.end(), last);
    last = std::copy(portVec.begin(), portVec.end(), last);

    //convert to appropriate buffer for sending via UDP and return
    return announceVec;
}

void UDPClient::handleAnnounceSend(const boost::system::error_code& error,
    const size_t& bytesTransferred)
{
    if (!error)
    {
        LOG_F(INFO, "Sent UDP announce request from %s:%hu to tracker %s:%hu "
            "(%s:%s); Status: %s; Sent bytes: %d; Sent payload (hex): %s.",
            socket.local_endpoint().address().to_string().c_str(),
            socket.local_endpoint().port(),
            remoteEndpoint.address().to_string().c_str(), remoteEndpoint.port(),
            peerHost.c_str(), peerPort.c_str(),
            error.message().c_str(), static_cast<int>(bytesTransferred),
              logBuffer.c_str());

        //set max peers for this client to 50 - more than enough
        //(6 bytes * 50 = 300) + 20 for other info = 320 bytes max
        //size initialised in constructor
        socket.async_receive(
            boost::asio::buffer(recAncBuffer, recScrapeBuffer.size()),
            boost::bind(&UDPClient::handleAnnounceReceive, this,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    }
    else
    {
        LOG_F(ERROR,
            "Failed to send UDP announce request to tracker %s:%hu (%s:%s) from "
            "%s:%hu! Error msg: \"%s\".",
            remoteEndpoint.address().to_string().c_str(), remoteEndpoint.port(),
            peerHost.c_str(), peerPort.c_str(),
            socket.local_endpoint().address().to_string().c_str(),
            socket.local_endpoint().port(), error.message().c_str());

        close();
    }
}

void UDPClient::handleAnnounceReceive(const boost::system::error_code& error,
    const size_t& bytesTransferred)
{
    if (!error)
    {
        //remove redundant data at end
        recAncBuffer.erase(recAncBuffer.begin() + bytesTransferred,
            recAncBuffer.end());

        logBuffer = toHex(recAncBuffer);

        LOG_F(INFO, "Received UDP announce response from tracker %s:%hu (%s:%s); "
            "Status: %s; Received bytes: %d; Received payload (hex): %s.",
            remoteEndpoint.address().to_string().c_str(), remoteEndpoint.port(),
            peerHost.c_str(), peerPort.c_str(),
            error.message().c_str(), static_cast<int>(bytesTransferred),
              logBuffer.c_str());

        //update relevant timekeeping
        lastRequestTime = boost::posix_time::second_clock::local_time();

        LOG_F(INFO, "Received UDP announce response at %s; "
            "Updated last request time. Tracker: %s:%hu (%s:%s).",
            boost::posix_time::to_simple_string(
                boost::posix_time::ptime(lastRequestTime)).c_str(),
            remoteEndpoint.address().to_string().c_str(),
            remoteEndpoint.port(), peerHost.c_str(), peerPort.c_str());

        //handle connect response
        handleAnnounceResp(bytesTransferred);
    }
    else
    {
        LOG_F(ERROR,
            "Failed to receive UDP announce response from tracker %s:%hu "
            "(%s:%s)! Error msg: \"%s\".",
            remoteEndpoint.address().to_string().c_str(), remoteEndpoint.port(),
            peerHost.c_str(), peerPort.c_str(), error.message().c_str());

        close();
    }
}

void UDPClient::handleAnnounceResp(const std::size_t& AncBytesRec)
{
    //validate size
    if (AncBytesRec < 20)
    {
        isFail = true;
        LOG_F(ERROR,
            "UDP announce response packet is less than the minimum size "
            "of 20 bytes! Tracker: %s:%hu (%s:%s).",
            remoteEndpoint.address().to_string().c_str(), remoteEndpoint.port(),
            peerHost.c_str(), peerPort.c_str());

        return;
    }

    //validate action
    std::vector<byte> receivedAction = { recAncBuffer.begin(),
        recAncBuffer.begin() + 4 };
    if (receivedAction != ancAction)
    {
        isFail = true;
        LOG_F(ERROR,
            "Received UDP announce response action is not \"announce\"! "
            "Tracker: %s:%hu (%s:%s).",
            remoteEndpoint.address().to_string().c_str(), remoteEndpoint.port(),
            peerHost.c_str(), peerPort.c_str());

        return;
    }

    //validate transaction id
    std::vector<byte> receivedTransactionID = { recAncBuffer.begin() + 4,
        recAncBuffer.begin() + 8 };
    if (receivedTransactionID != sentTransactionID)
    {
        isFail = true;
        LOG_F(ERROR,
            "UDP announce response transaction ID is not equal to sent "
            "transaction ID! Tracker: %s:%hu (%s:%s).",
            remoteEndpoint.address().to_string().c_str(), remoteEndpoint.port(),
            peerHost.c_str(), peerPort.c_str());

        return;
    }

    //convert interval bytes to integer and store as seconds
    int intInterval = 0;
    for (size_t i = 8; i < 12; ++i)
    {
        intInterval <<= 8;
        intInterval |= recAncBuffer.at(i);
    }
    peerRequestInterval = std::chrono::seconds(intInterval);

    //convert leechers bytes to int
    for (size_t i = 12; i < 16; ++i)
    {
        leechers <<= 8;
        leechers |= recAncBuffer.at(i);
    }

    //convert seeders bytes to int
    for (size_t i = 16; i < 20; ++i)
    {
        seeders <<= 8;
        seeders |= recAncBuffer.at(i);
    }

    LOG_F(INFO, "Handled UDP scrape response from tracker %s:%hu (%s:%s); "
        "Updated peer data.",
        remoteEndpoint.address().to_string().c_str(), remoteEndpoint.port(),
        peerHost.c_str(), peerPort.c_str());

    //convert ip address bytes to int
    std::vector<byte> peerInfo(recAncBuffer.begin() + 20,
        recAncBuffer.begin() + AncBytesRec);
    //info is split into chunks of 6 bytes
    for (size_t i = 0; i < peerInfo.size(); i+=6)
    {
        //first four bytes of each chunk form the ip address
        std::string ipAddress = std::to_string(peerInfo.at(i)) + "."
            + std::to_string(peerInfo.at(i + 1)) + "." +
            std::to_string(peerInfo.at(i + 2)) + "."
            + std::to_string(peerInfo.at(i + 3));
        //the two bytes representing port are in big endian
        //read in correct order directly using bit shifting
        std::string peerPort = std::to_string(
            (peerInfo.at(i + 4) << 8) | (peerInfo.at(i + 5)));
        //add to peers list
        peer singlePeer;
        singlePeer.ipAddress = ipAddress;
        singlePeer.port = peerPort;
        peerList.push_back(singlePeer);
    }

    LOG_F(INFO, "Handled UDP announce response from tracker %s:%hu (%s:%s); "
        "Updated peer data.",
        remoteEndpoint.address().to_string().c_str(), remoteEndpoint.port(),
        peerHost.c_str(), peerPort.c_str());
}

}
