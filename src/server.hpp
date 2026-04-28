#ifndef SERVER_HPP
#define SERVER_HPP

#include <fstream>
#include <iostream>
#include <string>

#include "packets.hpp"
#include "network.hpp"
#include "params.hpp"

class Server
{
    private:
        Packet packet;
        Network network;
        Params params;
        std::ofstream outputFile;
        std::ostream* output;


        void openOutput();
        void writeData(const std::string& data);
        void initServer();
        void sendMessage(MessageType type, uint32_t id, const std::string& data);
        
    public:
        Server(Params params);
        void startServer();
};

#endif // SERVER_HPP
