#ifndef EMERGENCY_PACKET_H
#define EMERGENCY_PACKET_H
#include <cstdint> // pentru 'packed'

struct __attribute__((packed)) emergencyPacket
{

    // HEADER (7 Bytes)
    uint32_t phone_id_hash; // 4B: Hash al numarului de telefon (Peppered)
    uint32_t timestamp;     // 2B: Contor minute curente (Anti-Replay)

    // 1B: Metadate
    // Bit 7 (MSB): 1 = GPS Valid, 0 = GPS Invalid
    // Bitii 6-5  : Future use
    // Bitii 4-0  : TTL (Time To Live, max 31 sarituri)
    uint8_t status_flags;

    // PAYLOAD (8 Bytes)
    union LocationData
    {
        // Cazul 1: GPS Valid
        struct __attribute__((packed))
        {
            int32_t latitude;  // 4B: Latitudine * 10^7
            int32_t longitude; // 4B: Longitudine * 10^7
        } gps;

        // Cazul 2: GPS Invalid. Folosit pentru a manipula cache-ul retelei.
        // Cand victima sta pe loc, ramane identic. Cand se misca, se genereaza altul.
        uint64_t nonce; // 8B: Numar aleatoriu / Message Counter
    } payload;

    // SECURITATE (8 Bytes)
    uint8_t hmac_sig[8]; // 8B: Semnatura criptografica trunchiata

    // 2 Bytes free
};

// Verificare compile-time a dimensiunii
static_assert(sizeof(emergencyPacket) == 25, "Dimensiunea pachetului trebuie sa fie exact 23 bytes!");
#endif