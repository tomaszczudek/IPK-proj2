#ifndef PARAMS_HPP
#define PARAMS_HPP

#define SERVER    1
#define CLIENT    0
#define UNDEFINED -1

#include <sysexits.h>
#include <iostream>
#include <string>

/**
 * Class representing the command-line parameters for the application.
 */
class Params
{
    private:
        std::string address;    //> Server address (client) or local bind address (server)
        std::string file;       //> File to read from (server) or write to (client)
        int isServer;           //> Flag indicating if the application is running as a server
        int portUDP;            //> UDP port number
        float timeout;            //> Timeout value

    public:
        /**
         * Constructor for the Params class.
         */
        Params();

        /**
         * Prints the help message for the application.
         */
        void printHelp();

        /**
         * Parses the command-line arguments.
         * @param argc The number of command-line arguments.
         * @param argv The array of command-line arguments.
         */
        void parseArgs(int argc, char **argv);

        /**
         * Validates the parsed parameters.
         */
        void validate();
        
        // Getters
        std::string getAddress() const { return this->address; }
        std::string getFile() const { return this->file; }
        int getIsServer() const { return this->isServer; }
        int getPortUDP() const { return this->portUDP; }
        float getTimeout() const { return this->timeout; }
};

#endif // PARAMS_HPP
