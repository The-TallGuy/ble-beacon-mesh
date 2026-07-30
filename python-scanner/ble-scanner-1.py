import asyncio
import struct
import hmac
import hashlib
from bleak import BleakScanner
from datetime import datetime

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

COLOR_RELAY = '\033[94m'    # Albastru deschis
COLOR_VICTIM = '\033[95m'   # Mov deschis
COLOR_ALERT  = '\033[91m'   # Rosu
COLOR_SUCCESS = '\033[92m'  # Verde
COLOR_RESET  = '\033[0m'    # Resetare la culoarea implicita

MAC_VICTIM = 'E8:DB:84:2E:D4:16'
MAC_RELAY = '88:13:BF:6F:98:7E'

def verify_hmac(payload_25_bytes: bytes) -> bool:
    mutable_payload = bytearray(payload_25_bytes[:17])
    mutable_payload[8] = mutable_payload[8] & TTL_MASK 

    computed_hash = hmac.new(SECRET_KEY, mutable_payload, hashlib.sha256).digest()

    return computed_hash[:8] == payload_25_bytes[17:25]

def callback(device, advertising_data):
    # {
    if not advertising_data.manufacturer_data:
        return

    if TEST_CID not in advertising_data.manufacturer_data:
        return
        
    # Bleak a scos primii 2 bytes ca si CID, deci data_bytes are fix cei 25 de octeti ai structurii tale
    data_bytes = advertising_data.manufacturer_data[TEST_CID]
    
    if len(data_bytes) != PACKET_BYTES:
        return

    phone_hash, timestamp, flags, payload_raw, _ = struct.unpack('<IIB8s8s', data_bytes)

    gps_valid = (flags >> 7) & 1
    ttl = flags & 0x1F
    is_valid = verify_hmac(data_bytes)

    mac_color = ""
    if device.address == MAC_RELAY:
        mac_color = COLOR_RELAY
    elif device.address == MAC_VICTIM:
        mac_color = COLOR_VICTIM

    status_color = COLOR_SUCCESS if is_valid else COLOR_ALERT

    print(f"[{mac_color}{device.address}{COLOR_RESET}] PACHET SOS DETECTAT")
    print(f"\tRSSI       : {advertising_data.rssi} dBm")
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
    # }

async def main():
    async with BleakScanner(detection_callback=callback):
        print("Server STS pornit. Se asculta dupa pachete de urgenta...")
        while True:
            await asyncio.sleep(1.0)

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\nScanner oprit manual.")