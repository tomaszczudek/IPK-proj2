# Changelog

## 2026-05-03

* Implemented reliable UDP file transfer functionality.
* Added client and server modes.
* Added custom packet format with connection ID, packet ID, checksum, data size, and message type.
* Added session establishment and termination using `START`, `CONFIRM_START`, `END`, and `CONFIRM_END`.
* Added acknowledgement handling for data packets.
* Added retransmission and timeout handling.
* Added duplicate and out-of-order packet handling.
* Added file and standard input/output transfer support.
* Added project README with protocol description, build instructions, testing overview, UML/FSM diagrams, known limitations, and AI usage declaration.
* Added GoogleTest-based automated tests for packet handling, parameter parsing, network helpers, and CLI behavior.