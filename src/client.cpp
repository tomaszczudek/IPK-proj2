#include "client.hpp"
#include <map>
#include <random>
#include <vector>

/**
 * Constructor for the Client class.
 * @param params Parameters for the client.
 */
Client::Client(const Params params) : params(params)
{
    this->input = nullptr;
    this->connectionId = this->generateConnectionId();
}

/**
 * Generates a random connection ID.
 * @return A random connection ID.
 */
std::uint32_t Client::generateConnectionId()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<std::uint32_t> dist(1, UINT32_MAX);
    return dist(gen);
}

/**
 * Opens the input stream (file or stdin).
 */
void Client::openInput()
{
    if (this->params.getFile().empty() || this->params.getFile() == "-")
    {
        this->input = &std::cin;
        return;
    }

    this->inputFile.open(this->params.getFile(), std::ios::binary);

    if (!this->inputFile.is_open())
    {
        std::cerr << "Error: Could not open input file: " << this->params.getFile() << std::endl;
        throw EX_OSERR;
    }

    this->input = &this->inputFile;
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
    this->packet.setPacket(this->connectionId, MessageType::NONE, 0, "");

    if (!this->network.isSocketSet())
    {
        std::cerr << "Error: Failed to create or configure client socket.\n";
        throw EX_OSERR;
    }
}

/**
 * Sends a message through the network.
 * @param connectionId The unique identifier for the connection.
 * @param type Type of the message.
 * @param id ID of the message.
 * @param data Data to be sent.
 */
void Client::sendMessage(const uint32_t connectionId, const MessageType type, const uint32_t id, const std::string& data)
{
    this->packet.setPacket(connectionId, type, id, data);
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

    int timeoutMs = this->params.getTimeout() * 1000;
    auto checkpoint = std::chrono::steady_clock::now();

    // Lambda function to check if the timeout has expired based on the checkpoint
    auto hasTimedout = [&]()
    {
        auto timeNow = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(timeNow - checkpoint).count();
        return elapsed >= timeoutMs;
    };

    FSMType fsmState = FSMType::SEND_START;
    uint32_t nextPacketId = 0;
    bool reachedEOF = false;
    std::map<uint32_t, SentPacketsData> unconfirmedPackets;

    auto timeEndSent = std::chrono::steady_clock::now();

    while (fsmState != FSMType::END)
    {
        switch (fsmState)
        {
            case FSMType::SEND_START:
            {
                // Send START message and move to WAIT_CONFIRM_START state
                this->sendMessage(this->connectionId, MessageType::START, 0, "");
                fsmState = FSMType::WAIT_CONFIRM_START;
                break;
            }

            case FSMType::WAIT_CONFIRM_START:
            {
                // Wait for message
                bool timedOut = false;
                ReceivedMessage receivedMessage = this->network.receiveMessage(RETRANSMIT_MS, timedOut);

                if (timedOut)
                {
                    if (hasTimedout())
                    {
                        std::cerr << "Error: Timeout while waiting for START confirmation\n";
                        throw EX_TEMPFAIL;
                    }

                    fsmState = FSMType::SEND_START;
                    break;
                }

                this->packet.deserialize(receivedMessage.data);
                if (!this->packet.isValid())
                    break;

                // Check if it's a valid CONFIRM_START for this connection and move to SEND_DATA state
                PacketHeader header = this->packet.getHeader();
                if (header.type == MessageType::CONFIRM_START && header.connectionId == this->connectionId)
                {
                    checkpoint = std::chrono::steady_clock::now();
                    fsmState = FSMType::SEND_DATA;
                }

                break;
            }

            case FSMType::SEND_DATA:
            {
                // Send packets until window is full or EOF is reached
                while (!reachedEOF && unconfirmedPackets.size() < WINDOW_SIZE)
                {
                    std::string data = this->readData();

                    if (data.empty())
                    {
                        reachedEOF = true;
                        break;
                    }

                    this->sendMessage(this->connectionId, MessageType::DATA, nextPacketId, data);
                    unconfirmedPackets[nextPacketId] = {data, std::chrono::steady_clock::now()};
                    nextPacketId++;
                }

                // If thera are no data to be sent SEND_END, otherwise wait for confirmations
                if (reachedEOF && unconfirmedPackets.empty())
                    fsmState = FSMType::SEND_END;
                else
                    fsmState = FSMType::WAIT_CONFIRM_DATA;

                break;
            }

            case FSMType::WAIT_CONFIRM_DATA:
            {
                // Resend unconfirmed packets
                auto timeNow = std::chrono::steady_clock::now();
                for (auto& item : unconfirmedPackets)
                {
                    uint32_t packetId = item.first;
                    SentPacketsData& sentPacket = item.second;

                    auto time = std::chrono::duration_cast<std::chrono::milliseconds>(timeNow - sentPacket.lastSent).count();

                    if (time >= RETRANSMIT_MS)
                    {
                        this->sendMessage(this->connectionId, MessageType::DATA, packetId, sentPacket.data);
                        sentPacket.lastSent = std::chrono::steady_clock::now();
                    }
                }

                // Wait for message and go to SEND_DATA if not timed out
                bool timedOut = false;
                ReceivedMessage receivedMessage = this->network.receiveMessage(POLL_TIMEOUT_MS, timedOut);

                if (!timedOut)
                {
                    this->packet.deserialize(receivedMessage.data);

                    if (this->packet.isValid())
                    {
                        PacketHeader header = this->packet.getHeader();

                        if (header.type == MessageType::CONFIRM_DATA && header.connectionId == this->connectionId)
                        {
                            std::size_t deleted = unconfirmedPackets.erase(header.packetId);

                            if (deleted > 0)
                                checkpoint = std::chrono::steady_clock::now();
                        }
                    }
                }

                if (hasTimedout())
                {
                    std::cerr << "Error: Timeout during data transfer\n";
                    throw EX_TEMPFAIL;
                }

                fsmState = FSMType::SEND_DATA;
                break;
            }

            case FSMType::SEND_END:
            {
                // Send END message and move to WAIT_CONFIRM_END state
                this->sendMessage(this->connectionId, MessageType::END, nextPacketId, "");

                timeEndSent = std::chrono::steady_clock::now();
                fsmState = FSMType::WAIT_CONFIRM_END;
                break;
            }

            case FSMType::WAIT_CONFIRM_END:
            {
                auto timeNow = std::chrono::steady_clock::now();
                auto timeoutTime = std::chrono::duration_cast<std::chrono::milliseconds>(timeNow - timeEndSent).count();

                // If SEND_END has timed out, move to SEND_END state to resend END message
                if (timeoutTime >= RETRANSMIT_MS)
                {
                    fsmState = FSMType::SEND_END;
                    break;
                }

                // Wait for message
                bool timedOut = false;
                ReceivedMessage receivedMessage = this->network.receiveMessage(POLL_TIMEOUT_MS, timedOut);

                if (!timedOut)
                {
                    this->packet.deserialize(receivedMessage.data);

                    if (!this->packet.isValid())
                        break;

                    PacketHeader header = this->packet.getHeader();
                    
                    // Check if it's a valid CONFIRM_END for this connection and move to END state
                    if (header.connectionId == this->connectionId && header.type == MessageType::CONFIRM_END && header.packetId == nextPacketId)
                    {
                        checkpoint = std::chrono::steady_clock::now();
                        fsmState = FSMType::END;
                        break;
                    }
                }

                if (hasTimedout())
                {
                    std::cerr << "Error: Timeout while waiting for END confirmation\n";
                    throw EX_TEMPFAIL;
                }

                break;
            }

            default:
            {
                break;
            }
        }
    }
}
