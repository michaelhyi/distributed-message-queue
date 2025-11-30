# Distributed Message Queue

An implementation of a Kafka-like distributed message queue in C. This message
queue utilizes sockets and a custom application-layer protocol.

### Features
- Topics
- Partitions
- Security
- Fault Tolerance

### Quick Start

#### Requirements
- Docker

Enter the Docker container:
```bash
# run `chmod +x ./docker.sh` if permission denied
./docker.sh
```

Once you've entered the Docker container, compile and start the service:
```
make
./build/distributed-message-queue
```
