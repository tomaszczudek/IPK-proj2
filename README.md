# Project 2 - Reliable File Transfer over UDP

## 1. Overview
This project implements a reliable file transfer application over UDP. The program can run in two modes: as a client 
that reads data from a file or standard input and sends it to a remote endpoint, or as a server that receives the 
data and writes it to a file or standard output.

The application implements reliable file transfer over UDP. It uses protocol messages (`START`, `DATA`, `END`) and 
confirmations (`CONFIRM_START`, `CONFIRM_DATA`, `CONFIRM_END`). File content is split into packets with a maximum 
size of 1200 bytes, where 13 bytes are reserved for the packet header and the rest is used as payload.

## 2. Build & Test Instructions
* To compile the project, run: `make`
* To compile and run the automated tests, execute: `make test`

### Execution

### Client:
`./ipk-rdt -c -p PORT -a HOST [-i INPUT] [-w TIMEOUT] [-h | --help]`

### Server:
`./ipk-rdt -s -p PORT [-a ADDRESS] [-o OUTPUT] [-w TIMEOUT] [-h | --help]`


* `-h`/`--help`  -- print this help message
* `-s`         -- run in server mode
* `-c`         -- run in client mode
* `-p` PORT    -- UDP port number
* `-a` ADDRESS -- in server mode - the local bind address. If omitted, the server listens on all suitable local addresses.
* `-a` HOST    -- in client mode - the destination hostname or IPv4/IPv6 address.
* `-i` INPUT   -- the input file to send. If omitted or if INPUT is -, the client reads from stdin.
* `-o` OUTPUT  -- the output file to create/overwrite. If omitted or if OUTPUT is -, the server writes to stdout.
* `-w` TIMEOUT -- timeout in whole seconds. Default value 1.

**Client example:** `./ipk-rdt -c -a 127.0.0.1 -p 9000 -i sample.bin`

**Server example:** `./ipk-rdt -s -p 9000 -o received.bin`

> **Important:** The options `-s` and `-c` are mutually exclusive; exactly one of them must be specified.

## 3. Implemented Features & Behavior
* UDP communication: The project uses UDP sockets for both client and server communication.
* IPv4/IPv6 support: Address resolution is implemented with `getaddrinfo()` using `AF_UNSPEC`, therefore both IPv4 and IPv6 addresses may be used.
* Client/server modes: The executable supports both sending and receiving data depending on the selected mode.
* Custom packet format: Every packet contains a connection ID, packet ID, checksum, payload length, and message type.
* Reliability over UDP: The protocol confirms the beginning of the transfer, individual data packets, and the end of the transfer.
* Sliding window: The client may keep up to `WINDOW_SIZE` unconfirmed data packets.
* Retransmission: Unconfirmed packets are resent after the retransmission interval.
* Out-of-order buffering: The server stores out-of-order data packets and writes them once all previous packets have been received.
* Duplicate handling: Duplicate data packets are confirmed again without writing duplicate data to the output.
* Timeout handling: If no protocol progress is made within the configured timeout, the program exits with a non-zero error code.

## 4. Important Design Decisions
* Code Structure: The project is split into logical C++ classes: `Params` handles command-line arguments, `Packet` handles 
serialization and validation, `Network` wraps UDP socket operations, and `Client`/`Server` implement the protocol state machines.
* Packet IDs: Data packets are identified using a 32-bit `packetId`. This simplifies packet confirmation and out-of-order 
buffering, but also limits the maximum number of packets in one transfer.
* Connection ID: The client generates a random 32-bit `connectionId`. The server accepts packets only from the selected 
connection and endpoint after receiving `START`.
* Checksum Validation: Packets contain a 16-bit checksum. Invalid packets are discarded and not processed by the state machine.

## 5. Protocol
The application implements a simple reliable data transfer protocol over UDP.

### 5.1 Protocol packet/header format
Each protocol message consists of a fixed-size header followed by an optional data. The header contains metadata required 
for connection identification, packet ordering, checksum validation, and message type recognition.

| Field | Size | Description |
|---|---:|---|
| `connectionId` | 32 bits | Identifier of one client-server transfer session. |
| `packetId` | 32 bits | Sequential packet number used for ordering and acknowledgements. |
| `checkSum` | 16 bits | Checksum used to detect corrupted packet data. |
| `dataSize` | 16 bits | Size of the data in bytes. |
| `type` | 8 bits | Message type, for example `START`, `DATA`, `END`, or confirmation messages. |

The packet data is stored after the header. Control packets such as `START`, `CONFIRM_START`,
 `END`, and `CONFIRM_END` contain no data. `DATA` packets contain a part of the 
 transferred file.

