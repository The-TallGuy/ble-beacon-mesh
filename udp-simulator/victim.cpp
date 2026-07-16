#include "emergencyPacket.h"
#include "public/MurmurHash3.h"
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <endian.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>

#define SERVERPORT 4950
#define MURMUR_SEED 112
#define TTL 20
#define GPS_PRECISION 10000000
#define BRD_LIFETIME 30

const unsigned char secret_key[32] = {
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
    0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
    0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20};

uint64_t nonce = 0;

int main(int argc, char *argv[])
{

    // PACKAGE ASSEMBLY

    emergencyPacket packet;
    memset(&packet, 0, sizeof(packet));
    MurmurHash3_x86_32(argv[1], strlen(argv[1]), MURMUR_SEED, &packet.phone);

    // Tot codul pentru status_flag e scris sub presupunerea ca am avea codul in little endian, deci MSB ar fi "cel mai din stanga" bit.
    // Noi avem nevoie de big endian, deci avem de facut swap cand datele sunt gata sa fie trimise, si NU mai avem nevoie sa lucram cu ele pe sender

    packet.timestamp = time(NULL) / 60;
    if (strcmp(argv[2], "1") == 0)
    {
        packet.payload.gps.latitude = (int32_t)(atof(argv[3]) * GPS_PRECISION);
        packet.payload.gps.longitude = (int32_t)(atof(argv[4]) * GPS_PRECISION);
        packet.flags = packet.flags | (1 << 7);
    }
    else
        packet.payload.nonce = nonce++;

    packet.phone = htonl(packet.phone);
    packet.timestamp = htonl(packet.timestamp);

    if (strcmp(argv[2], "0") == 0)
        packet.payload.nonce = htobe64(packet.payload.nonce);
    else
    {
        packet.payload.gps.latitude = htonl(packet.payload.gps.latitude);
        packet.payload.gps.longitude = htonl(packet.payload.gps.longitude);
    }

    unsigned int hash_len;
    unsigned char hash[32];
    HMAC(
        EVP_sha256(),
        secret_key,
        32,
        (unsigned char *)&packet,
        sizeof(packet),
        hash,
        &hash_len);

    memcpy(packet.hmac_sig, hash, sizeof(packet.hmac_sig));

    packet.flags = packet.flags | TTL;

    // NETWORKING

    // char addr_ipv4[16] = "255.255.255.255";
    // uint32_t addr_bin;
    // memset(&addr_bin, 255, 4);

    int sockfd = socket(PF_INET, SOCK_DGRAM, 0);
    int broadcast = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));

    struct sockaddr_in node;
    node.sin_family = AF_INET;               // 1 Byte field. No need to worry abt endianness
    node.sin_addr.s_addr = INADDR_BROADCAST; // Also needs to have network's endianness, but since it's literally all 1s, it's the same either way
    node.sin_port = htons(SERVERPORT);
    memset(node.sin_zero, 0, sizeof(node.sin_zero));
    // getaddrinfo(addr_ipv4, NULL, );

    int numbytes;

    for (uint8_t brdSecond = 0; brdSecond < BRD_LIFETIME; brdSecond++)
    {
        numbytes = sendto(sockfd, &packet, sizeof(packet), 0, (sockaddr *)&node, sizeof(node));
        printf("Sent %d bytes to %s\n", numbytes, inet_ntoa(node.sin_addr));
        sleep(1);
    }
    close(sockfd);
    return 0;
}