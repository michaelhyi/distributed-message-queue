#!/usr/bin/env bash

docker build -t dmq .
docker run -it -v "$(pwd)":/root -w /root dmq
