#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <fstream>
#include <iostream>
#include <string>

#include "packets.hpp"
#include "network.hpp"
#include "params.hpp"

/**
 * Client class for handling client-side operations
 */
class Client
{
    private:
        Packet packet;              //> Packet instance for handling packet creation and parsing
        Network network;            //> Network instance for handling UDP communication
        Params params;              //> Parameters for the client
        std::ifstream inputFile;    //> Input file stream for reading data from a file
        std::istream* input;        //> Pointer to the input stream (file or stdin)

        /**
         * Opens the input stream (file or stdin).
         */
        void openInput();

        /**
         * Reads data from the input stream.
         * @return String containing the data if EOF is not reached, otherwise empty string.
         */
        std::string readData();

        /**
         * Initializes the client.
         * Sets up client, packet, and opens input stream.
         */
        void initClient();

        /**
         * Sends a message through the network.
         * @param type Type of the message.
         * @param id ID of the message.
         * @param data Data to be sent.
         */
        void sendMessage(MessageType type, uint32_t id, const std::string& data);

    public:
        /**
         * Constructor for the Client class.
         * @param params Parameters for the client.
         */
        Client(Params params);

        /**
         * Starts the client-side FSM to handle the communication.
         */
        void startClient();
};

#endif // CLIENT_HPP
