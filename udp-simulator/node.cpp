#include "emergencyPacket.h"
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <endian.h>
#include "lib/circularBuff.h"

#define SERVERPORT 4950
#define GPS_PRECISION 10000000

using namespace std;

/* TO DO:
    1) Receive a Broadcast ✔️
    2) Cache ✔️
    3) TTL decrease ✔️
*/
int main()
{
    // LISTENING

    struct emergencyPacket packet;
    memset(&packet, 0, sizeof(emergencyPacket));

    int listenFd = socket(PF_INET, SOCK_DGRAM, 0);

    /*
        This is the variable holding the addresses we are listening to
        They are SENDING information to us.
    */
    struct sockaddr_in senders;
    int reuse = 1;

    senders.sin_family = AF_INET;
    senders.sin_addr.s_addr = INADDR_ANY;
    senders.sin_port = htons(SERVERPORT);

    setsockopt(listenFd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));
    memset(senders.sin_zero, 0, sizeof(senders.sin_zero));
    bind(listenFd, (sockaddr *)&senders, sizeof(senders));

    int recvBytes;
    int recvAddrBytes = sizeof(senders);

    // BROADCASTING

    /*
        struct emergencyPacket Spacket;
        memset(&Spacket, 0, sizeof(emergencyPacket));
    */

    int sendFd = socket(PF_INET, SOCK_DGRAM, 0);
    int broadcast = 1;
    setsockopt(sendFd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));

    struct sockaddr_in listeners;
    listeners.sin_family = AF_INET;               // 1 Byte field. No need to worry abt endianness
    listeners.sin_addr.s_addr = INADDR_BROADCAST; // Also needs to have network's endianness, but since it's literally all 1s, it's the same either way
    listeners.sin_port = htons(SERVERPORT);
    memset(listeners.sin_zero, 0, sizeof(listeners.sin_zero));

    int sentBytes;

    circularBuffer cache;
    memset(&cache, 0, sizeof(circularBuffer));

    while (1)
    {
        puts("Listening...");
        recvBytes = recvfrom(listenFd, &packet, sizeof(emergencyPacket), 0, (sockaddr *)&senders, (socklen_t *)&recvAddrBytes);
        printf("Received %d bytes from %s\n", recvBytes, inet_ntoa(senders.sin_addr));

        // cacheCheck()
        int16_t location = containsBuff(&cache, packet.hmac_sig);
        if (location == -1)
        {
            addToBuff(&cache, packet.hmac_sig);
            uint8_t ttl = packet.flags & (0xFF >> 3);
            if (ttl > 0)
            {
                ttl -= 1;
                if (ttl != 0)
                {
                    packet.flags = (packet.flags & (0xFF << 5)) | ttl;
                    sentBytes = sendto(sendFd, &packet, sizeof(emergencyPacket), 0, (sockaddr *)&listeners, sizeof(listeners));
                    printf("Sent %d bytes to %s\n", sentBytes, inet_ntoa(listeners.sin_addr));
                }
                else
                    puts("The package's TTL has expired!");
            }
        }
        else
            printf("This package has already been processed recently. You can find its hash on row %d.\n", location);
        sleep(1);
    }
    close(sendFd);
    return 0;
}