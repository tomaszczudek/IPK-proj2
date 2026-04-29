#include "network.hpp"
#include <cstring>
#include <iostream>
#include <unistd.h>

#define MAX_PACKET_SIZE 1200

/**
 * Constructor for the Network class.
 */
Network::Network()
{
    this->sock = -1;
    this->destAddressLength = sizeof(this->destAddress);
    this->knowsDest = false;
}

/**
 * Destructor for the Network class.
 */
Network::~Network()
{
    if (this->sock != -1)
    {
        close(this->sock);
    }
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

    int ret = getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &result);

    if (ret != 0)
    {
        std::cerr << "Error: getaddrinfo failed: " << gai_strerror(ret) << std::endl;
        throw EX_USAGE;
    }

    for (struct addrinfo* rp = result; rp != nullptr; rp = rp->ai_next)
    {
        this->sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

        if (this->sock == -1)
            continue;

        std::memcpy(&this->destAddress, rp->ai_addr, rp->ai_addrlen);
        this->destAddressLength = static_cast<socklen_t>(rp->ai_addrlen);
        this->knowsDest = true;

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

    int ret = getaddrinfo(bindAddress, std::to_string(port).c_str(), &hints, &result);

    if (ret != 0)
    {
        std::cerr << "Error: getaddrinfo failed: " << gai_strerror(ret) << std::endl;
        throw EX_USAGE;
    }

    for (struct addrinfo* rp = result; rp != nullptr; rp = rp->ai_next)
    {
        int tmp = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

        if (tmp == -1)
            continue;

        int opt = 1;
        if (setsockopt(tmp, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
        {
            std::cerr << "Error: setsockopt failed: " << strerror(errno) << std::endl;
            close(tmp);
            continue;
        }

        if (bind(tmp, rp->ai_addr, rp->ai_addrlen) == 0)
        {
            this->sock = tmp;
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
 * Send a message through the network.
 * @param message The message to send.
 * @return True if the message was sent successfully, false otherwise.
 */
bool Network::sendMessage(const std::string& message)
{
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
 * @return The received message.
 */
std::string Network::receiveMessage()
{
    char buffer[MAX_PACKET_SIZE];

    sockaddr_storage senderAddress {};
    socklen_t senderAddressLength = sizeof(senderAddress);

    ssize_t received = recvfrom(
        this->sock,
        buffer,
        sizeof(buffer),
        0,
        reinterpret_cast<struct sockaddr*>(&senderAddress),
        &senderAddressLength
    );

    if (received == -1)
    {
        std::cerr << "Error: recvfrom failed: " << strerror(errno) << std::endl;
        throw EX_OSERR;
    }

    if (!this->knowsDest)
    {
        std::memcpy(&this->destAddress, &senderAddress, senderAddressLength);
        this->destAddressLength = senderAddressLength;
        this->knowsDest = true;
    }

    return std::string(buffer, static_cast<std::size_t>(received));
}
