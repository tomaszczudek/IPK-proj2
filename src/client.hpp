#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <fstream>
#include <iostream>
#include <string>

#include "packets.hpp"
#include "network.hpp"
#include "params.hpp"

class Client
{
    private:
        Packet packet;
        Network network;
        Params params;
        std::ifstream inputFile;
        std::istream* input;


        void openInput();
        std::string readData();
        void initClient();
        void sendMessage(MessageType type, uint32_t id, const std::string& data);

    public:
        Client(Params params);
        void startClient();
};

#endif // CLIENT_HPP
