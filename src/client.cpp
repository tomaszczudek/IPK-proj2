#include "client.hpp"

Client::Client()
{

}

void Client::openInput(const std::string& filename)
{
    if (filename == "-")
    {
        this->input = &std::cin;
        return;
    }
    else
    {
        this->inputFile.open(filename, std::ios::binary);
        if (!this->inputFile.is_open())
        {
            std::cerr << "Error: Could not open input file: " << filename << std::endl;
            throw EX_OSERR;
        }
        this->input = &this->inputFile;
        return;
    }
}

void Client::readData()
{
    std::vector<char> buffer(MAX_DATA_SIZE);

    if (this->input == nullptr) {
        std::cerr << "Input is not open\n";
        throw EX_OSERR;
    }

    this->input->read(buffer.data(), buffer.size());

    std::streamsize bytesRead = this->input->gcount();

    if (bytesRead <= 0)
        throw EX_OSERR;
    
    buffer.resize(static_cast<std::size_t>(bytesRead));
}

void Client::setUpClient()
{

}