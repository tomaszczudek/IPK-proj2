#ifndef NETWORK_INFO_HPP
#define NETWORK_INFO_HPP

#include <string>
#include <cstddef>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sysexits.h>

#define WINDOW_SIZE 32
#define RETRANSMIT_MS 120
#define POLL_TIMEOUT_MS 20
#define MAX_PACKET_SIZE 1200

/**
 * Structure for storing received messages and their sender addresses.
 */
struct ReceivedMessage
{
    std::string data;                //> Data of the received message
    sockaddr_storage address{};      //> Address of the sender
    socklen_t addressLength = 0;     //> Length of the sender's address
};

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
         * Stores the destination address.
         */
        void setDestination(const sockaddr_storage& address, socklen_t addressLength);

        /**
         * Checks if the supplied address is the currently stored destination.
         */
        bool isSameDestination(const sockaddr_storage& address, socklen_t addressLength) const;

        /**
         * Send a message through the network.
         * @param message The message to send.
         * @return True if the message was sent successfully, false otherwise.
         */
        bool sendMessage(const std::string& message);

        /**
         * Receive a message from the network.
         * @return The received message and sender address.
         */
        ReceivedMessage receiveMessage(int timeoutMs, bool& timedOut);

        // Getters
        bool isSocketSet() const { return this->sock != -1; }
        bool isDestinationKnown() const { return this->knowsDest; }
};

#endif // NETWORK_INFO_HPP
