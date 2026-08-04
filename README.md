# BLE Beacon Mesh

A decentralized Bluetooth Low Energy mesh for emergency payload relay over short-range broadcast links. The design is built around BLE 4.0 Advertising only, so any compatible device can participate as a transmitter or relay without setting up a connection.

## Core Model

The system uses managed flooding.

A source node builds a fixed-size emergency packet, publishes it as Manufacturer Specific Data, and repeats the broadcast in short bursts. Relay nodes scan continuously, inspect each advertisement, discard duplicates through a local cache, decrement TTL, and rebroadcast only after a randomized delay. A gateway node is just another relay that can forward the payload to an upstream service when external connectivity exists.

The goal is to avoid attempting to keep track of the network's state. There is no routing table, no connection graph, and no explicit neighbor discovery protocol. Forwarding decisions are local and time bounded.

## Packet Format

The on-air payload is packed into a 25-byte `emergencyPacket` structure. In BLE advertising, it is carried inside Manufacturer Specific Data with a 2-byte Company ID prefix, so the full manufacturer blob is 27 bytes and still fits inside the 31-byte legacy advertising data budget.

```cpp
struct __attribute__((packed)) emergencyPacket
{
    uint32_t phone;
    uint32_t timestamp;
    uint8_t flags;

    union LocationData
    {
        struct __attribute__((packed))
        {
            int32_t latitude;
            int32_t longitude;
        } gps;

        uint64_t nonce;
    } payload;

    uint8_t hmac_sig[8];
};
```

### Field Breakdown

- `phone` is a 32-bit MurmurHash3 hash of the sender identifier with a fixed seed and peppered input
- `timestamp` is a minute-resolution timestamp used for preventing anti-replay attacks
- `flags` stores one location availability bit and a 5-bit TTL field
- `payload` is either GPS coordinates or an opaque nonce used to keep repeated emissions distinct when required
- `hmac_sig` is an 8-byte truncated HMAC over the packet body

## Routing Logic

Forwarding is controlled by a local cache rather than by topology knowledge.

Each relay keeps a fixed-size time-aware cache indexed by the packet HMAC. A new frame is accepted only once per cache lifetime. If the packet is new, the relay decrements TTL, stores the frame, and schedules a rebroadcast after a random delay between 50 ms and 200 ms. The delay is the suppression window that reduces synchronized retransmissions.

Broadcast storm control comes from three checks:

- duplicate suppression through the HMAC cache
- TTL exhaustion at each hop
- redundancy suppression using observed sender MACs and a small receive counter threshold

If a relay hears the same packet too many times before its own timer expires, it marks the frame as covered and cancels rebroadcast. The implementation also uses a bounded 500 ms transmit burst on the advertising side, then returns to scanning.

## BLE Constraints

The implementation relies on legacy BLE Advertising because it is the lowest common denominator for older phones and embedded nodes. This choice avoids connection setup and pairing overhead.

Only Manufacturer Specific Data is used for the application payload. The layout is intentionally compact so it stays inside the legacy advertising data limit without depending on newer extended advertising features.

## Repository Layout

- `arduino/` ESP32 implementation for the source and relay roles
- `udp-simulator/` C++ mesh simulator that mirrors the relay logic over UDP
- `python-scanner/` simple BLE scan utility for inspecting live advertisements
- `setup-arduino.sh` and `setup-arduino.ps1` helpers that prepare Arduino-compatible sketches from the PlatformIO source tree

## Build Surfaces

### ESP32 with PlatformIO

The Arduino implementation is configured for `esp32dev` in `arduino/platformio.ini`.

### Arduino IDE Sketch Export

Use the helper scripts as a fallback when PlatformIO is unable to compile.

On Linux or WSL:
```bash
./setup-arduino.sh relay
./setup-arduino.sh victim
```

On Windows PowerShell:
```powershell
./setup-arduino.ps1 relay
./setup-arduino.ps1 victim
```

### UDP Mesh Simulator

The simulator reproduces the same packet forwarding model over UDP for offline testing.

Boot the simulator:
```bash
cd udp-simulator
./tools/boot-sim.sh
```

Watch live packet flow:
```bash
./tools/logs.sh
```

Shut the simulator down:
```bash
./tools/close-sim.sh
```

## Implementation Notes

- TTL is stored in the low 5 bits of `flags` and decremented at every hop
- GPS and nonce share the same 8-byte payload slot through a packed union
- The victim side rebuilds the packet periodically so the cache and anti-replay logic stay aligned with the current emission cycle
- The mesh is designed for short-range on-demand propagation, not for stable end-to-end paths
