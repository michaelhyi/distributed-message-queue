# Distributed Message Queue

An implementation of a Kafka-like distributed message queue in C. This message queue utilizes sockets and a custom application-layer protocol.

### Features
- Topics
- Partitions
- Security
- Fault Tolerance

### Quick Start

#### Requirements
- Docker

```bash
# run `chmod +x ./docker.sh` if permission denied
./docker.sh
make
./build/distributed-message-queue
```
