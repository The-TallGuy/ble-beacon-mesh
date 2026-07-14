#include "emergencyPacket.h"
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <endian.h>

#define SERVERPORT 4950
#define GPS_PRECISION 10000000

using namespace std;

/* TO DO:
    1) Receive a Broadcast
    2) Cache
    3) TTL decrease
*/
int main()
{
    // LISTENING

    int listenFd = socket(PF_INET, SOCK_DGRAM, 0);

    // BROADCASTING

    int sendFd = socket(PF_INET, SOCK_DGRAM, 0);
    int broadcast = 1;
    setsockopt(sendFd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));

    struct sockaddr_in neighbor;
    neighbor.sin_family = AF_INET;               // 1 Byte field. No need to worry abt endianness
    neighbor.sin_addr.s_addr = INADDR_BROADCAST; // Also needs to have network's endianness, but since it's literally all 1s, it's the same either way
    neighbor.sin_port = htons(SERVERPORT);
    memset(neighbor.sin_zero, 0, sizeof(neighbor.sin_zero));

    int numbytes;

    while (1)
    {
        // Need to receive the packet first
        // numbytes = sendto(sendFd, &packet, sizeof(packet), 0, (sockaddr *)&neighbor, sizeof(neighbor));
        printf("Sent %d bytes to %s\n", numbytes, inet_ntoa(neighbor.sin_addr));
        sleep(1);
    }
    close(sendFd);

    //

    return 0;
}