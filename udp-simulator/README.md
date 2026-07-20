# UDP Mesh Simulation

A low-level, layer-2 UDP mesh network simulation built in C++. This project models emergency packet routing and caching algorithms across dynamically generated isolated subnets using Docker bridge networks.

## Prerequisites
This simulation relies on bash automation and is designed to be executed within a **Linux/WSL2** environment.

* **Docker & Docker Compose** (Docker Desktop recommended for WSL integration)
* **Python 3.x**
* **Jinja2** (for infrastructure templating)
* **C++ Build Tools** (g++, libssl-dev)

### Installing Python Dependencies
```bash
pip install jinja2
```

## Quick Start

1. **Clone the repository:**
   ```bash
   git clone https://github.com/The-TallGuy/ble-beacon-mesh.git
   ```
   ```bash
   cd ble-beacon-mesh/udp-simulator
   ```

2. **Boot the Simulation:**
   Run the master initialization script. This will prompt you for the number of relay nodes to generate, compile the C++ binaries, map the randomized network topologies, and boot the Compoviz visualizer.
   ```bash
   ./tools/boot-sim.sh
   ```

3. **Monitor the Packets:**
   Once the mesh is running, you can track the emergency broadcasts propagating through the network by tailing the container logs:
   ```bash
   ./tools/logs.sh
   ```

4. **Teardown:**
   To completely destroy the virtual infrastructure and clean up all containers and networks:
   ```bash
   ./tools/close-sim.sh
   ```

## Architecture
* `victim.cpp`: The broadcaster node that generates and signs the emergency payload.
* `node.cpp`: The relay nodes that implement the `circularBuff` caching logic to prevent packet storms while bridging isolated network zones.
* `tools/generate_mesh.py`: The Python engine that dynamically renders the `docker-compose.yml` topology using semi-random clustering.