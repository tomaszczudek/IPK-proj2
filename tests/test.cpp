/**
 * Při tvorbě této testovací sady byl využit ChatGPT jako pomocný nástroj.
 */

#include <gtest/gtest.h>

#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <netinet/in.h>
#include <string>
#include <sys/wait.h>
#include <sysexits.h>
#include <unistd.h>
#include <vector>

#include "../src/packets.cpp"
#include "../src/params.cpp"
#include "../src/network.cpp"

namespace
{
    void parseAndValidate(Params& params, std::initializer_list<const char*> args)
    {
        std::vector<char*> argv;
        argv.reserve(args.size());
        for (const char* arg : args)
            argv.push_back(const_cast<char*>(arg));

        params.parseArgs(static_cast<int>(argv.size()), argv.data());
        params.validate();
    }

    int exitCodeFromSystem(int status)
    {
        if (status == -1)
            return -1;

        if (WIFEXITED(status))
            return WEXITSTATUS(status);

        return -1;
    }

    bool projectBinaryExists()
    {
        return access("./ipk-rdt", X_OK) == 0;
    }
} // namespace

// Packet tests
TEST(PacketTest, packetConstructor)
{
    Packet packet;
    PacketHeader header = packet.getHeader();

    EXPECT_EQ(header.connectionId, 0u);
    EXPECT_EQ(header.packetId, 0u);
    EXPECT_EQ(header.checkSum, 0u);
    EXPECT_EQ(header.dataSize, 0u);
    EXPECT_EQ(header.type, MessageType::NONE);
    EXPECT_EQ(packet.getData(), "");
    EXPECT_TRUE(packet.isValid());
}

TEST(PacketTest, reliableSerializationDeserialization)
{
    Packet packet;
    packet.setPacket(42, MessageType::DATA, 7, "hello world");

    const std::string serialized = packet.serialize();
    ASSERT_EQ(serialized.size(), PACKET_HEADER_SIZE + std::string("hello world").size());

    Packet parsed;
    parsed.deserialize(serialized);

    ASSERT_TRUE(parsed.isValid());
    EXPECT_EQ(parsed.getHeader().connectionId, 42u);
    EXPECT_EQ(parsed.getHeader().packetId, 7u);
    EXPECT_EQ(parsed.getHeader().dataSize, std::string("hello world").size());
    EXPECT_EQ(parsed.getHeader().type, MessageType::DATA);
    EXPECT_EQ(parsed.getData(), "hello world");
}

TEST(PacketTest, emptyPacketSerializationDeserialization)
{
    Packet packet;
    packet.setPacket(1234, MessageType::START, 0, "");

    Packet parsed;
    parsed.deserialize(packet.serialize());

    ASSERT_TRUE(parsed.isValid());
    EXPECT_EQ(parsed.getHeader().connectionId, 1234u);
    EXPECT_EQ(parsed.getHeader().packetId, 0u);
    EXPECT_EQ(parsed.getHeader().dataSize, 0u);
    EXPECT_EQ(parsed.getHeader().type, MessageType::START);
    EXPECT_TRUE(parsed.getData().empty());
}

TEST(PacketTest, dataTooLargeForPacket)
{
    Packet packet;
    std::string tooLarge(MAX_DATA_SIZE + 1, 'x');

    EXPECT_THROW(packet.setPacket(1, MessageType::DATA, 0, tooLarge), int);
}

TEST(PacketTest, headerTooShortForPacket)
{
    Packet packet;
    packet.deserialize("short");

    EXPECT_FALSE(packet.isValid());
    EXPECT_EQ(packet.getType(), MessageType::INVALID);
}

TEST(PacketTest, corruptedChecksumInvalidatesPacket)
{
    Packet packet;
    packet.setPacket(77, MessageType::DATA, 3, "abcdef");

    std::string serialized = packet.serialize();
    serialized.back() = serialized.back() == 'x' ? 'y' : 'x';

    Packet parsed;
    parsed.deserialize(serialized);

    EXPECT_FALSE(parsed.isValid());
    EXPECT_EQ(parsed.getType(), MessageType::INVALID);
}

