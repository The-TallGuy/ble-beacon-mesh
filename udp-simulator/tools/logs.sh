#!/usr/bin/env bash

# Exit immediately if a command exits with a non-zero status
set -e

echo "Streaming mesh network logs..."
docker compose -f docker/docker-compose.yml logs -f