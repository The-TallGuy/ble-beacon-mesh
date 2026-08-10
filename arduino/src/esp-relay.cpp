#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEAdvertising.h>
#include <string.h>
#include "emergencyPacket.h"
#include "timeAwareCache.h"

#define GATEWAY_MODE

#ifdef GATEWAY_MODE
#include <WiFi.h>
#include <WiFiUdp.h>
#include "secrets.h"

WiFiUDP udpClient;
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 8000;
#endif

BLEScan *pBLEScan;
BLEAdvertising *pAdvertising;
timeAwareCache cache;
constexpr uint16_t TARGET_CID = 0xFFFF;
constexpr uint8_t REDUNDANCY = 4;
constexpr uint8_t LED_STATUS_PIN = 2;
constexpr uint16_t BROADCAST_BURST = 500; // in miliseconds

SemaphoreHandle_t cacheMutex;

bool isBroadcasting = false;
uint32_t broadcastEndTime = 0;
int16_t currentBroadcastIndex = -1;

char *cacheMacStatus;

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
  void onResult(BLEAdvertisedDevice advertisedDevice)
  {
    if (advertisedDevice.haveManufacturerData())
    {
      String strManufacturerData = advertisedDevice.getManufacturerData();
      const uint8_t *payloadRaw = (const uint8_t *)strManufacturerData.c_str();
      size_t length = strManufacturerData.length();

      if (length == sizeof(uint16_t) + sizeof(emergencyPacket))
      {
        uint16_t receivedCID = (uint16_t)payloadRaw[0] | ((uint16_t)payloadRaw[1] << 8);

        if (receivedCID == TARGET_CID)
        {
          emergencyPacket packet;
          memcpy(&packet, payloadRaw + sizeof(receivedCID), sizeof(emergencyPacket));

          uint32_t now = millis();

          const uint8_t *senderMAC = advertisedDevice.getAddress().getNative();

          // Activam semaforul (marcam ca foloseste cineva resursa)
          xSemaphoreTake(cacheMutex, portMAX_DELAY);
          int16_t location = checkCache(&cache, packet.hmac_sig, now, senderMAC, &cacheMacStatus);
          Serial.println(cacheMacStatus);
          if (location == -1)
          {
#ifdef GATEWAY_MODE
            if (WiFi.status() == WL_CONNECTED)
            {
              // Fire 3 UDP packets spaced by 5ms to guarantee delivery
              for (uint8_t b = 0; b < 3; b++)
              {
                udpClient.beginPacket(SERVER_IP, SERVER_PORT);
                udpClient.write(payloadRaw, length);
                udpClient.endPacket();
                delay(5);
              }

              Serial.println("[INTERNET] Packet forwarded via UDP Burst. Ignoring BLE rebroadcast.");

              addToCache(&cache, packet.hmac_sig, &packet, now, 0, senderMAC);

              int16_t newLoc = checkCache(&cache, packet.hmac_sig, now, senderMAC, &cacheMacStatus);
              if (newLoc != -1)
              {
                cache.entries[newLoc].broadcasted = true;
              }
            }
            else
#endif
            {
              uint8_t ttl = packet.flags & (0xFF >> 3);

              if (ttl > 0)
              {
                ttl -= 1;
                if (ttl != 0)
                {
                  packet.flags = (packet.flags & (0xFF << 5)) | ttl;

                  uint32_t randomJitter = random(50, 201);

                  addToCache(&cache, packet.hmac_sig, &packet, now, randomJitter, senderMAC);
                  Serial.printf("[QUEUED] Packet queued. Jitter: %d ms\r\n", randomJitter);
                }
                else
                {
                  Serial.printf("[DROP] Package TTL has expired!\r\n");
                }
              }
            }
          }
          xSemaphoreGive(cacheMutex);
        }
      }
    }
  }
};

void setup()
{
  Serial.begin(115200);
  pinMode(LED_STATUS_PIN, OUTPUT);
  digitalWrite(LED_STATUS_PIN, LOW);

#ifdef GATEWAY_MODE
  Serial.print("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  uint32_t wifiStartAttempt = millis();

  while (WiFi.status() != WL_CONNECTED &&
         (millis() - wifiStartAttempt) < WIFI_CONNECT_TIMEOUT_MS)
  {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("\n[SYSTEM] Wi-Fi Connected!");
  }
  else
  {
    Serial.println("\n[SYSTEM] Wi-Fi not found. Continuing in BLE-only relay mode.");
    WiFi.disconnect(true);
  }
#endif

  randomSeed(analogRead(0));

  cacheMutex = xSemaphoreCreateMutex();

  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan(); // create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); // active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99); // less or equal setInterval value

  pAdvertising = BLEDevice::getAdvertising();

  // Pornim scanarea in regim continuu, asincron
  pBLEScan->start(0, nullptr, false);
}

void loop()
{
  uint32_t now = millis();

  if (isBroadcasting)
  {
    if (now >= broadcastEndTime)
    {
      pAdvertising->stop();
      digitalWrite(LED_STATUS_PIN, LOW);
      pBLEScan->start(0, nullptr, false);
      isBroadcasting = false;
      cache.entries[currentBroadcastIndex].broadcasted = true; // Nu-l mai emitem iar
      Serial.println("[BURST] Transmission completed and stopped.\r\n");
    }
    return;
  }

  // Serial.println("[MUTEX] Semaphore enabled!");
  xSemaphoreTake(cacheMutex, portMAX_DELAY);
  for (uint32_t i = 0; i < CACHE_SIZE; i++)
  {
    if (cache.entries[i].active && !cache.entries[i].broadcasted)
    {
      if (now >= cache.entries[i].broadcast_time)
      {
        if (cache.entries[i].rx_count < REDUNDANCY)
        {
          BLEAdvertisementData advData;
          uint8_t rebroadcastData[sizeof(uint16_t) + sizeof(emergencyPacket)];

          memcpy(rebroadcastData, &TARGET_CID, sizeof(TARGET_CID));
          memcpy(rebroadcastData + sizeof(TARGET_CID), &(cache.entries[i].packet), sizeof(emergencyPacket));

          advData.setManufacturerData(String(reinterpret_cast<const char *>(rebroadcastData), sizeof(rebroadcastData)));

          Serial.println("[BROADCAST] Starting broadcast");
          pBLEScan->stop();
          pAdvertising->setAdvertisementData(advData);
          digitalWrite(LED_STATUS_PIN, HIGH);
          pAdvertising->start();

          isBroadcasting = true;
          broadcastEndTime = now + BROADCAST_BURST; // in miliseconds
          currentBroadcastIndex = i;

          Serial.printf("[FORWARD] Air clear. Started 500ms burst for row %d.\r\n", i);
          break;
        }
        else
        {
          cache.entries[i].broadcasted = true;
          Serial.printf("[CANCEL] Broadcast Storm prevented. Heard %d times. Aborted row %d.\r\n", cache.entries[i].rx_count, i);
        }
      }
    }
  }
  xSemaphoreGive(cacheMutex);
}