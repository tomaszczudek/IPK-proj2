#include "client.hpp"

Client::Client(Params params)
{
    this->params = params;
}

void Client::openInput()
{
    if (this->params.getFile() == "-")
    {
        this->input = &std::cin;
        return;
    }
    else
    {
        this->inputFile.open(this->params.getFile(), std::ios::binary);
        if (!this->inputFile.is_open())
        {
            std::cerr << "Error: Could not open input file: " << this->params.getFile() << std::endl;
            throw EX_OSERR;
        }
        this->input = &this->inputFile;
        return;
    }
}

std::string Client::readData()
{
    std::vector<char> buffer(MAX_DATA_SIZE);

    if (this->input == nullptr)
    {
        std::cerr << "Input is not open\n";
        throw EX_OSERR;
    }

    this->input->read(buffer.data(), buffer.size());
    std::streamsize bytesRead = this->input->gcount();

    if (bytesRead > 0)
        return std::string(buffer.data(), static_cast<std::size_t>(bytesRead));

    if (this->input->bad())
    {
        std::cerr << "Error while reading input\n";
        throw EX_OSERR;
    }

    return "";
}

void Client::initClient()
{
    this->openInput();
    this->network.setUpClient(this->params.getAddress(), this->params.getPortUDP());
    this->packet.setPacket(MessageType::NONE, 0, "");

    if (!this->network.isSocketSet())
    {
        std::cerr << "Error: Failed to create or configure client socket.\n";
        throw EX_OSERR;
    }
}

void Client::sendMessage(MessageType type, uint32_t id, const std::string& data)
{
    this->packet.setPacket(type, id, data);
    if (!this->network.sendMessage(this->packet.serialize()))
    {
        std::cerr << "Error: Failed to send message.\n";
        throw EX_OSERR;
    }
}

void Client::startClient()
{
    this->initClient();

    uint8_t fsmState = FSM::START;
    std::string receivedData;

    while (true)
    {
        switch (fsmState)
        {
            case FSM::START:
                this->sendMessage(MessageType::START, 0, "");
                fsmState = FSM::WAIT_CONFIRM;
                break;
            
            case FSM::WAIT_CONFIRM:
                receivedData = this->network.receiveMessage();
                break;

            case FSM::SEND_DATA:
                break;

            case FSM::WAIT_CONFIRM_DATA:
                break;

            case FSM::END:
                break;

            default:
                break;
        }
    }
}