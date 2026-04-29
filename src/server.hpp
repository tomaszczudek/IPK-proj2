#ifndef SERVER_HPP
#define SERVER_HPP

#include <fstream>
#include <iostream>
#include <string>

#include "packets.hpp"
#include "network.hpp"
#include "params.hpp"

/**
 * Server class for handling server-side operations.
 */
class Server
{
    private:
        Packet packet;              //> Packet instance for handling packet creation and parsing
        Network network;            //> Network instance for handling UDP communication
        Params params;              //> Parameters for the server
        std::ofstream outputFile;   //> Output file stream for writing data to a file
        std::ostream* output;       //> Pointer to the output stream (file or stdout)

        /**
         * Opens the output stream (file or stdout).
         */
        void openOutput();

        /**
         * Writes data to the output stream.
         * @param data The data to write.
         */
        void writeData(const std::string& data);

        /**
         * Initializes the server.
         * Sets up server, packet, and opens output stream.
         */
        void initServer();

        /**
         * Sends a message through the network.
         * @param type Type of the message.
         * @param id ID of the message.
         * @param data Data to be sent.
         */
        void sendMessage(MessageType type, uint32_t id, const std::string& data);
        
    public:
        /**
         * Constructor for the Server class.
         * @param params Parameters for the server.
         */
        Server(Params params);

        /**
         * Starts the server-side FSM to handle the communication.
         */
        void startServer();
};

#endif // SERVER_HPP
