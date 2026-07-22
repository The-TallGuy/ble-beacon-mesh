#include "emergencyPacket.h"
#include <netdb.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include "lib/circularBuff.h"

// TERMINAL COLORING
#define LOG_RESET "\033[0m"
#define LOG_GREEN "\033[42;1;37m"   // Green background, bold white text
#define LOG_CYAN "\033[46;1;37m"    // Cyan background, bold white text
#define LOG_YELLOW "\033[43;1;37m"  // Yellow background, bold white text
#define LOG_MAGENTA "\033[45;1;37m" // Magenta background, bold white text
//

#define SERVERPORT 4950
#define GPS_PRECISION 10000000

using namespace std;

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

    //

    // CACHING

    circularBuffer cache;
    memset(&cache, 0, sizeof(circularBuffer));

    while (1)
    {
        // puts("Listening...");
        fflush(stdout);
        recvBytes = recvfrom(listenFd, &packet, sizeof(emergencyPacket), 0, (sockaddr *)&senders, (socklen_t *)&recvAddrBytes);
        printf("%s[RECEIVE] Received %d bytes from %s%s\n", LOG_CYAN, recvBytes, inet_ntoa(senders.sin_addr), LOG_RESET);
        fflush(stdout);

        // cacheCheck()
        int16_t location = containsBuff(&cache, packet.hmac_sig);
        if (location == -1)
        {
            addToBuff(&cache, packet.hmac_sig);
            packet.flags = packet.flags & (0xFF << 5);
            printf("%s[FINISH] Sending %d bytes to the STS servers!%s\n", LOG_MAGENTA, recvBytes, LOG_RESET);
            fflush(stdout);
        }
        else
        {
            printf("%s[DROP] Package already processed (Hash on row %d). Dropping.%s\n", LOG_YELLOW, location, LOG_RESET);
            fflush(stdout);
        }
    }
    return 0;
}