TEST(PacketTest, invalidMessageType)
{
    Packet packet;
    packet.setPacket(1, MessageType::DATA, 0, "payload");

    std::string serialized = packet.serialize();
    serialized[12] = static_cast<char>(255);

    Packet parsed;
    parsed.deserialize(serialized);

    EXPECT_FALSE(parsed.isValid());
    EXPECT_EQ(parsed.getType(), MessageType::INVALID);
}

TEST(PacketTest, headerSizeMismatch)
{
    Packet packet;
    packet.setPacket(1, MessageType::DATA, 0, "abc");

    PacketHeader header = packet.getHeader();
    header.dataSize = 100;
    packet.setHeader(header);

    EXPECT_THROW(packet.serialize(), int);
}

// Params tests
TEST(ParamsTest, validClientArguments)
{
    Params params;
    parseAndValidate(params, {"ipk-rdt", "-c", "-p", "2024", "-a", "127.0.0.1", "-i", "input.bin", "-w", "3"});

    EXPECT_EQ(params.getIsServer(), CLIENT);
    EXPECT_EQ(params.getPortUDP(), 2024);
    EXPECT_EQ(params.getAddress(), "127.0.0.1");
    EXPECT_EQ(params.getFile(), "input.bin");
    EXPECT_FLOAT_EQ(params.getTimeout(), 3.0f);
}

TEST(ParamsTest, validServerArguments)
{
    Params params;
    parseAndValidate(params, {"ipk-rdt", "-s", "-p", "9999", "-a", "::1", "-o", "out.bin"});

    EXPECT_EQ(params.getIsServer(), SERVER);
    EXPECT_EQ(params.getPortUDP(), 9999);
    EXPECT_EQ(params.getAddress(), "::1");
    EXPECT_EQ(params.getFile(), "out.bin");
    EXPECT_FLOAT_EQ(params.getTimeout(), 1.0f);
}

TEST(ParamsTest, noFileSpecified)
{
    Params params;
    parseAndValidate(params, {"ipk-rdt", "-c", "-p", "2024", "-a", "localhost"});

    EXPECT_EQ(params.getFile(), "-");
}

TEST(ParamsTest, clientInputDash)
{
    Params params;
    parseAndValidate(params, {"ipk-rdt", "-c", "-p", "2024", "-a", "localhost", "-i", "-"});

    EXPECT_EQ(params.getFile(), "-");
}

TEST(ParamsTest, serverOutputDash)
{
    Params params;
    parseAndValidate(params, {"ipk-rdt", "-s", "-p", "2024", "-o", "-"});

    EXPECT_EQ(params.getFile(), "-");
}

TEST(ParamsTest, helpExitCode)
{
    Params params;
    std::vector<char*> argv = {const_cast<char*>("ipk-rdt"), const_cast<char*>("-h")};

    try
    {
        params.parseArgs(static_cast<int>(argv.size()), argv.data());
        FAIL() << "Expected help to throw EX_OK";
    }
    catch (int code)
    {
        EXPECT_EQ(code, EX_OK);
    }
}

TEST(ParamsTest, noModeSpecified)
{
    Params params;
    std::vector<char*> argv = {const_cast<char*>("ipk-rdt"), const_cast<char*>("-p"), const_cast<char*>("2024")};

    params.parseArgs(static_cast<int>(argv.size()), argv.data());
    EXPECT_THROW(params.validate(), int);
}

TEST(ParamsTest, bothModesSpecified)
{
    Params params;
    std::vector<char*> argv = {const_cast<char*>("ipk-rdt"), const_cast<char*>("-s"), const_cast<char*>("-c"), const_cast<char*>("-p"), const_cast<char*>("2024")};

    EXPECT_THROW(params.parseArgs(static_cast<int>(argv.size()), argv.data()), int);
}

