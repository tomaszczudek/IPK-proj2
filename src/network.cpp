#include "network.hpp"
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <poll.h>
#include <cerrno>

/**
 * Constructor for the Network class.
 */
Network::Network()
{
    this->sock = -1;
    this->destAddress = {};
    this->destAddressLength = 0;
    this->knowsDest = false;
}

/**
 * Destructor for the Network class.
 */
Network::~Network()
{
    if (this->sock != -1)
        close(this->sock);
}

/**
 * Set up the network as a client.
 * @param host The hostname or IP address of the server to connect to.
 * @param port The port number to connect to.
 */
void Network::setUpClient(const std::string& host, int port)
{
    struct addrinfo hints {};
    struct addrinfo* result = nullptr;

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    std::string portString = std::to_string(port);
    int ret = getaddrinfo(host.c_str(), portString.c_str(), &hints, &result);

    if (ret != 0)
    {
        std::cerr << "Error: getaddrinfo failed: " << gai_strerror(ret) << std::endl;
        throw EX_USAGE;
    }

    for (struct addrinfo* info = result; info != nullptr; info = info->ai_next)
    {
        int tmp = socket(info->ai_family, info->ai_socktype, info->ai_protocol);

        if (tmp == -1)
            continue;

        this->sock = tmp;
        sockaddr_storage resolvedAddress {};
        std::memcpy(&resolvedAddress, info->ai_addr, info->ai_addrlen);
        this->setDestination(resolvedAddress, static_cast<socklen_t>(info->ai_addrlen));

        freeaddrinfo(result);
        return;
    }

    freeaddrinfo(result);

    std::cerr << "Error: Could not create UDP client socket" << std::endl;
    throw EX_UNAVAILABLE;
}

/**
 * Set up the network as a server.
 * @param address The IP address to bind to.
 * @param port The port number to listen on.
 */
void Network::setUpServer(const std::string& address, int port)
{
    struct addrinfo hints {};
    struct addrinfo* result = nullptr;

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    const char* bindAddress = nullptr;

    if (!address.empty())
        bindAddress = address.c_str();

    std::string portString = std::to_string(port);
    int ret = getaddrinfo(bindAddress, portString.c_str(), &hints, &result);

    if (ret != 0)
    {
        std::cerr << "Error: getaddrinfo failed: " << gai_strerror(ret) << std::endl;
        throw EX_USAGE;
    }

    for (struct addrinfo* info = result; info != nullptr; info = info->ai_next)
    {
        int tmp = socket(info->ai_family, info->ai_socktype, info->ai_protocol);

        if (tmp == -1)
            continue;

        int opt = 1;
        if (setsockopt(tmp, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
        {
            std::cerr << "Error: setsockopt failed: " << strerror(errno) << std::endl;
            close(tmp);
            continue;
        }

        if (bind(tmp, info->ai_addr, info->ai_addrlen) == 0)
        {
            this->sock = tmp;
            this->knowsDest = false;
            this->destAddress = {};
            this->destAddressLength = 0;
            freeaddrinfo(result);
            return;
        }

        close(tmp);
    }

    freeaddrinfo(result);

    std::cerr << "Error: Could not bind UDP server socket" << std::endl;
    throw EX_UNAVAILABLE;
}

/**
 * Stores the destination address.
 */
void Network::setDestination(const sockaddr_storage& address, socklen_t addressLength)
{
    if (addressLength > sizeof(this->destAddress))
    {
        std::cerr << "Error: Destination address is too large" << std::endl;
        throw EX_OSERR;
    }

    this->destAddress = {};
    std::memcpy(&this->destAddress, &address, addressLength);
    this->destAddressLength = addressLength;
    this->knowsDest = true;
}

/**
 * Checks if the supplied address is the currently stored destination.
 */
bool Network::isSameDestination(const sockaddr_storage& address, socklen_t addressLength) const
{
    if (!this->knowsDest)
        return false;

    if (this->destAddressLength != addressLength)
        return false;

    return std::memcmp(&this->destAddress, &address, addressLength) == 0;
}

/**
 * Send a message through the network.
 * @param message The message to send.
 * @return True if the message was sent successfully, false otherwise.
 */
bool Network::sendMessage(const std::string& message)
{
    if (this->sock == -1)
    {
        std::cerr << "Error: Socket is not initialized" << std::endl;
        return false;
    }

    if (!this->knowsDest)
    {
        std::cerr << "Error: Destination address is not set" << std::endl;
        return false;
    }

    ssize_t sent = sendto(
        this->sock,
        message.data(),
        message.size(),
        0,
        reinterpret_cast<struct sockaddr*>(&this->destAddress),
        this->destAddressLength
    );

    if (sent == -1)
    {
        std::cerr << "Error: sendto failed: " << strerror(errno) << std::endl;
        return false;
    }

    if (static_cast<std::size_t>(sent) != message.size())
    {
        std::cerr << "Error: sendto sent only part of the message" << std::endl;
        return false;
    }

    return true;
}

/**
 * Receive a message from the network.
 * @return The received message and sender address.
 */
ReceivedMessage Network::receiveMessage(int timeoutMs, bool& timedOut)
{
    timedOut = false;

    if (this->sock == -1)
    {
        std::cerr << "Error: Socket is not initialized\n";
        throw EX_OSERR;
    }

    struct pollfd pollSock {};
    pollSock.fd = this->sock;
    pollSock.events = POLLIN;
    int ret = poll(&pollSock, 1, timeoutMs);

    if (ret == -1)
    {
        if (errno == EINTR)
        {
            timedOut = true;
            return {};
        }

        std::cerr << "Error: poll failed: " << strerror(errno) << std::endl;
        throw EX_OSERR;
    }

    if (ret == 0)
    {
        timedOut = true;
        return {};
    }

    char buffer[MAX_PACKET_SIZE];

    ReceivedMessage message {};
    message.addressLength = sizeof(message.address);

    ssize_t received = recvfrom(
        this->sock,
        buffer,
        sizeof(buffer),
        0,
        reinterpret_cast<struct sockaddr*>(&message.address),
        &message.addressLength
    );

    if (received == -1)
    {
        std::cerr << "Error: recvfrom failed: " << strerror(errno) << std::endl;
        throw EX_OSERR;
    }

    message.data = std::string(buffer, static_cast<std::size_t>(received));
    return message;
}
