#ifndef EMERGENCY_PACKET_H
#define EMERGENCY_PACKET_H
#include <cstdint> // pentru 'packed'

#define BRD_DELAY 5

struct __attribute__((packed)) emergencyPacket
{

    // HEADER (7 Bytes)

    uint32_t phone;     // 4B: Hash al numarului de telefon (Peppered)
    uint32_t timestamp; // 4B: Contor minute de la Unix Epoch (Anti-Replay)

    /*
     1B Metadate
     Bit 7 (MSB): 1 = GPS Valid, 0 = GPS Invalid
     Bitii 6-5  : Future use
     Bitii 4-0  : TTL (Time To Live, max 31 sarituri)
    */
    uint8_t flags;

    // Asta nu are treaba cu Endianness. Are doar un byte.

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

    // Nici aici nu conteaza Endianness. Standardul C spune clar ca un array va fi aranjat in memorie in ordinea declarata.

    // 2 Bytes free
};

// Verificare compile-time a dimensiunii
static_assert(sizeof(emergencyPacket) == 25, "Dimensiunea pachetului trebuie sa fie exact 25 bytes!");
#endif