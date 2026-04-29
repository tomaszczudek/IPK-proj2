#ifndef NETWORK_INFO_HPP
#define NETWORK_INFO_HPP

#include <string>
#include <cstddef>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sysexits.h>

/*
 * Network class for handling UDP communication
 */
class Network
{
    private:
        int sock;                       //> Socket file descriptor
        sockaddr_storage destAddress;   //> Destination address for sending messages
        socklen_t destAddressLength;    //> Length of the destination address
        bool knowsDest;                 //> Flag indicating if the destination address is known

    public:
        /**
         * Constructor for the Network class.
         */
        Network();

        /**
         * Destructor for the Network class.
         */
        ~Network();

        /**
         * Set up the network as a client.
         * @param host The hostname or IP address of the server to connect to.
         * @param port The port number to connect to.
         */
        void setUpClient(const std::string& host, int port);

        /**
         * Set up the network as a server.
         * @param address The IP address to bind to.
         * @param port The port number to listen on.
         */
        void setUpServer(const std::string& address, int port);

        /**
         * Send a message through the network.
         * @param message The message to send.
         * @return True if the message was sent successfully, false otherwise.
         */
        bool sendMessage(const std::string& message);

        /**
         * Receive a message from the network.
         * @return The received message.
         */
        std::string receiveMessage();

        // Getters
        bool isSocketSet() const { return this->sock != -1; }

}; 

#endif // NETWORK_INFO_HPP
