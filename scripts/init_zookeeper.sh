#!/usr/bin/env bash

set -euo pipefail

/opt/zookeeper/bin/zkCli.sh -server 127.0.0.1:2181 create /partitions
/opt/zookeeper/bin/zkCli.sh -server 127.0.0.1:2181 create /partitions/lock
/opt/zookeeper/bin/zkCli.sh -server 127.0.0.1:2181 create /partitions/free-count 0

/opt/zookeeper/bin/zkCli.sh -server 127.0.0.1:2181 create /topics
