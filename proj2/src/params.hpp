#ifndef PARAMS_HPP
#define PARAMS_HPP

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
        int isServer();

};

#endif // PARAMS_HPP
