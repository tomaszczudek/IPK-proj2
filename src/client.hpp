#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <fstream>
#include <iostream>
#include <string>

#include "packets.hpp"
#include "network.hpp"

class Client
{
    private:
        Packet packet;
        Network network;
        std::ifstream inputFile;
        std::istream* input;


        void openInput(const std::string& filename);
        void readData();
        void setUpClient();

    public:
        Client();

};

#endif // CLIENT_HPP
