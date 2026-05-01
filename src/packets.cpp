#include "packets.hpp"

/**
 * Constructor for the Packet class.
 */
Packet::Packet()
{
    this->header.connectionId = 0;
    this->header.packetId = 0;
    this->header.checkSum = 0;
    this->header.dataSize = 0;
    this->header.type = MessageType::NONE;
    
    this->data.reserve(MAX_DATA_SIZE);
}

/**
 * Sets the properties of the packet.
 * @param connectionId The unique identifier for the connection.
 * @param type The type of the message.
 * @param id The unique identifier for the packet.
 * @param data The data of the packet.
 */
void Packet::setPacket(const uint32_t connectionId, const MessageType type, const uint32_t id, const std::string& data)
{
    if (data.size() > MAX_DATA_SIZE)
    {
        std::cerr << "Error: Data size exceeds maximum allowed size of " << MAX_DATA_SIZE << " bytes.\n";
        throw EX_DATAERR;
    }

    this->header.connectionId = connectionId;
    this->header.packetId = id;
    this->header.checkSum = 0;
    this->header.dataSize = static_cast<std::uint16_t>(data.size());
    this->header.type = type;

    this->data = data;
    this->header.checkSum = this->checkSum(this->serialize());
}

/**
 * Calculates the checksum for the given data.
 * @param data The data for which to calculate the checksum.
 * @return The calculated checksum.
 */
uint16_t Packet::checkSum(const std::string& data)
{
    const auto* bytes = reinterpret_cast<const uint8_t*>(data.data());

    size_t length = data.size();
    uint32_t sum = 0;

    while (length > 1)
    {
        sum += static_cast<uint16_t>((bytes[0] << 8) | bytes[1]);
        bytes += 2;
        length -= 2;
    }

    if (length == 1)
        sum += static_cast<uint16_t>(bytes[0] << 8);

    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);

    return static_cast<uint16_t>(~sum);
}

/**
 * Invalidates the packet, marking it as invalid and clearing its data.
 */
void Packet::invalidate()
{
    this->header.connectionId = 0;
    this->header.packetId = 0;
    this->header.checkSum = 0;
    this->header.dataSize = 0;
    this->header.type = MessageType::INVALID;

    this->data.clear();
}

/**
 * Serializes the packet into a string format for transmission.
 * @return The serialized packet as a string.
 */
std::string Packet::serialize() const
{
    if (this->data.size() > MAX_DATA_SIZE)
    {
        std::cerr << "Error: Packet payload too large.\n";
        throw EX_DATAERR;
    }

    if (this->header.dataSize != this->data.size())
    {
        std::cerr << "Error: Packet header dataSize does not match payload size.\n";
        throw EX_DATAERR;
    }

    std::string serializedData;
    serializedData.reserve(PACKET_HEADER_SIZE + this->data.size());

    // Serialize header
    serializedData.append(reinterpret_cast<const char*>(&this->header.connectionId), sizeof(this->header.connectionId));
    serializedData.append(reinterpret_cast<const char*>(&this->header.packetId), sizeof(this->header.packetId));
    serializedData.append(reinterpret_cast<const char*>(&this->header.checkSum), sizeof(this->header.checkSum));
    serializedData.append(reinterpret_cast<const char*>(&this->header.dataSize), sizeof(this->header.dataSize));
    uint8_t typeByte = static_cast<uint8_t>(this->header.type);
    serializedData.append(reinterpret_cast<const char*>(&typeByte), sizeof(typeByte));

    // Serialize data
    serializedData.append(this->data);

    if (serializedData.size() > MAX_PACKET_SIZE)
    {
        std::cerr << "Error: Serialized packet exceeds maximum allowed size.\n";
        throw EX_DATAERR;
    }

    return serializedData;
}

/**
 * Deserializes a string into a packet, extracting the header and data.
 * @param serializedData The data string to deserialize.
 */
void Packet::deserialize(const std::string& serializedData)
{
    if (serializedData.size() < PACKET_HEADER_SIZE)
    {
        std::cerr << "Error: Data buffer too small to contain a valid packet header.\n";
        this->invalidate();
        return;
    }

    if (serializedData.size() > MAX_PACKET_SIZE)
    {
        std::cerr << "Error: Data buffer exceeds maximum allowed packet size.\n";
        this->invalidate();
        return;
    }

    int offset = 0;
    std::memcpy(&this->header.connectionId, serializedData.data(), sizeof(this->header.connectionId));
    offset += sizeof(this->header.connectionId);

    std::memcpy(&this->header.packetId, serializedData.data() + offset, sizeof(this->header.packetId));
    offset += sizeof(this->header.packetId);

    std::memcpy(&this->header.checkSum, serializedData.data() + offset, sizeof(this->header.checkSum));
    offset += sizeof(this->header.checkSum);

    std::memcpy(&this->header.dataSize, serializedData.data() + offset, sizeof(this->header.dataSize));
    offset += sizeof(this->header.dataSize);

    uint8_t messageType = 0;
    std::memcpy(&messageType, serializedData.data() + offset, sizeof(messageType));
    this->header.type = static_cast<MessageType>(messageType);

    if (messageType > static_cast<uint8_t>(MessageType::CONFIRM_DATA))
    {
        std::cerr << "Error: Invalid message type in packet header.\n";
        this->invalidate();
        return;
    }

    if (this->header.dataSize > MAX_DATA_SIZE)
    {
        std::cerr << "Error: Packet data size exceeds maximum allowed size.\n";
        this->invalidate();
        return;
    }

    if (this->header.dataSize != (serializedData.size() - PACKET_HEADER_SIZE))
    {
        std::cerr << "Error: Packet data size does not match received data size.\n";
        this->invalidate();
        return;
    }

    std::uint16_t receivedChecksum = this->header.checkSum;

    this->header.checkSum = 0;
    this->data = serializedData.substr(PACKET_HEADER_SIZE, this->header.dataSize);

    if (receivedChecksum != this->checkSum(this->serialize()))
    {
        std::cerr << "Error: Invalid packet checksum.\n";
        this->invalidate();
        return;
    }

    this->header.checkSum = receivedChecksum;
}
