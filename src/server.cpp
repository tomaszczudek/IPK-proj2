#include "server.hpp"

Server::Server()
{
    
}

void Server::openOutput(const std::string& filename)
{
    if (filename == "-")
    {
        this->output = &std::cout;
        return;
    }
    else
    {
        this->outputFile.open(filename, std::ios::binary);
        if (!this->outputFile.is_open())
        {
            std::cerr << "Error: Could not open output file: " << filename << std::endl;
            throw EX_OSERR;
        }
        this->output = &this->outputFile;
        return;
    }
}

void Server::writeData(const std::string& data)
{
    if (this->output == nullptr) {
        std::cerr << "Output is not open\n";
        throw EX_OSERR;
    }

    (*this->output) << data;
}

void Server::setUpServer()
{

}
