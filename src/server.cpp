#include "server.hpp"
#include <map>

/**
 * Constructor for the Server class.
 * @param params Parameters for the server.
 */
Server::Server(Params params) : params(params)
{
    this->output = nullptr;
    this->connectionId = 0;
}

/**
 * Opens the output stream (file or stdout).
 */
void Server::openOutput()
{
    if (this->params.getFile().empty() || this->params.getFile() == "-")
    {
        this->output = &std::cout;
        return;
    }

    this->outputFile.open(this->params.getFile(), std::ios::binary);

    if (!this->outputFile.is_open())
    {
        std::cerr << "Error: Could not open output file: " << this->params.getFile() << std::endl;
        throw EX_OSERR;
    }

    this->output = &this->outputFile;
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
    this->packet.setPacket(0, MessageType::NONE, 0, "");

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
    this->packet.setPacket(this->connectionId, type, id, data);
    if (!this->network.sendMessage(this->packet.serialize()))
    {
        std::cerr << "Error: Failed to send message.\n";
        throw EX_OSERR;
    }
}

/**
 * Sends a message through the network twice.
 * @param type Type of the message.
 * @param id ID of the message.
 */
void Server::sendMessage(MessageType type, uint32_t id)
{
    this->packet.setPacket(connectionId, type, id, "");

    for (int i = 0; i < 2; i++)
    {
        if (!this->network.sendMessage(this->packet.serialize()))
        {
            std::cerr << "Error: Failed to send message.\n";
            throw EX_OSERR;
        }
    }
}

/**
 * Starts the server-side FSM to handle the communication.
 */
void Server::startServer()
{
    this->initServer();

    int timeoutMs = this->params.getTimeout() * 1000;
    auto checkpoint = std::chrono::steady_clock::now();

    // Lambda function to check if the timeout has expired based on the checkpoint
    auto hasTimedout = [&]()
    {
        auto timeNow = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(timeNow - checkpoint).count();
        return elapsed >= timeoutMs;
    };

    FSMType fsmState = FSMType::WAIT_START;
    uint32_t expectedPacketId = 0;
    uint32_t endPacketId = 0;
    std::map<uint32_t, std::string> bufferedPackets;

    while (fsmState != FSMType::END)
    {
        // Wait for a message with a timeout
        bool timedOut = false;
        ReceivedMessage receivedMessage = this->network.receiveMessage(POLL_TIMEOUT_MS, timedOut);

        if (timedOut)
        {
            if (hasTimedout())
            {
                std::cerr << "Error: Timeout without protocol progress\n";
                throw EX_TEMPFAIL;
            }

            continue;
        }

        this->packet.deserialize(receivedMessage.data);

        if (!this->packet.isValid())
            continue;

        // Received a valid packet
        PacketHeader header = this->packet.getHeader();

        switch (fsmState)
        {
            case FSMType::WAIT_START:
            {
                if (header.type != MessageType::START)
                    continue;

                // Set connection ID and destination address
                this->connectionId = header.connectionId;
                this->network.setDestination(receivedMessage.address, receivedMessage.addressLength);
                
                // Send confirmation for START (CONFIRM_START) and move to WAIT_DATA state
                this->sendMessage(MessageType::CONFIRM_START, header.packetId);
                checkpoint = std::chrono::steady_clock::now();
                fsmState = FSMType::WAIT_DATA;
                break;
            }

            case FSMType::WAIT_DATA:
            {   
                // Check if the packet is from the same connection
                if (header.connectionId != this->connectionId)
                    continue;

                if (!this->network.isSameDestination(receivedMessage.address, receivedMessage.addressLength))
                    continue;

                // Reconfirm START if received again
                if (header.type == MessageType::START)
                {
                    this->sendMessage(MessageType::CONFIRM_START, header.packetId);
                    break;
                }

                // Received DATA packet
                if (header.type == MessageType::DATA)
                {
                    // Duplicate packet send CONFIRM_DATA again
                    if (header.packetId < expectedPacketId)
                    {
                        this->sendMessage(MessageType::CONFIRM_DATA, header.packetId);
                    }
                    // In-order packet send CONFIRM_DATA and write to output
                    else if (header.packetId == expectedPacketId)
                    {
                        this->writeData(this->packet.getData());
                        this->sendMessage(MessageType::CONFIRM_DATA, header.packetId);
                        expectedPacketId++;

                        // Update checkpoint and check for any other packets that are now in-order
                        checkpoint = std::chrono::steady_clock::now();
                        while (bufferedPackets.find(expectedPacketId) != bufferedPackets.end())
                        {
                            std::string data = bufferedPackets[expectedPacketId];
                            bufferedPackets.erase(expectedPacketId);

                            this->writeData(data);
                            this->sendMessage(MessageType::CONFIRM_DATA, expectedPacketId);

                            expectedPacketId++;
                            checkpoint = std::chrono::steady_clock::now();
                        }
                    }
                    // Out-of-order packet, store it if not already stored and send CONFIRM_DATA
                    else
                    {
                        if (bufferedPackets.find(header.packetId) == bufferedPackets.end())
                        {
                            bufferedPackets[header.packetId] = this->packet.getData();
                            checkpoint = std::chrono::steady_clock::now();
                        }

                        this->sendMessage(MessageType::CONFIRM_DATA, header.packetId);
                    }
                }
                // Receive END packet
                else if (header.type == MessageType::END)
                {
                    // Set final packet ID and send CONFIRM_END
                    endPacketId = header.packetId;
                    if (expectedPacketId == header.packetId)
                    {
                        this->sendMessage(MessageType::CONFIRM_END, endPacketId);
                        checkpoint = std::chrono::steady_clock::now();
                        fsmState = FSMType::END;
                    }
                }
                break;
            }

            case FSMType::END:
            default:
                break;
        }
    }
}
