#include "packets.hpp"

Packet::Packet()
{
    this->header = {MessageType::NONE, 0, 0, 0};
    this->data.reserve(MAX_DATA_SIZE);
}

void Packet::setPacket(const MessageType type, const uint32_t id, const std::string& data)
{
    if (data.size() > MAX_DATA_SIZE)
    {
        std::cerr << "Error: Data size exceeds maximum allowed size of " << MAX_DATA_SIZE << " bytes.\n";
        throw EX_DATAERR;
    }

    this->header = {type, id, 0, static_cast<uint16_t>(data.size())};
    this->data = data;
    this->header.checkSum = this->checkSum(this->serialize());
}

uint32_t calculateChecksum(const std::string& data)
{
    uint32_t checksum = 0;
    for (char byte : data)
    {
        checksum += static_cast<uint8_t>(byte);
    }
    return checksum;
}

uint16_t Packet::checkSum(const std::string& data)
{
    const auto* bytes = reinterpret_cast<const uint8_t*>(data.data());

    size_t length = data.size();
    uint32_t sum = 0;

    while (length > 1)
    {
        sum += static_cast<uint16_t>((bytes[0] << 8) | bytes[1]);;
        bytes += 2;
        length -= 2;
    }

    if (length == 1)
        sum += static_cast<uint16_t>(bytes[0] << 8);

    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);

    return static_cast<uint16_t>(~sum);
}

std::string Packet::serialize() const
{
    std::string rawData(sizeof(PacketHeader) + this->data.size(), '\0');
    std::memcpy(rawData.data(), &this->header, sizeof(PacketHeader));
    std::memcpy(rawData.data() + sizeof(PacketHeader), this->data.data(), this->data.size());
    return rawData;
}

void Packet::deserialize(const std::string& rawData)
{
    if (rawData.size() < sizeof(PacketHeader))
        throw EX_DATAERR;

    std::memcpy(&this->header, rawData.data(), sizeof(PacketHeader));
    this->data = rawData.substr(sizeof(PacketHeader), this->header.dataSize);
}