TEST(ParamsTest, clientNoAddress)
{
    Params params;
    std::vector<char*> argv = {const_cast<char*>("ipk-rdt"), const_cast<char*>("-c"), const_cast<char*>("-p"), const_cast<char*>("2024")};

    params.parseArgs(static_cast<int>(argv.size()), argv.data());
    EXPECT_THROW(params.validate(), int);
}

TEST(ParamsTest, portOutOfRangeTooLow)
{
    Params params;
    std::vector<char*> argv = {const_cast<char*>("ipk-rdt"), const_cast<char*>("-s"), const_cast<char*>("-p"), const_cast<char*>("0")};

    params.parseArgs(static_cast<int>(argv.size()), argv.data());
    EXPECT_THROW(params.validate(), int);
}

TEST(ParamsTest, portOutOfRangeTooHigh)
{
    Params params;
    std::vector<char*> argv = {const_cast<char*>("ipk-rdt"), const_cast<char*>("-s"), const_cast<char*>("-p"), const_cast<char*>("65536")};

    params.parseArgs(static_cast<int>(argv.size()), argv.data());
    EXPECT_THROW(params.validate(), int);
}

TEST(ParamsTest, optionUnknown)
{
    Params params;
    std::vector<char*> argv = {const_cast<char*>("ipk-rdt"), const_cast<char*>("--unknown")};

    EXPECT_THROW(params.parseArgs(static_cast<int>(argv.size()), argv.data()), int);
}

// Network tests
TEST(NetworkTest, networkNoSocketDestination)
{
    Network network;

    EXPECT_FALSE(network.isSocketSet());
    EXPECT_FALSE(network.isDestinationKnown());
}

TEST(NetworkTest, setDestination)
{
    Network network;

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(2024);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    sockaddr_storage storage{};
    std::memcpy(&storage, &address, sizeof(address));

    network.setDestination(storage, sizeof(address));

    EXPECT_TRUE(network.isDestinationKnown());
    EXPECT_TRUE(network.isSameDestination(storage, sizeof(address)));
}

TEST(NetworkTest, differentPort)
{
    Network network;

    sockaddr_in first{};
    first.sin_family = AF_INET;
    first.sin_port = htons(2024);
    first.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    sockaddr_in second = first;
    second.sin_port = htons(2025);

    sockaddr_storage firstStorage{};
    sockaddr_storage secondStorage{};
    std::memcpy(&firstStorage, &first, sizeof(first));
    std::memcpy(&secondStorage, &second, sizeof(second));

    network.setDestination(firstStorage, sizeof(first));

    EXPECT_FALSE(network.isSameDestination(secondStorage, sizeof(second)));
}

// CLI tests
TEST(CliTest, help)
{
    if (!projectBinaryExists())
        GTEST_SKIP() << "./ipk-rdt is not built";

    int status = std::system("./ipk-rdt -h > /tmp/ipk_rdt_help.out 2> /tmp/ipk_rdt_help.err");
    EXPECT_EQ(exitCodeFromSystem(status), 0);
}

TEST(CliTest, missingMode)
{
    if (!projectBinaryExists())
        GTEST_SKIP() << "./ipk-rdt is not built";

    int status = std::system("./ipk-rdt -p 2024 > /tmp/ipk_rdt_missing_mode.out 2> /tmp/ipk_rdt_missing_mode.err");
    EXPECT_NE(exitCodeFromSystem(status), 0);
}

TEST(CliTest, clientNoAddress)
{
    if (!projectBinaryExists())
        GTEST_SKIP() << "./ipk-rdt is not built";

    int status = std::system("./ipk-rdt -c -p 2024 > /tmp/ipk_rdt_missing_addr.out 2> /tmp/ipk_rdt_missing_addr.err");
    EXPECT_NE(exitCodeFromSystem(status), 0);
}

TEST(CliTest, invalidPort)
{
    if (!projectBinaryExists())
        GTEST_SKIP() << "./ipk-rdt is not built";

    int status = std::system("./ipk-rdt -s -p 70000 > /tmp/ipk_rdt_bad_port.out 2> /tmp/ipk_rdt_bad_port.err");
    EXPECT_NE(exitCodeFromSystem(status), 0);
}

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
