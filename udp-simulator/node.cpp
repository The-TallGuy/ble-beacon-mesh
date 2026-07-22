#include "emergencyPacket.h"
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <endian.h>
#include "lib/circularBuff.h"
#include <ifaddrs.h>

// TERMINAL COLORING
#define LOG_RESET "\033[0m"
#define LOG_GREEN "\033[42;1;37m"  // Green background, bold white text
#define LOG_CYAN "\033[46;1;37m"   // Cyan background, bold white text
#define LOG_YELLOW "\033[43;1;37m" // Yellow background, bold white text
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

    // BROADCASTING

    /*
        struct emergencyPacket Spacket;
        memset(&Spacket, 0, sizeof(emergencyPacket));
    */

    int sendFd = socket(PF_INET, SOCK_DGRAM, 0);
    int broadcast = 1;
    setsockopt(sendFd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));

    /*
        / Good code, but it only send through the eth0 interface. Not good for the UDP simulations. See solution below
        struct sockaddr_in listeners;
        listeners.sin_family = AF_INET;               // 1 Byte field. No need to worry abt endianness
        listeners.sin_addr.s_addr = INADDR_BROADCAST; // Also needs to have network's endianness, but since it's literally all 1s, it's the same either way
        listeners.sin_port = htons(SERVERPORT);
        memset(listeners.sin_zero, 0, sizeof(listeners.sin_zero));
    */

    int sentBytes;

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
            uint8_t ttl = packet.flags & (0xFF >> 3);
            if (ttl > 0) // Might not be necesary, but too afraid to remove atp
            {
                ttl -= 1;
                if (ttl != 0)
                {
                    packet.flags = (packet.flags & (0xFF << 5)) | ttl;

                    struct ifaddrs *ifAddrList, *ifAddrItem;
                    if (getifaddrs(&ifAddrList) == 0)
                    {
                        // Loop through every network interface attached to this container
                        for (ifAddrItem = ifAddrList; ifAddrItem != NULL; ifAddrItem = ifAddrItem->ifa_next)
                        {
                            if (ifAddrItem->ifa_addr == NULL)
                                continue;

                            // We only care about IPv4 interfaces
                            if (ifAddrItem->ifa_addr->sa_family == AF_INET)
                            {
                                struct sockaddr_in *sa = (struct sockaddr_in *)ifAddrItem->ifa_broadaddr;

                                if (sa != NULL)
                                {
                                    // Do not broadcast back onto the container's loopback interface
                                    if (strcmp(ifAddrItem->ifa_name, "lo") == 0)
                                        continue;

                                    sa->sin_port = htons(SERVERPORT);

                                    // Send a copy of the packet specifically to this subnet's broadcast address
                                    sentBytes = sendto(sendFd, &packet, sizeof(emergencyPacket), 0, (sockaddr *)sa, sizeof(*sa));
                                    printf("%s[FORWARD] Rebroadcasting %d bytes out %s to %s%s\n", LOG_GREEN, sentBytes, ifAddrItem->ifa_name, inet_ntoa(sa->sin_addr), LOG_RESET);
                                    fflush(stdout);
                                }
                            }
                        }
                        freeifaddrs(ifAddrList);
                    }
                }
                else
                {
                    printf("%s[DROP] Package TTL has expired!%s\n", LOG_YELLOW, LOG_RESET);
                    fflush(stdout);
                }
            }
        }
        else
        {
            printf("%s[DROP] Package already processed (Hash on row %d). Dropping.%s\n", LOG_YELLOW, location, LOG_RESET);
            fflush(stdout);
        }
        // sleep(1);
        //! I THINK we don't need this. recvfrom() is blocking already, soo...; Plus we already have the logs
    }
    close(sendFd);
    return 0;
}