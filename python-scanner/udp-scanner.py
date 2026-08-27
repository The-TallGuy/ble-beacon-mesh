import asyncio
import struct
import hmac
import hashlib
from datetime import datetime

# IP configurat pentru hotspotul laptopului
UDP_IP = "0.0.0.0"
UDP_PORT = 8080

TEST_CID = 0xFFFF
GPS_PRECISION = 10000000
TTL_MASK = 0b11100000
PACKET_BYTES = 25
SECRET_KEY = bytes([
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
    0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
    0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20
])

COLOR_GATEWAY = '\033[94m'  # Light Blue
COLOR_ALERT  = '\033[91m'   # Red
COLOR_SUCCESS = '\033[92m'  # Green
COLOR_RESET  = '\033[0m'

def verify_hmac(payload_25_bytes: bytes) -> bool:
    mutable_payload = bytearray(payload_25_bytes[:17])
    mutable_payload[8] = mutable_payload[8] & TTL_MASK
    computed_hash = hmac.new(SECRET_KEY, mutable_payload, hashlib.sha256).digest()
    return computed_hash[:8] == payload_25_bytes[17:25]

class EmergencyProtocol(asyncio.DatagramProtocol):
    def __init__(self):
        super().__init__()
        self.cache = {}
        self.CACHE_TTL = 0.020
        self.mapped_victims = set()

    # Apelat automat cand portul este deschis cu succes
    def connection_made(self, transport):
        self.transport = transport
        print(f"Server pornit pe portul {UDP_PORT}. Se asculta dupa pachete de urgenta...")

    # Apelat automat de OS cand un pachet UDP ajunge la port
    def datagram_received(self, data, addr):
        if len(data) != PACKET_BYTES + 2:
            return

        received_cid = struct.unpack('<H', data[:2])[0]
        if received_cid != TEST_CID:
            return

        data_bytes = data[2:]

        hmac_sig = data_bytes[17:25]
        now = asyncio.get_running_loop().time()

        # Sa nu arate toate pachetele dintr-un burst
        if hmac_sig in self.cache:
            if now - self.cache[hmac_sig] < self.CACHE_TTL:
                return

        self.cache[hmac_sig] = now

        phone_hash, timestamp, flags, payload_raw, _ = struct.unpack('<IIB8s8s', data_bytes)

        gps_valid = (flags >> 7) & 1
        ttl = flags & 0x1F
        is_valid = verify_hmac(data_bytes)

        status_color = COLOR_SUCCESS if is_valid else COLOR_ALERT

        print(f"[{COLOR_GATEWAY}{addr[0]}{COLOR_RESET}] PACHET SOS DETECTAT")
        print(f"\tPhone Hash : {phone_hash}")
        print(f"\tTimestamp  : {datetime.fromtimestamp(timestamp * 60).strftime('%H:%M, %d %B %Y')}")
        print(f"\tTTL        : {ttl}")

        if gps_valid:
            lat_raw, lon_raw = struct.unpack('<ii', payload_raw)
            print(f"\tLocatie    : Lat {lat_raw / GPS_PRECISION}, Lon {lon_raw / GPS_PRECISION}")

        else:
            nonce = struct.unpack('<Q', payload_raw)[0]
            print(f"\tNonce      : {nonce} (Fara date GPS)")

        print(f"Integritate: {status_color}{'VALIDA' if is_valid else 'COMPROMISA'}{COLOR_RESET}")
        print("-" * 60)

async def main():
    loop = asyncio.get_running_loop()

    transport, protocol = await loop.create_datagram_endpoint(
        lambda: EmergencyProtocol(),
        local_addr=(UDP_IP, UDP_PORT)
    )

    try:
        await asyncio.Future()
    finally:
        transport.close()

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\nScanner oprit manual.")