#ifndef TIME_AWARE_CACHE_H
#define TIME_AWARE_CACHE_H

#include <stdint.h>
#include <string.h>
#include "emergencyPacket.h"

#define CACHE_SIZE 256
#define HASH_SIZE 8
#define CACHE_TTL_MS 2000
#define MAX_TRACKED_MACS 4

/*
const char *cacheForwardMsg = "[FORWARD] New MAC detected. Forwarding packet.";
const char *cacheDropMsg = "[DROP] Packet dropped, MAC seen before in the past 15 seconds.";
*/

typedef struct
{
    uint8_t hmac[HASH_SIZE];
    uint32_t expires_at;
    uint8_t rx_count;
    bool active;
    bool broadcasted;
    uint32_t broadcast_time; // Random Jitter
    emergencyPacket packet;
    uint8_t seen_macs[MAX_TRACKED_MACS][6];
} cacheEntry;

typedef struct
{
    cacheEntry entries[CACHE_SIZE];
} timeAwareCache;

int16_t checkCache(timeAwareCache *cache, const uint8_t hmac[HASH_SIZE], uint32_t current_millis, const uint8_t *sender_mac, char **cacheMacStatus);
int16_t addToCache(timeAwareCache *cache, const uint8_t hmac[HASH_SIZE], const emergencyPacket *packet, uint32_t current_millis, uint32_t jitter_delay_ms, const uint8_t *sender_mac);

#endif