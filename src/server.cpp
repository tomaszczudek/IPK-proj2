#include "server.hpp"
#include <map>

/**
 * Constructor for the Server class.
 * @param params Parameters for the server.
 */
Server::Server(Params params)
{
    this->params = params;
}

/**
 * Opens the output stream (file or stdout).
 */
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

/**
 * Writes data to the output stream.
 * @param data The data to write.
 */
void Server::writeData(const std::string& data)
{
    if (this->output == nullptr)
    {
        std::cerr << "Error: Output is not open\n";
        throw EX_OSERR;
    }

    this->output->write(data.data(), data.size());

    if (!(*this->output))
    {
        std::cerr << "Error: Output write failed" << std::endl;
        throw EX_OSERR;
    }
}

/**
 * Initializes the server.
 * Sets up server, packet, and opens output stream.
 */
void Server::initServer()
{
    this->openOutput();
    this->network.setUpServer(this->params.getAddress(), this->params.getPortUDP());
    this->packet.setPacket(MessageType::NONE, 0, "");

    if (!this->network.isSocketSet())
    {
        std::cerr << "Error: Failed to create or configure server socket.\n";
        throw EX_OSERR;
    }
}

/**
 * Sends a message through the network.
 * @param type Type of the message.
 * @param id ID of the message.
 * @param data Data to be sent.
 */
void Server::sendMessage(MessageType type, uint32_t id, const std::string& data)
{
    this->packet.setPacket(type, id, data);
    if (!this->network.sendMessage(this->packet.serialize()))
    {
        std::cerr << "Error: Failed to send message.\n";
        throw EX_OSERR;
    }
}

/**
 * Starts the server-side FSM to handle the communication.
 */
void Server::startServer()
{
    this->initServer();

    FSMType fsmState = FSMType::WAIT_START;
    std::string receivedData;

    uint32_t expectedPacketId = 0;
    bool endReceived = false;
    uint32_t endPacketId = 0;
    std::map<uint32_t, std::string> bufferedPackets;

    while (fsmState != FSMType::END)
    {
        switch (fsmState)
        {
            case FSMType::WAIT_START:
            {
                receivedData = this->network.receiveMessage();
                this->packet.deserialize(receivedData);

                if (this->packet.getHeader().type == MessageType::START)
                {
                    this->sendMessage(MessageType::CONFIRM, this->packet.getHeader().packetId, "");
                    fsmState = FSMType::WAIT_DATA;
                }

                break;
            }

            case FSMType::WAIT_DATA:
            {
                receivedData = this->network.receiveMessage();
                this->packet.deserialize(receivedData);

                PacketHeader header = this->packet.getHeader();

                if (header.type == MessageType::DATA)
                {
                    if (header.packetId < expectedPacketId)
                    {
                        this->sendMessage(MessageType::CONFIRM_DATA, header.packetId, "");
                    }
                    else if (header.packetId == expectedPacketId)
                    {
                        this->writeData(this->packet.getData());
                        this->sendMessage(MessageType::CONFIRM_DATA, header.packetId, "");
                        expectedPacketId++;

                        while (bufferedPackets.find(expectedPacketId) != bufferedPackets.end())
                        {
                            std::string data = bufferedPackets[expectedPacketId];
                            bufferedPackets.erase(expectedPacketId);

                            this->writeData(data);
                            this->sendMessage(MessageType::CONFIRM_DATA, expectedPacketId, "");
                            expectedPacketId++;
                        }
                    }
                    else
                    {
                        bufferedPackets[header.packetId] = this->packet.getData();
                        this->sendMessage(MessageType::CONFIRM_DATA, header.packetId, "");
                    }
                }
                else if (header.type == MessageType::END)
                {
                    endReceived = true;
                    endPacketId = header.packetId;
                }

                if (endReceived && expectedPacketId == endPacketId)
                {
                    this->sendMessage(MessageType::CONFIRM, endPacketId, "");
                    fsmState = FSMType::END;
                }

                break;
            }

            case FSMType::END:
            default:
                break;
        }
    }
}