**Supported message types**
| Message type | Meaning |
|---|---|
| `START` | Starts a new transfer session. |
| `CONFIRM_START` | Confirms that the server accepted the session. |
| `DATA` | Carries one data segment. |
| `CONFIRM_DATA` | Acknowledges a received data segment. |
| `END` | Requests transfer termination. |
| `CONFIRM_END` | Confirms successful termination. |
| `INVALID` | Represents an invalid or corrupted packet. |
| `NONE` | Represents an uninitialized packet state. |

### 5.2 Session establishment and termination
A session starts when the client sends a `START` packet with a generated `connectionId`. 
The server waits for this packet in its initial state. After receiving a valid `START`, 
the server stores the sender address and connection identifier, then replies with `CONFIRM_START`.

After all input data has been sent and acknowledged, the client sends an `END` packet. 
The server confirms this packet using `CONFIRM_END`. After this confirmation, both sides
 can terminate the transfer.

The usual successful communication flow is shown in the [Protocol Sequence Diagram](#protocol-sequence-diagram).

### 5.3 Sequencing and acknowledgement strategy
Each data packet has a `packetId`. The client uses packet IDs to identify sent data segments 
and to match them with received acknowledgements. The server uses packet IDs to detect 
whether a packet is expected, duplicated, or out of order.

For each valid `DATA` packet, the server sends a `CONFIRM_DATA` message with the corresponding
 `packetId`. The client removes the packet from its unconfirmed packet list only after 
 receiving the matching acknowledgement.

This mechanism allows the client to track which packets were delivered successfully 
and which packets must be retransmitted.

### 5.4 Retransmission strategy and timeout handling
Because UDP does not guarantee delivery, the client keeps sent but unconfirmed packets in memory. Each stored packet contains its serialized data and the time when it was last sent.

If an acknowledgement is not received before the retransmission timeout expires, the client sends the packet again. The same idea is used for important control packets such as `START` and `END`.

The application also uses a global timeout. If no valid progress is made for too long, the transfer is terminated with an error instead of waiting forever.

Timeout handling is therefore used for two purposes:

* retransmitting packets that may have been lost,
* stopping the program when the peer does not respond for too long.

### 5.5 Duplicate and out-of-order packet handling
The receiver must handle packets that arrive more than once or in a different order.
Duplicate packets are not written to the output again. Instead, the server resends the corresponding confirmation so that the client can stop retransmitting them.

Out-of-order data packets are accepted and temporarily stored if they belong to the current connection. Once all previous packets have been received, the buffered data can be written in the correct order.

Packets from a different address or with a different `connectionId` are ignored, because they do not belong to the active transfer session.

### 5.6 Connection identification strategy
Each client transfer session uses a generated `connectionId`. This value is placed into 
every protocol packet. The server stores the `connectionId` from the initial `START` packet 
and accepts only packets with the same identifier.

The server also stores the remote socket address of the first valid client. Later packets 
are checked against this stored endpoint. This prevents packets from another client or 
another transfer from being accidentally accepted as part of the current session.

### 5.7 Segment size and window behavior
The implementation uses a fixed maximum packet size. The header occupies part of the packet, 
and the remaining space is used for application data. The maximum payload size is 
therefore derived from the maximum packet size minus the packet header size.

The client can send multiple data packets before receiving confirmations, according 
to the configured window size. This improves throughput compared to sending only one packet 
and waiting for its acknowledgement every time.

The window limits how many unconfirmed packets may exist at the same time. When the 
window is full, the client waits for acknowledgements or retransmission events 
before sending more data.

### 5.8 UML and protocol diagrams
### UML Class Diagram

<p align="center">
  <img src="https://drive.google.com/thumbnail?id=1ZPd4SrdbJNyT-ZA7VnLbLeGDAmXbRDAL&sz=w1000" alt="UML class diagram">
  <br>
  <em>UML class diagram – project architecture</em>
</p>

### Client FSM

<p align="center">
  <img src="https://drive.google.com/thumbnail?id=1ChJjpGX0JW4X_NBwDJqVZB_WJQ31oVdG&sz=w1000" alt="Client FSM diagram">
  <br>
  <em>FSM diagram – client</em>
</p>

### Server FSM

<p align="center">
  <img src="https://drive.google.com/thumbnail?id=1QfS3y4XzvJzOSN29xdrNpiIkWeObXnif&sz=w1000" alt="Server FSM diagram">
  <br>
  <em>FSM diagram – server</em>
</p>

### Protocol Sequence Diagram

<p align="center">
  <img src="https://drive.google.com/thumbnail?id=10ShVq1HmijaTwq2bznTGCSFiWNwu1k3I&sz=w1000" alt="Protocol sequence diagram">
  <br>
  <em>Sequence diagram – client/server communication</em>
</p>

## 6. Testing
Automated testing is implemented using the **Google Test (GTest)** framework, which covers internal logic and CLI behavior. 

### 6.1 What was tested
1. Packet serialization and deserialization:
   * Valid packets preserve connection ID, packet ID, message type, payload size, and payload content.
   * Empty control packets such as `START` can be serialized and parsed.
   * Packets smaller than the required header size are rejected.
   * Corrupted serialized data is rejected by checksum validation.
   * Payloads larger than `MAX_DATA_SIZE` are rejected.

2. Parameter parsing:
   * Valid client arguments are parsed correctly.
   * Valid server arguments are parsed correctly.
   * Default file behavior uses `-` when no input/output file is specified.
   * Missing mode, invalid port, unknown options, and conflicting `-s`/`-c` modes are rejected.
   * Help mode exits successfully.

3. Network helper logic:
   * Contructed `Network` object has no socket and no known destination.
   * `setDestination()` stores a socket address.
   * `isSameDestination()` distinguishes identical and different endpoints.

4. CLI behavior:
   * The compiled binary returns `0` for `-h`.
   * Invalid argument combinations return non-zero exit codes.

### 6.2 Why it was tested
These parts were tested because they form the core of the application. The program depends on correct packet serialization, reliable checksum validation, valid command-line arguments, and proper recognition of remote endpoints. If any of these parts failed, the client and server could reject valid packets, accept corrupted data, communicate with the wrong peer, or start with an invalid configuration.

### 6.3 How it was tested
Using C++ unit tests for internal classes (`Params`, `Network`, `Packet`) and `std::system` calls to verify the binary's exit status.

### 6.4 Testing Environment
Nix `c` developer shell on an Ubuntu instance (WSL2).

| Test Area | Input | Expected Behavior | Result |
|---|---|---|---|
| Packet serialization | connection ID `42`, packet ID `7`, data `testdata` | Deserialized packet contains the same header and payload | PASS |
| Packet validation | Serialized packet with one corrupted payload byte | Packet is marked invalid | PASS |
| Packet size | Payload larger than `MAX_DATA_SIZE` | `EX_DATAERR` is thrown | PASS |
| Parameter parsing | `-c -p 2024 -a 127.0.0.1 -i input.bin -w 3` | Client parameters are stored correctly | PASS |
| Parameter parsing | `-s -p 2024 -o out.bin` | Server parameters are stored correctly | PASS |
| Parameter validation | `-s -c -p 2024` | Conflicting modes are rejected | PASS |
| CLI behavior | `./ipk-rdt -h` | Help is printed and exit code is `0` | PASS |
| CLI behavior | `./ipk-rdt -c -p 2024` | Missing destination address is rejected | PASS |

### Comparable Tool - TFTP
The project is similar to TFTP because both transfer data over UDP and implement their own reliability mechanisms. Unlike TFTP, this implementation uses a custom packet header, explicit connection ID, sliding window behavior, and packet buffering on the receiver side.

## 7. Known Limitations
* Maximum transfer size is limited by the 32-bit `packetId`. With the current maximum packet size, the theoretical upper bound is approximately `1200 * 2^32` bytes, and the usable payload limit is lower because 13 bytes are used by the `PacketHeader`.
* The server handles one selected client connection after receiving `START`, it is not a multi-client file server.

## 8. AI
During the project, ChatGPT was used as a supporting tool for:
* creating the automated tests (GoogleTest),
* improving the structure and wording of this README,
* creating images for [5.8 UML and protocol diagrams](#58-uml-and-protocol-diagrams)
## 9. Bibliography
* POSTEL, Jon, 1980. RFC 768: User Datagram Protocol. Online. Request for Comments. Internet Engineering Task Force. Available at: https://www.rfc-editor.org/rfc/rfc768. [Accessed 2 May 2026].
* POSTEL, Jon, 1981. RFC 793: Transmission Control Protocol. Online. Request for Comments. Internet Engineering Task Force. Available at: https://www.rfc-editor.org/rfc/rfc793. [Accessed 2 May 2026].
* EDDY, Wesley M., 2022. RFC 9293: Transmission Control Protocol (TCP). Online. Request for Comments. Internet Engineering Task Force. Available at: https://www.rfc-editor.org/rfc/rfc9293. [Accessed 2 May 2026].
* PAXSON, Vern, ALLMAN, Mark, CHU, Jerry and SARGENT, Matt, 2011. RFC 6298: Computing TCP's Retransmission Timer. Online. Request for Comments. Internet Engineering Task Force. Available at: https://www.rfc-editor.org/rfc/rfc6298. [Accessed 2 May 2026].
* SOLLINS, Karen, 1992. RFC 1350: The TFTP Protocol (Revision 2). Online. Request for Comments. Internet Engineering Task Force. Available at: https://www.rfc-editor.org/rfc/rfc1350. [Accessed 2 May 2026].
* The Linux man-pages project. `getaddrinfo(3)`, `socket(2)`, `poll(2)`, `sendto(2)`, `recvfrom(2)`. Online. Available at: https://man7.org/linux/man-pages/. [Accessed 2 May 2026].
