#!/usr/bin/env bash

# Localhost-only binding — do not swap for the non-"-localhost" URL
# unless you specifically want port 5001 reachable from other hosts.

set -euo pipefail

EDGESHARK_URL="https://github.com/siemens/edgeshark/raw/main/deployments/wget/docker-compose-localhost.yaml"

case "${1:-}" in
  up)
    wget -q --no-cache -O - "$EDGESHARK_URL" \
      | DOCKER_DEFAULT_PLATFORM= docker compose -p edgeshark -f - up -d
    echo "Opening Edgeshark visualizer in default browser..."
    powershell.exe -Command "Start-Process 'http://localhost:5001'"
    ;;
  down)
    wget -q --no-cache -O - "$EDGESHARK_URL" \
      | DOCKER_DEFAULT_PLATFORM= docker compose -p edgeshark -f - down
    ;;
  *)
    echo "Usage: $0 {up|down}"
    exit 1
    ;;
esac
