#!/usr/bin/env bash

set -euo pipefail

ATTACH=false
IMAGE=distributed-message-queue
ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

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
    docker stop "$IMAGE" >/dev/null 2>&1 || true 
    docker rm "$IMAGE" >/dev/null 2>&1 || true 
    docker build -f "$ROOT_DIR/Dockerfile" -t "$IMAGE" "$ROOT_DIR"
    docker run -it \
        --name "$IMAGE" \
        -v "$ROOT_DIR":/root \
        -w /root \
        "$IMAGE"
fi
