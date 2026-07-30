#include "timeAwareCache.h"

int16_t checkCache(timeAwareCache *cache, const uint8_t hmac[HASH_SIZE], uint32_t current_millis, const uint8_t *sender_mac, char **cacheMacStatus)
{
    for (uint32_t i = 0; i < CACHE_SIZE; i++)
    {
        if (cache->entries[i].active)
        {
            if (current_millis >= cache->entries[i].expires_at)
            {
                cache->entries[i].active = false;
            }
            else
            {
                if (memcmp(cache->entries[i].hmac, hmac, HASH_SIZE) == 0)
                {
                    bool mac_known = false;
                    for (uint8_t j = 0; j < cache->entries[i].rx_count && j < MAX_TRACKED_MACS; j++)
                    {
                        if (memcmp(cache->entries[i].seen_macs[j], sender_mac, 6) == 0)
                        {
                            mac_known = true;
                            *cacheMacStatus = "[FORWARD] New MAC detected. Forwarding packet.";
                            break;
                        }
                    }

                    *cacheMacStatus = "[DROP] Packet dropped, MAC seen before in the past 15 seconds.";

                    if (!mac_known && cache->entries[i].rx_count < MAX_TRACKED_MACS)
                    {
                        memcpy(cache->entries[i].seen_macs[cache->entries[i].rx_count], sender_mac, 6);
                        cache->entries[i].rx_count++;
                    }

                    return i;
                }
            }
        }
    }
    return -1;
}

int16_t addToCache(timeAwareCache *cache, const uint8_t hmac[HASH_SIZE], const emergencyPacket *packet, uint32_t current_millis, uint32_t jitter_delay_ms, const uint8_t *sender_mac)
{
    int16_t target_idx = -1;

    for (uint32_t i = 0; i < CACHE_SIZE; i++)
    {
        if (!cache->entries[i].active || (current_millis >= cache->entries[i].expires_at))
        {
            cache->entries[i].active = false;
            if (target_idx == -1)
            {
                target_idx = i;
            }
        }
    }

    if (target_idx == -1)
    {
        target_idx = 0;
        uint32_t oldest_time = 0xFFFFFFFF;
        for (uint32_t i = 0; i < CACHE_SIZE; i++)
        {
            if (cache->entries[i].expires_at < oldest_time)
            {
                oldest_time = cache->entries[i].expires_at;
                target_idx = i;
            }
        }
    }

    memcpy(cache->entries[target_idx].hmac, hmac, HASH_SIZE);
    memcpy(&(cache->entries[target_idx].packet), packet, sizeof(emergencyPacket));
    cache->entries[target_idx].expires_at = current_millis + CACHE_TTL_MS;
    cache->entries[target_idx].broadcast_time = current_millis + jitter_delay_ms;
    cache->entries[target_idx].rx_count = 1;
    memcpy(cache->entries[target_idx].seen_macs[0], sender_mac, 6);
    cache->entries[target_idx].active = true;
    cache->entries[target_idx].broadcasted = false;

    return target_idx;
}