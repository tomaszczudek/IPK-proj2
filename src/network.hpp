#ifndef NETWORK_INFO_HPP
#define NETWORK_INFO_HPP

#include <string>
#include <cstddef>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sysexits.h>

class Network
{
    private:
        int sock;
        sockaddr_storage destAddress;
        socklen_t destAddressLength;
        bool knowsDest;

    public:
        Network();
        ~Network();
        void setUpClient(const std::string& host, int port);
        void setUpServer(const std::string& address, int port);
        
}; 

#endif // NETWORK_INFO_HPP
