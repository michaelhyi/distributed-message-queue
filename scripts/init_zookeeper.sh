#!/usr/bin/env bash

set -euo pipefail

PARENT_ZNODES=("free" "topics")

# create parent znodes
for parent_znode in "${PARENT_ZNODES[@]}"; do
    /opt/zookeeper/bin/zkCli.sh -server 127.0.0.1:2181 create "/$parent_znode"
done
