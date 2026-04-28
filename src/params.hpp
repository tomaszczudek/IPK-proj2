#ifndef PARAMS_HPP
#define PARAMS_HPP

#define SERVER    1
#define CLIENT    0
#define UNDEFINED -1

#include <sysexits.h>
#include <iostream>
#include <string>

class Params
{
    private:
        std::string address;
        std::string file;
        int isServer;
        int portUDP;
        int timeout;

    public:
        Params();
        void printHelp();
        void parseArgs(int argc, char **argv);
        void validate();
        
        // Getters
        std::string getAddress() const { return this->address; }
        std::string getFile() const { return this->file; }
        int getIsServer() const { return this->isServer; }
        int getPortUDP() const { return this->portUDP; }
        int getTimeout() const { return this->timeout; }
};

#endif // PARAMS_HPP
