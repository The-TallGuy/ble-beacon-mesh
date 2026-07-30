import asyncio
from bleak import BleakScanner

def callback(device, advertising_data):
    # if device.address == "E8:DB:84:2E:D4:16":
    print(f"[{device.address}] {device.name}")
    print(f"  RSSI: {advertising_data.rssi}")
    print(f"  Manufacturer Data: {advertising_data.manufacturer_data}")
    print("-----------------------------------------------------------------\n")

async def main():
    # Starts the scanner
    async with BleakScanner(detection_callback=callback):
        print("Listening for advertisements...")
        # Keeps the scanner alive for 10 seconds
        await asyncio.sleep(2.0)

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\nScanner stopped.")