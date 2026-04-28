#include "server.hpp"

Server::Server(Params params)
{
    this->params = params;
}

void Server::openOutput()
{
    if (this->params.getFile() == "-")
    {
        this->output = &std::cout;
        return;
    }
    else
    {
        this->outputFile.open(this->params.getFile(), std::ios::binary);
        if (!this->outputFile.is_open())
        {
            std::cerr << "Error: Could not open output file: " << this->params.getFile() << std::endl;
            throw EX_OSERR;
        }
        this->output = &this->outputFile;
        return;
    }
}

void Server::writeData(const std::string& data)
{
    if (this->output == nullptr)
    {
        std::cerr << "Output is not open\n";
        throw EX_OSERR;
    }

    this->output->write(data.data(), data.size());

    if (!(*this->output))
    {
        std::cerr << "Output write failed" << std::endl;
        throw EX_OSERR;
    }
}

void Server::initServer()
{
    this->openOutput();
    this->network.setUpServer(this->params.getAddress(), this->params.getPortUDP());
    this->packet.setPacket(MessageType::NONE, 0, "");

    if (!this->network.isSocketSet())
    {
        std::cerr << "Error: Failed to create or configure client socket.\n";
        throw EX_OSERR;
    }
}

void Server::sendMessage(MessageType type, uint32_t id, const std::string& data)
{
    this->packet.setPacket(type, id, data);
    if (!this->network.sendMessage(this->packet.serialize()))
    {
        std::cerr << "Error: Failed to send message.\n";
        throw EX_OSERR;
    }
}

void Server::startServer()
{
    this->initServer();
    uint8_t fsmState = FSM::WAIT_START;

    while(fsmState != FSM::END)
    {
        switch (fsmState)
        {
            case FSM::WAIT_START:
                break;

            case FSM::WAIT_DATA:
                break;

            default:
                break;
        }
    }
}