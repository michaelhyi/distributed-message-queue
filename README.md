# Distributed Message Queue

An implementation of a Kafka-like distributed message queue in C. This message
queue utilizes a custom application-layer protocol called DMQP.

### Features
- Topics
- Partitions
- DMQP: Custom Application-Layer Protocol
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
./build/bin/run
```

### Design

#### Architecture

This distributed system follows a hierarchial, tree-like architecture. There's
a top-level server that handles all user requests, routing them to topics.
Topics, to the user, are different logical queues. Topics then route requests to 
partitions, which are horizontal shards of queues.

![Architecture diagram of the distributed message queue](.github/architecture.png)
