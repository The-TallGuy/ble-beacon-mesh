#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEAdvertising.h>
#include <mbedtls/md.h>
#include <string.h>
#include <time.h>
#include "emergencyPacket.h"
#include "MurmurHash3.h"

constexpr char VICTIM_PHONE[] = "+40712345678";
constexpr uint32_t MURMUR_SEED = 112;
constexpr uint8_t SECRET_KEY[32] = {
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
    0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
    0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20};

constexpr bool USE_GPS = true;
constexpr int32_t TEST_LATITUDE = 444378000;
constexpr int32_t TEST_LONGITUDE = 260472000;
constexpr uint32_t GPS_PRECISION = 10000000;
constexpr uint32_t TIME_OFFSET = 1782864000 / 60; // Minutes since epoch on 01/07/2026, 00:00

constexpr uint8_t TTL = 20;
constexpr uint16_t CID = 0xFFFF;

uint64_t nonce = 1;
emergencyPacket globalPacket;
uint32_t lastMinute = TIME_OFFSET;

void buildEmergencyPacket(emergencyPacket &packet)
{
  memset(&packet, 0, sizeof(packet));

  MurmurHash3_x86_32(VICTIM_PHONE, strlen(VICTIM_PHONE), MURMUR_SEED, &packet.phone);
  packet.timestamp = TIME_OFFSET + static_cast<uint32_t>(time(nullptr) / 60);

  if (USE_GPS)
  {
    packet.payload.gps.latitude = static_cast<int32_t>(TEST_LATITUDE);
    packet.payload.gps.longitude = static_cast<int32_t>(TEST_LONGITUDE);
    packet.flags = packet.flags | (1 << 7);
  }
  else
  {
    packet.payload.nonce = nonce;
    // This is meant to happen if we detect movement via the phone's sensors (accelerometer, gyroscope, etc.)
    // packet.payload.nonce = nonce++;
  }

  unsigned char hash[32];
  const mbedtls_md_info_t *md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  if (md_info != nullptr)
  {
    if (mbedtls_md_hmac(md_info,
                        SECRET_KEY,
                        sizeof(SECRET_KEY),
                        reinterpret_cast<const unsigned char *>(&packet),
                        sizeof(packet) - sizeof(packet.hmac_sig),
                        hash) == 0)
    {
      memcpy(packet.hmac_sig, hash, 8 * sizeof(uint8_t));
    }
  }

  packet.flags = packet.flags | TTL;
}

void setup()
{
  Serial.begin(115200);
  BLEDevice::init();

  buildEmergencyPacket(globalPacket);

  BLEAdvertisementData advData;
  uint8_t payloadData[sizeof(CID) + sizeof(globalPacket)];
  memcpy(payloadData, &CID, sizeof(CID));
  memcpy(payloadData + sizeof(CID), &globalPacket, sizeof(globalPacket));

  advData.setManufacturerData(String(reinterpret_cast<const char *>(payloadData), sizeof(payloadData)));

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->setAdvertisementData(advData);

  // pAdvertising->start();
}

void loop()
{
  if (time(nullptr) / 60 != lastMinute)
  {
    lastMinute = time(nullptr) / 60;
    buildEmergencyPacket(globalPacket);

    BLEAdvertisementData advData;
    uint8_t payloadData[sizeof(CID) + sizeof(globalPacket)];
    memcpy(payloadData, &CID, sizeof(CID));
    memcpy(payloadData + sizeof(CID), &globalPacket, sizeof(globalPacket));

    advData.setManufacturerData(String(reinterpret_cast<const char *>(payloadData), sizeof(payloadData)));

    BLEDevice::getAdvertising()->setAdvertisementData(advData);

    Serial.println("Updated emergency packet");
  }

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->start();
  Serial.println("[VICTIM] Broadcasting SOS beacon for 1 second...");
  delay(500);

  pAdvertising->stop();
  Serial.println("[VICTIM] Radio OFF. Waiting 4 seconds...");
  delay(4500);
}