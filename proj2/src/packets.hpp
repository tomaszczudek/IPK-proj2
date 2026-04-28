#ifndef PACKETS_HPP
#define PACKETS_HPP
#include <string>

class BasePacket
{
    std::string data;
};

class Sender : BasePacket
{

};

class Receiver : BasePacket
{

};
#endif // PACKETS_HPP
