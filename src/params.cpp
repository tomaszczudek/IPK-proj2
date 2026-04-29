#include "params.hpp"

/**
 * Constructor for the Params class.
 */
Params::Params()
{
    this->address = "";
    this->file = "";
    this->isServer = UNDEFINED;
    this->portUDP = -1;
    this->timeout = 1;
}

/**
 * Prints the help message for the application.
 */
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

/**
 * Parses the command-line arguments.
 * @param argc The number of command-line arguments.
 * @param argv The array of command-line arguments.
 */
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
            if (this->isServer != UNDEFINED)
            {
                std::cerr << "Error: Both -s and -c options specified." << std::endl;
                throw EX_USAGE;
            }
            this->isServer = SERVER;
        }
        else if (arg == "-c")
        {
            if (this->isServer != UNDEFINED)
            {
                std::cerr << "Error: Both -s and -c options specified." << std::endl;
                throw EX_USAGE;
            }
            this->isServer = CLIENT;
        }
        else if (arg == "-p")
        {
            if (i + 1 >= argc)
            {
                std::cerr << "Error: Missing argument for -p option." << std::endl;
                throw EX_USAGE;
            }
            this->portUDP = std::stoi(argv[++i]);
        }
        else if (arg == "-a")
        {
            if (i + 1 >= argc)
            {
                std::cerr << "Error: Missing argument for -a option." << std::endl;
                throw EX_USAGE;
            }
            this->address = argv[++i];
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
                this->file = "-";
            }
            else
            {
                this->file = arg;
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
                this->file = "-";
            }
            else
            {
                this->file = arg;
            }
        }
        else if (arg == "-w")
        {
            if (i + 1 >= argc)
            {
                std::cerr << "Error: Missing argument for -w option." << std::endl;
                throw EX_USAGE;
            }
            this->timeout = std::stoi(argv[++i]);
        }
        else
        {
            std::cerr << "Error: Unknown option " << arg << "." << std::endl;
            throw EX_USAGE;
        }
    }
}

/**
 * Validates the parsed parameters.
 */
void Params::validate()
{
    if (this->isServer == UNDEFINED)
    {
        std::cerr << "Error: Either -s or -c option must be specified." << std::endl;
        throw EX_USAGE;
    }

    if (this->isServer == SERVER && this->portUDP == -1)
    {
        std::cerr << "Error: -p option must be specified in server mode." << std::endl;
        throw EX_USAGE;
    }

    if (this->isServer == CLIENT && (this->portUDP == -1 || this->address == ""))
    {
        std::cerr << "Error: Both -p and -a options must be specified in client mode." << std::endl;
        throw EX_USAGE;
    }

    if (this->file == "")
        this->file = "-";

    if (this->portUDP <= 0 || this->portUDP > 65535)
    {
        std::cerr << "Error: Invalid port number." << std::endl;
        throw EX_USAGE;
    }
}
