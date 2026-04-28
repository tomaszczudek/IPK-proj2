#include "params.hpp"

Params::Params()
{
    address = "";
    file = "";
    isServer = -1;
    portUDP = -1;
    timeout = 1;
}

void Params::printHelp()
{
    std::cout << "Usage" << std::endl;
    std::cout << "Server:  " << std::endl << "\t";
    std::cout << "./ipk-rdt -s -p PORT [-a ADDRESS] [-o OUTPUT] [-w TIMEOUT] [-h | --help]" << std::endl;
    std::cout << "Client:  " << std::endl << "\t";
    std::cout << "./ipk-rdt -c -p PORT -a HOST [-i INPUT] [-w TIMEOUT] [-h | --help]" << std::endl << std::endl;
    std::cout << "\t-h/--help  -- print this help message" << std::endl;
    std::cout << "\t-s         -- run in server mode" << std::endl;
    std::cout << "\t-c         -- run in client mode" << std::endl;
    std::cout << "\t-p PORT    -- UDP port number" << std::endl;
    std::cout << "\t-a ADDRESS -- in server mode - the local bind address. If omitted, the server listens on all suitable local addresses." << std::endl;
    std::cout << "\t-a HOST    -- in client mode - the destination hostname or IPv4/IPv6 address." << std::endl;
    std::cout << "\t-i INPUT   -- the input file to send. If omitted or if INPUT is -, the client reads from stdin.:" << std::endl;
    std::cout << "\t-o OUTPUT  -- the output file to create/overwrite. If omitted or if OUTPUT is -, the server writes the received data to stdout." << std::endl;
    std::cout << "\t-w TIMEOUT -- timeout in whole seconds. Default value 1." << std::endl << std::endl;

    std::cout << "Exacly one of the options -s and -c must be specified." << std::endl;
}

void Params::parseArgs(int argc, char **argv)
{
    if (argc < 2)
    {
        printHelp();
        throw EX_USAGE;
    }

    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help")
        {
            printHelp();
            throw EX_OK;
        }
        else if (arg == "-s")
        {
            if (isServer != -1)
            {
                std::cerr << "Error: Both -s and -c options specified." << std::endl;
                throw EX_USAGE;
            }
            isServer = 1;
        }
        else if (arg == "-c")
        {
            if (isServer != -1)
            {
                std::cerr << "Error: Both -s and -c options specified." << std::endl;
                throw EX_USAGE;
            }
            isServer = 0;
        }
        else if (arg == "-p")
        {
            if (i + 1 >= argc)
            {
                std::cerr << "Error: Missing argument for -p option." << std::endl;
                throw EX_USAGE;
            }
            portUDP = std::stoi(argv[++i]);
        }
        else if (arg == "-a")
        {
            if (i + 1 >= argc)
            {
                std::cerr << "Error: Missing argument for -a option." << std::endl;
                throw EX_USAGE;
            }
            address = argv[++i];
        }
        else if (arg == "-i")
        {
            if (i + 1 >= argc)
            {
                std::cerr << "Error: Missing argument for -i option." << std::endl;
                throw EX_USAGE;
            }
            arg = argv[++i];
            if (arg == "-")
            {
                file = "stdin";
            }
            else
            {
                file = arg;
            }
        }
        else if (arg == "-o")
        {
            if (i + 1 >= argc)
            {
                std::cerr << "Error: Missing argument for -o option." << std::endl;
                throw EX_USAGE;
            }
            arg = argv[++i];
            if (arg == "-")
            {
                file = "stdout";
            }
            else
            {
                file = arg;
            }
        }
        else if (arg == "-w")
        {
            if (i + 1 >= argc)
            {
                std::cerr << "Error: Missing argument for -w option." << std::endl;
                throw EX_USAGE;
            }
            timeout = std::stoi(argv[++i]);
        }
        else
        {
            std::cerr << "Error: Unknown option " << arg << "." << std::endl;
            throw EX_USAGE;
        }
    }

    if (file == "")
    {
        if (isServer == 1)
        {
            file = "stdout";
        }
        else if (isServer == 0)
        {
            file = "stdin";
        }
    }
}
