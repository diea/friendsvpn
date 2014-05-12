// This is an example program from the website www.microhowto.info
// © 2012 Graham Shaw
// Copying and distribution of this software, with or without modification,
// is permitted in any medium without royalty.
// This software is offered as-is, without any warranty.

// Modified for my purposes (send tcp/udp with spoofed ipv6 source) by Pieter Lewyllie
// based on rfc4861
// TODO udp

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pcap.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <net/ethernet.h>
#include <netinet/if_ether.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <netdb.h>
#include <ctype.h>
#include <stdbool.h>

#ifdef __APPLE__
#include <ifaddrs.h>
#include <net/if_dl.h>
#endif /* __APPLE__ */

#include "raw_structs.h"

void print_usage() {
    fprintf(stderr,"usage: sendRaw <interface> <ipv6-src> <ipv6-dst> <sockType> <port-src> [<mac-dst>] \n");
    fprintf(stderr,"    mac-dst must be used if Ethernet interface\n");
    exit(1);
}

int main(int argc,const char* argv[]) {
    // Get interface name and target IP address from command line.
    if (argc < 6) {
        print_usage();
    }
    const char* if_name=argv[1];
    const char* target_ip_string=argv[2];

    // Open a PCAP packet capture descriptor for the specified interface.
    char pcap_errbuf[PCAP_ERRBUF_SIZE];
    pcap_errbuf[0]='\0';
    pcap_t* pcap = pcap_open_live(if_name,96,0,0,pcap_errbuf);
    if (pcap_errbuf[0]!='\0') {
        fprintf(stderr,"%s\n",pcap_errbuf);
    }
    if (!pcap) {
        exit(1);
    }

    /* make sure we're capturing on an Ethernet or loopback interface */
    int datalink = pcap_datalink(pcap);
    if (datalink != DLT_EN10MB && datalink != DLT_NULL) { // only ethernet or loopback
        fprintf(stderr, "%s is not an Ethernet or loopback\n", argv[1]);
        return -3;
    }

    void* linkHeader = NULL;
    size_t linkHeaderSize = 0;
    if (datalink == DLT_EN10MB) {
        printf("argc: %d\n", argc);
        bool linuxLoopback = false;
        if (argc < 7) {
            #ifdef __APPLE__
            print_usage();
            #elif __GNUC__
            // special zero-ed out linux ethernet loopback packet
            linuxLoopback = true;
            #endif
        }
        // Construct Ethernet header (except for source MAC address).
        // (Destination set to broadcast address, FF:FF:FF:FF:FF:FF.)
        struct ether_header header;
        memset(&header, 0, sizeof(struct ether_header));
        header.ether_type=htons(ETH_IPV6);

        if (linuxLoopback)
            goto ethLoopback;

        printf("reading mac!\n");
        // read mac-addr from argv[6]
        char *token;
        int i = 0;
        while (((token = strsep(&argv[6], ":")) != NULL) && (i < 6)) {
            header.ether_dhost[i] = strtoul(token, NULL, 16);
            printf("%x\n", header.ether_dhost[i]);
            i++;
        }
        printf("ether dhost set!\n");

    #ifdef __APPLE__
        struct ifaddrs *ifap, *ifaptr;
        unsigned char *ptr;
        const unsigned char* source_mac_addr;
        if (getifaddrs(&ifap) == 0) {
            for(ifaptr = ifap; ifaptr != NULL; ifaptr = (ifaptr)->ifa_next) {
                if (!strcmp((ifaptr)->ifa_name, if_name) && (((ifaptr)->ifa_addr)->sa_family == AF_LINK)) {
                    ptr = (unsigned char *)LLADDR((struct sockaddr_dl *)(ifaptr)->ifa_addr);
                    /*sprintf(macaddrstr, "%02x:%02x:%02x:%02x:%02x:%02x",
                                        *ptr, *(ptr+1), *(ptr+2), *(ptr+3), *(ptr+4), *(ptr+5));*/
                    fprintf(stdout, "%02x:%02x:%02x:%02x:%02x:%02x\n",
                                        *ptr, *(ptr+1), *(ptr+2), *(ptr+3), *(ptr+4), *(ptr+5));
                    break;
                }
            }
            freeifaddrs(ifap);
        }
        source_mac_addr = ptr;
    #elif __GNUC__
        struct ifreq ifr;
        size_t if_name_len=strlen(if_name);
        if (if_name_len<sizeof(ifr.ifr_name)) {
            memcpy(ifr.ifr_name,if_name,if_name_len);
            ifr.ifr_name[if_name_len]=0;
        } else {
            fprintf(stderr,"interface name is too long");
            exit(1);
        }
        // Open an IPv4-family socket for use when calling ioctl.
        int fd=socket(AF_INET,SOCK_DGRAM,0);
        if (fd==-1) {
            perror(0);
            exit(1);
        }
        // Obtain the source MAC address, copy into Ethernet header and ARP request.
        if (ioctl(fd,SIOCGIFHWADDR,&ifr)==-1) {
            perror(0);
            close(fd);
            exit(1);
        }
        if (ifr.ifr_hwaddr.sa_family != ARPHRD_ETHER) {
            fprintf(stderr,"not an Ethernet interface\n");
            close(fd);
            /* On linux ubuntu the loopback is actually an Ethernet interface with the ethernet header
            zeroed out! */
        }
        const unsigned char* source_mac_addr=(unsigned char*)ifr.ifr_hwaddr.sa_data;
        close(fd);
    #endif
        printf("memcpy ether s_host\n");
        memcpy(header.ether_shost,source_mac_addr,sizeof(header.ether_shost));
        printf("memcpy ether s_host out!\n");
ethLoopback:
        linkHeader = (void*) &header;
        linkHeaderSize = sizeof(struct ether_header);
    } else {
        printf("Loopback if!\n");
        struct loopbackHeader loopheader;
        loopheader.type = 0x1E; // ipv6 type
        linkHeader = (void*) &loopheader;
        linkHeaderSize = sizeof(struct loopbackHeader);
    }

    // Construct IPv6 Packet
    struct ipv6hdr ip6;
    printf("sizeof ip6 %d\n", sizeof(ip6));
    memset(&ip6, 0, sizeof(ip6)); // set all to 0
    ip6.ip6_vfc = 6 << 4;
    ip6.ip6_nxt = (atoi(argv[4]) == SOCK_DGRAM) ? SOL_UDP : SOL_TCP;
    ip6.ip6_hlim = TTL;

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET6;
    //hints.ai_socktype = atoi(argv[1]);
    int adret = getaddrinfo(argv[2], NULL, &hints, &res);
    if (adret) {
        fprintf(stderr, "%s\n", gai_strerror(adret));
        exit(0);
    }
    ip6.ip6_src = ((struct sockaddr_in6 *) res->ai_addr)->sin6_addr;
    adret = getaddrinfo(argv[3], NULL, &hints, &res);
    if (adret) {
        fprintf(stderr, "%s\n", gai_strerror(adret));
        exit(0);
    }
    ip6.ip6_dst = ((struct sockaddr_in6 *) res->ai_addr)->sin6_addr;

    printf("ipv6 constructed!\n");

    // checksum of nreq
    // begin by making pseudo header
    struct ipv6upper pHeader;
    pHeader.ip6_src = ip6.ip6_src;
    pHeader.ip6_dst = ip6.ip6_dst;
    pHeader.nextHeader = ip6.ip6_nxt;

    //void* bufferChecksum = malloc(sizeof(struct ipv6upper) + sizeof(struct neighbor_req));
    //memcpy(bufferChecksum, &pHeader, sizeof(struct ipv6upper));
    //memcpy(bufferChecksum + sizeof(struct ipv6upper), &nreq, sizeof(struct neighbor_req));

    //nreq.checksum = ~(checksum(bufferChecksum, sizeof(struct ipv6upper) + sizeof(struct neighbor_req)));

    int sockType = atoi(argv[4]);
    struct sniff_udp* udp = NULL;
    struct sniff_tcp* tcp = NULL;

    // get packet size from stdin
    char nbBuf[20];
    while (1) {
        char c = getchar();
        fprintf(stderr, "got char! %c\n", c);
        unsigned cnt = 0;
        c = getchar();
        while (c != ']') {
            if (!isdigit(c)) {
                fprintf(stderr, "format error, digit required between []\n");
                return 3;
            }
            if (cnt == 19) { 
                fprintf(stderr, "we only have a buffer of 19bytes for packet length, sorry...\n"); 
                return 3; 
            }
            nbBuf[cnt] = c;
            c = getchar();
            cnt++;
        }
        nbBuf[cnt] = '\0';
        uint32_t packet_send_size = atoi(nbBuf) ; // tcp + payload length
        ip6.ip6_plen = htons((uint16_t) packet_send_size) ; // transport + payload length

        void* packet_send = malloc(packet_send_size);
        fprintf(stderr, "going into fread for %d chars\n", packet_send_size);
        fread(packet_send, packet_send_size, 1, stdin);

        if (sockType == SOCK_DGRAM) { // udp has length field, that is used in pseudo header (RFC 2460)
            udp = (struct sniff_udp*) (packet_send);
            pHeader.payload_len = udp->udp_length;

            // change src_port
            udp->sport = htons(atoi(argv[5]));
        } else { // tcp has no length field, so same as ipv6
            pHeader.payload_len = htonl(atoi(nbBuf));

            // change src_port
            tcp = (struct sniff_tcp*) (packet_send);
            tcp->th_sport = htons(atoi(argv[5]));
        }

        printf("packet_send %s\n", (char*)packet_send);
        fflush(stdout);

        // Combine the Ethernet header, IP and tcp/udp into a contiguous block.
        unsigned char frame[sizeof(linkHeaderSize) + sizeof(struct ipv6hdr) + packet_send_size];

        memcpy(frame, linkHeader, linkHeaderSize);
        memcpy(frame + linkHeaderSize, &ip6 ,sizeof(struct ipv6hdr));
        memcpy(frame + linkHeaderSize + sizeof(struct ipv6hdr), packet_send, packet_send_size);

        // Write the frame to the interface.
        if (pcap_inject(pcap, frame, sizeof(frame)) == -1) {
            pcap_perror(pcap,0);
            pcap_close(pcap);
            exit(1);
        }
    }

    // Close the PCAP descriptor.
    pcap_close(pcap);

    return 0;
}
