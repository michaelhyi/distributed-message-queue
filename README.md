# Distributed Message Queue

An implementation of a Kafka-like distributed message queue in C. This message
queue utilizes a custom application-layer protocol called DMQP.

### Features
- Topics
- Partitions
- DMQP: Custom Application-Layer Protocol
- Security
- Fault Tolerance

### Design

#### Architecture

This distributed system follows a hierarchial, tree-like architecture. There's
a top-level server that handles all user requests, routing them to topics.
Topics, to the user, are different logical queues. Topics then route requests to 
partitions, which are horizontal shards of queues.

![Architecture diagram of the distributed message queue](.github/architecture.png)

### Quick Start

#### Requirements
- Docker

Enter the Docker container:
```bash
# run `chmod +x ./scripts/docker.sh` if permission denied
./scripts/docker.sh # add flag --attach to attach to running container
```

Compile and start a partition:
```bash
cd partition
make
./build/bin/partition
```
