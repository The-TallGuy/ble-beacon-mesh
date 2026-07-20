#!/usr/bin/env bash

set -e

# 1. Generate the new topology
echo "Generating mesh topology..."
python3 tools/generate_mesh.py

# 2. Start Compoviz in the background
echo "Starting Compoviz..."
docker run -d -p 8080:80 --name compoviz --rm ghcr.io/adavesik/compoviz:latest

# 3. Boot the actual infrastructure
echo "Starting Docker Compose infrastructure..."
cd docker && docker compose up --build -d

# 4. Automate opening the default browser
echo "Opening visualizer in default browser..."
powershell.exe -Command "Start-Process 'http://localhost:8080'"

echo "Mesh is running. Please manually upload docker/docker-compose.yml in the Compoviz UI."