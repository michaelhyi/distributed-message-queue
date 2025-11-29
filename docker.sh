#!/usr/bin/env bash

docker build -t distributed-message-queue .
docker run -it -v "$(pwd)":/root -w /root distributed-message-queue
