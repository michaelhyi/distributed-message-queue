#!/usr/bin/env bash

set -euo pipefail

if [ $# -ne 1 ]; then
    echo "Usage: $0 [host:port]"
    exit 1
fi

HOST=$1

/opt/zookeeper/bin/zkCli.sh -server $HOST create /partitions
/opt/zookeeper/bin/zkCli.sh -server $HOST create /partitions/lock
/opt/zookeeper/bin/zkCli.sh -server $HOST create /partitions/free-count 0
/opt/zookeeper/bin/zkCli.sh -server $HOST create /topics
