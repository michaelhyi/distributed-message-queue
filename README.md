# Distributed Message Queue

An implementation of a Kafka-like distributed message queue in C. This message
queue utilizes a custom application-layer protocol called DMQP.

### Features
- Topics
- Horizontal Partitions: Sharding & Replication
- DMQP: Custom Application-Layer Protocol
- Security
- Fault Tolerance

### Design

#### Architecture

This distributed system follows a hierarchial architecture. There's a top-level
server called the controller that handles all user requests, routing them to
topics. Topics, to the user, are different logical queues. Topics then route
requests to horizontal shards of queues. Shards are replicated, following a
leader-follower architecture. Therefore, topics route requests to the replica
set leader. The replica set leader then replicates data to followers. The Raft
algorithm is used for leader election within a replica set.

![Architecture diagram of the distributed message queue](.github/architecture.png)

#### Networking

This distributed system uses a custom application-layer protocol called DMQP.
DMQP stands for Distributed Message Queue Protocol. A DMQP message uses the
following format:

```
+-------------------------------------------------------------------------------+
|                            Timestamp (8 bytes)                                |
+-------------------------------------------------------------------------------+
|                              Length (4 bytes)                                 |
+-------------------------------------------------------------------------------+
| Method (1 byte) | Topic ID (1 byte) | Status Code (1 byte) | Unused (1 byte)  |
+-------------------------------------------------------------------------------+
|                             Payload (Max 1MB)                                 |
+-------------------------------------------------------------------------------+
```

DMQP uses TCP with persistent connections, leveraging keepalive to preserve
resources only for active queue producers while maximizing throughput. DMQP
also uses TLS encryption and a timeout of 30 seconds on each socket.

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
make
./partition/partition
```
