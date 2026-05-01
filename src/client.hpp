#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <fstream>
#include <iostream>
#include <string>
#include <chrono>

#include "packets.hpp"
#include "network.hpp"
#include "params.hpp"

/**
 * Struct to store data about sent packets.
 */
struct SentPacketsData
{
    std::string data;                               //> Data of the sent packet
    std::chrono::steady_clock::time_point lastSent; //> Time when the packet was last sent
};

/**
 * Client class for handling client-side operations
 */
class Client
{
    private:
        Packet packet;              //> Packet instance for handling packet creation and parsing
        Network network;            //> Network instance for handling UDP communication
        const Params params;        //> Parameters for the client
        std::ifstream inputFile;    //> Input file stream for reading data from a file
        std::istream* input;        //> Pointer to the input stream (file or stdin)
        uint32_t connectionId;      //> Unique identifier for the connection
        
        /**
         * Generates a random connection ID.
         * @return A random connection ID.
         */
        std::uint32_t generateConnectionId();

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
         * @param connectionId The unique identifier for the connection.
         * @param type Type of the message.
         * @param id ID of the message.
         * @param data Data to be sent.
         */
        void sendMessage(uint32_t connectionId, MessageType type, uint32_t id, const std::string& data);

    public:
        /**
         * Constructor for the Client class.
         * @param params Parameters for the client.
         */
        Client(const Params params);

        /**
         * Starts the client-side FSM to handle the communication.
         */
        void startClient();

        // Getters
        uint32_t getConnectionId() const { return this->connectionId; }
};

#endif // CLIENT_HPP
