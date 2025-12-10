#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR=$(cd "$(dirname "$0")"/.. && pwd)
cd "$ROOT_DIR"
docker build -f docker/Dockerfile.linux -t autosplatoon-linux .
CID=$(docker create autosplatoon-linux)
docker cp "$CID:/workspace/dist" "$ROOT_DIR/AutoSplatoon/"
docker rm "$CID" >/dev/null
echo "输出目录: AutoSplatoon/dist"
