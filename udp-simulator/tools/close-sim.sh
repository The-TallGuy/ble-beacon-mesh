#!/usr/bin/env bash

# Exit immediately if a command exits with a non-zero status
set -e

echo "Stopping Compoviz visualizer..."
# Stopping it will trigger the --rm flag, automatically deleting it
docker stop compoviz || true

echo "Tearing down Docker Compose infrastructure..."
cd docker && docker compose down

echo "Mesh teardown complete."