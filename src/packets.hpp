#ifndef PACKETS_HPP
#define PACKETS_HPP

#define MAX_PACKET_SIZE 1200
#define MAX_DATA_SIZE (MAX_PACKET_SIZE-sizeof(PacketHeader))

#include <string>
#include <cstdint>
#include <sysexits.h>
#include <iostream>

enum class FSMType : short
{
    START,
    WAIT_CONFIRM,
    SEND_DATA,
    WAIT_CONFIRM_DATA,
    END,

    WAIT_START,
    WAIT_DATA,
};

enum class MessageType : short
{
    NONE,           //> No message type, initialization or invalid to be used
    INVALID,        //> Invalid message type, used for error handling
    // Base connection messages
    START,          //> Initial message to start the connection
    ERROR,          //> Error message for any issues during connection
    SIG_TERM,       //> Signal to terminate the connection
    CONFIRM,        //> Confirmation message(START, END)
    END,            //> End message

    // Data transfer messages
    DATA,           //> Message containing a chunk of data
    CONFIRM_DATA,   //> Message indicating valid data, CONFIRM response to DATA
    RESEND,         //> Message requesting data retransmission
};

struct PacketHeader
{
    uint32_t packetId;
    uint16_t checkSum;
    uint16_t dataSize;
    MessageType type;
};

class Packet
{
    private:
        PacketHeader header;
        std::string data;

    public:
        Packet();
        void setPacket(const MessageType type, const uint32_t id, const std::string& data);
        uint16_t checkSum(const std::string& data);
        std::string serialize() const;
        void deserialize(const std::string& rawData);

        // Getters and setters
        PacketHeader getHeader() const { return this->header; }
        std::string getData() const { return this->data; }
        MessageType getType() const { return this->header.type; }
        void setHeader(PacketHeader h) { this->header = h; }
        void setData(const std::string &d) { this->data = d; }
        void setType(MessageType type) { this->header.type = type; }
        
};

#endif // PACKETS_HPP
