#ifndef SERVER_HPP
#define SERVER_HPP

#include <fstream>
#include <iostream>
#include <string>

#include "packets.hpp"
#include "network.hpp"

class Server
{
    private:
        Packet packet;
        Network network;
        std::ofstream outputFile;
        std::ostream* output;


        void openOutput(const std::string& filename);
        void writeData(const std::string& data);
        void setUpServer();

    public:
        Server();
};

#endif // SERVER_HPP
