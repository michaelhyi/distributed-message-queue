#!/usr/bin/env bash

IMAGE=distributed-message-queue
ATTACH=false

while [[ $# -gt 0 ]]; do
  case "$1" in
    --attach)
      ATTACH=true
      shift
      ;;
    *)
      echo "unknown option $1"
      exit 1
      ;;
  esac
done

if $ATTACH; then
  if docker ps --format '{{.Names}}' | grep -q "^${IMAGE}$"; then
    docker exec -it "$IMAGE" /bin/bash
  else
    echo "no running container ${IMAGE}"
    exit 1
  fi
else
    docker build -t "$IMAGE" .
    docker run -it \
        --name "$IMAGE" \
        -v "$(pwd)":/root \
        -w /root \
        "$IMAGE"
fi
