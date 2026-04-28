#include "network.hpp"
#include <cstring>
#include <iostream>
#include <unistd.h>

Network::Network()
{
    this->sock = -1;
    this->destAddressLength = sizeof(this->destAddress);
    this->knowsDest = false;
}

Network::~Network()
{
    if (this->sock != -1)
    {
        close(this->sock);
    }
}

void Network::setUpClient(const std::string& host, int port)
{
    struct addrinfo hints {};
    struct addrinfo* result = nullptr;

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    int ret = getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &result);

    if (ret != 0)
    {
        std::cerr << "getaddrinfo failed: " << gai_strerror(ret) << std::endl;
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

    std::cerr << "Could not create UDP client socket" << std::endl;
    throw EX_UNAVAILABLE;
}

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
        std::cerr << "getaddrinfo failed: " << gai_strerror(ret) << std::endl;
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
            std::cerr << "setsockopt failed: " << strerror(errno) << std::endl;
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

    std::cerr << "Could not bind UDP server socket" << std::endl;
    throw EX_UNAVAILABLE;
}
