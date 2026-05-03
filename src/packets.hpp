#ifndef PACKETS_HPP
#define PACKETS_HPP

#define MAX_PACKET_SIZE 1200
#define PACKET_HEADER_SIZE 13
#define MAX_DATA_SIZE (MAX_PACKET_SIZE - PACKET_HEADER_SIZE)

#include <string>
#include <cstdint>
#include <sysexits.h>
#include <iostream>
#include <cstring>

/**
 * Enumeration for the finite state machine (FSM) states used in client/server communication. 
 */
enum class FSMType : uint8_t
{
    SEND_START = 0,         //> State for sending the initial START message
    WAIT_CONFIRM_START,     //> State for waiting for confirmation of the START message

    SEND_DATA,              //> State for sending DATA packets
    WAIT_CONFIRM_DATA,      //> State for waiting for confirmation of DATA packets

    SEND_END,               //> State for sending the END message
    WAIT_CONFIRM_END,       //> State for waiting for confirmation of the END message

    END,                    //> State indicating the end of the communication process

    // Server-side states
    WAIT_START,             //> State for waiting for the initial START message
    WAIT_DATA,              //> State for waiting for DATA packets
};

/**
 * Enumeration for the message types used in the communication protocol.
 */
enum class MessageType : uint8_t
{
    NONE = 0,       //> No message type, initialization or invalid to be used
    INVALID,        //> Local invalid message type, used for error handling

    START,          //> Initial message to start the connection
    CONFIRM_START,  //> Confirmation message for START
    END,            //> End message
    CONFIRM_END,    //> Confirmation message for END

    DATA,           //> Message containing data
    CONFIRM_DATA,   //> Message indicating valid data, CONFIRM response to DATA
};

/**
 * Structure representing the header of a packet.
 */
struct PacketHeader
{
    uint32_t connectionId;      //> Unique identifier for the connection
    uint32_t packetId;          //> Unique identifier for the packet
    uint16_t checkSum;          //> Checksum for data integrity verification
    uint16_t dataSize;          //> Size of the data
    MessageType type;           //> Type of the message contained in the packet
};

/**
 * Class representing a packet in the communication protocol.
 */
class Packet
{
    private:
        PacketHeader header;    //> Header containing metadata about the packet
        std::string data;       //> Payload of the packet, containing the actual file data

    public:
        /**
         * Constructor for the Packet class.
         */
        Packet();

        /**
         * Sets the properties of the packet.
         * @param connectionId The unique identifier for the connection.
         * @param type The type of the message.
         * @param id The unique identifier for the packet.
         * @param data The data of the packet.
         */
        void setPacket(const uint32_t connectionId, const MessageType type, const uint32_t id, const std::string& data);

        /**
         * Calculates the checksum for the given data.
         * @param data The data for which to calculate the checksum.
         * @return The calculated checksum.
         */
        uint16_t checkSum(const std::string& data);

        /**
         * Invalidates the packet, marking it as invalid and clearing its data.
         */
        void invalidate();

        /**
         * Serializes the packet into a string format for transmission.
         * @return The serialized packet as a string.
         */
        std::string serialize() const;

        /**
         * Deserializes a string into a packet, extracting the header and data.
         * @param data The data string to deserialize.
         */
        void deserialize(const std::string& data);
        
        /**
         * Checks if the packet is valid based on its type.
         * @return True if the packet is valid, false otherwise.
         */
        bool isValid() const { return this->header.type != MessageType::INVALID; }

        // Getters and setters
        PacketHeader getHeader() const { return this->header; }
        std::string getData() const { return this->data; }
        MessageType getType() const { return this->header.type; }
        void setHeader(PacketHeader h) { this->header = h; }
        void setData(const std::string &d) { this->data = d; }
        void setType(MessageType type) { this->header.type = type; }
        
};

#endif // PACKETS_HPP
