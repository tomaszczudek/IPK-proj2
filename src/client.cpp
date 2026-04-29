#include "client.hpp"
#include <map>

#define WINDOW_SIZE 16

/**
 * Constructor for the Client class.
 * @param params Parameters for the client.
 */
Client::Client(Params params)
{
    this->params = params;
}

/**
 * Opens the input stream (file or stdin).
 */
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

/**
 * Reads data from the input stream.
 * @return String containing the data if EOF is not reached, otherwise empty string.
 */
std::string Client::readData()
{
    std::vector<char> buffer(MAX_DATA_SIZE);

    if (this->input == nullptr)
    {
        std::cerr << "Error: Input is not open\n";
        throw EX_OSERR;
    }

    this->input->read(buffer.data(), buffer.size());
    std::streamsize bytesRead = this->input->gcount();

    if (bytesRead > 0)
        return std::string(buffer.data(), static_cast<std::size_t>(bytesRead));

    if (this->input->bad())
    {
        std::cerr << "Error: Failed to read from input\n";
        throw EX_OSERR;
    }

    return "";
}

/**
 * Initializes the client.
 * Sets up client, packet, and opens input stream.
 */
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

/**
 * Sends a message through the network.
 * @param type Type of the message.
 * @param id ID of the message.
 * @param data Data to be sent.
 */
void Client::sendMessage(MessageType type, uint32_t id, const std::string& data)
{
    this->packet.setPacket(type, id, data);
    if (!this->network.sendMessage(this->packet.serialize()))
    {
        std::cerr << "Error: Failed to send message.\n";
        throw EX_OSERR;
    }
}

/**
 * Starts the client-side FSM to handle the communication.
 */
void Client::startClient()
{
    this->initClient();

    FSMType fsmState = FSMType::START;
    std::string receivedData;
    std::string sentData;

    uint32_t nextPacketId = 0;
    bool inputEnded = false;
    std::map<uint32_t, std::string> unconfirmedPackets;

    while (fsmState != FSMType::END)
    {
        switch (fsmState)
        {
            case FSMType::START:
            {
                this->sendMessage(MessageType::START, 0, "");
                fsmState = FSMType::WAIT_CONFIRM_START;
                break;
            }

            case FSMType::WAIT_CONFIRM_START:
            {
                receivedData = this->network.receiveMessage();
                this->packet.deserialize(receivedData);

                if (this->packet.getType() == MessageType::CONFIRM)
                {
                    fsmState = FSMType::SEND_DATA;
                }
                else
                {
                    std::cerr << "Error: Expected CONFIRM message, received different type.\n";
                    fsmState = FSMType::START;
                }

                break;
            }

            case FSMType::SEND_DATA:
            {
                while (!inputEnded && unconfirmedPackets.size() < WINDOW_SIZE)
                {
                    sentData = this->readData();

                    if (sentData.empty())
                    {
                        inputEnded = true;
                        break;
                    }

                    this->sendMessage(MessageType::DATA, nextPacketId, sentData);
                    unconfirmedPackets[nextPacketId] = sentData;
                    nextPacketId++;
                }

                if (inputEnded && unconfirmedPackets.empty())
                {
                    this->sendMessage(MessageType::END, nextPacketId, "");
                    fsmState = FSMType::END;
                }
                else
                {
                    fsmState = FSMType::WAIT_CONFIRM_DATA;
                }

                break;
            }

            case FSMType::WAIT_CONFIRM_DATA:
            {
                receivedData = this->network.receiveMessage();
                this->packet.deserialize(receivedData);

                PacketHeader header = this->packet.getHeader();

                if (header.type == MessageType::CONFIRM_DATA)
                {
                    unconfirmedPackets.erase(header.packetId);
                    fsmState = FSMType::SEND_DATA;
                }
                #if 0 // Change to timeout-based resend mechanism
                else if (header.type == MessageType::RESEND)
                {
                    auto it = unconfirmedPackets.find(header.packetId);

                    if (it != unconfirmedPackets.end())
                    {
                        this->sendMessage(MessageType::DATA, it->first, it->second);
                    }

                    fsmState = FSMType::WAIT_CONFIRM_DATA;
                }
                #endif
                else
                {
                    std::cerr << "Error: Expected CONFIRM_DATA message, received different type.\n";
                    fsmState = FSMType::WAIT_CONFIRM_DATA;
                }

                break;
            }

            case FSMType::END:
            default:
                break;
        }
    }
}
