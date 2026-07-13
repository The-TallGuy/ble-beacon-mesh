#include "emergencyPacket.h"
#include "public/MurmurHash3.h"
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#define SERVERPORT 4950
#define SEED 112

int main(int argc, char *argv[])
{
    // char addr_ipv4[16] = "255.255.255.255";
    emergencyPacket packet;
    MurmurHash3_x86_32(argv[1], strlen(argv[1]), SEED, &packet.phone_id_hash);

    packet.timestamp = time(NULL);
    if (argv[2])
    {
        packet.payload.gps.latitude = atoi(argv[3]);
        packet.payload.gps.longitude = atoi(argv[4]);
    }
    else
        packet.payload.nonce =

            uint32_t addr_bin;
    memset(&addr_bin, 255, 4);

    int sockfd = socket(PF_INET, SOCK_DGRAM, 0);
    int broadcast = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof broadcast);

    struct sockaddr_in node;
    node.sin_family = AF_INET;
    node.sin_addr.s_addr = addr_bin;
    node.sin_port = htons(SERVERPORT);
    memset(node.sin_zero, 0, sizeof node.sin_zero);
    // getaddrinfo(addr_ipv4, NULL, );

    int numbytes;

    while (1)
    {
        numbytes = sendto(sockfd, "Test 1", 7, 0, (sockaddr *)&node, sizeof node);
        printf("Sent %d bytes to %s\n", numbytes, inet_ntoa(node.sin_addr));
        sleep(1);
    }
    close(sockfd);
    return 0;
}