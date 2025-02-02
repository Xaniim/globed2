#include "udp_socket.hpp"
#include <util/net.hpp>

UdpSocket::UdpSocket() : socket_(0) {
    memset(&destAddr_, 0, sizeof(destAddr_));
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    socket_ = sock;

    GLOBED_REQUIRE(sock != -1, "failed to create a udp socket: socket failed");
}

UdpSocket::~UdpSocket() {
    this->close();
}

Result<> UdpSocket::connect(const std::string_view serverIp, unsigned short port) {
    destAddr_.sin_family = AF_INET;

    GLOBED_UNWRAP(util::net::initSockaddr(serverIp, port, destAddr_))

    connected = true;
    return Ok();
}

int UdpSocket::send(const char* data, unsigned int dataSize) {
    GLOBED_REQUIRE(connected, "attempting to call UdpSocket::send on a disconnected socket")

    int retval = sendto(socket_, data, dataSize, 0, reinterpret_cast<struct sockaddr*>(&destAddr_), sizeof(destAddr_));

    if (retval == -1) {
        util::net::throwLastError();
    }

    return retval;
}

Result<int> UdpSocket::sendTo(const char* data, unsigned int dataSize, const std::string_view address, unsigned short port) {
    // stinky windows returns wsa error 10014 if sockaddr is a stack pointer
    std::unique_ptr<sockaddr_in> addr = std::make_unique<sockaddr_in>();

    addr->sin_family = AF_INET;

    GLOBED_UNWRAP(util::net::initSockaddr(address, port, *addr))

    int retval = sendto(socket_, data, dataSize, 0, reinterpret_cast<struct sockaddr*>(addr.get()), sizeof(sockaddr));

    if (retval == -1) {
        return Err(util::net::lastErrorString());
    }

    return Ok(retval);
}

void UdpSocket::disconnect() {
    connected = false;
}

RecvResult UdpSocket::receive(char* buffer, int bufferSize) {
    sockaddr_in source;
    socklen_t addrLen = sizeof(source);

    int result = recvfrom(socket_, buffer, bufferSize, 0, reinterpret_cast<struct sockaddr*>(&source), &addrLen);

    bool fromServer = false;
    if (this->connected) {
        fromServer = util::net::sameSockaddr(source, destAddr_);
    }

    return RecvResult {
        .fromServer = fromServer,
        .result = result,
    };
}

bool UdpSocket::close() {
    if (!connected) return true;

    connected = false;
#ifdef GEODE_IS_WINDOWS
    return ::closesocket(socket_) == 0;
#else
    return ::close(socket_) == 0;
#endif
}

Result<bool> UdpSocket::poll(int msDelay, bool in) {
    GLOBED_SOCKET_POLLFD fds[1];

    fds[0].fd = socket_;
    fds[0].events = in ? POLLIN : POLLOUT;

    int result = GLOBED_SOCKET_POLL(fds, 1, msDelay);

    if (result == -1) {
        return Err(util::net::lastErrorString());
    }

    return Ok(result > 0);
}

void UdpSocket::setNonBlocking(bool nb) {
    GLOBED_UNIMPL("UdpSocket::setNonBlocking")
}