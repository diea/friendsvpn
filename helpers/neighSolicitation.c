// This is an example program from the website www.microhowto.info
// © 2012 Graham Shaw
// Copying and distribution of this software, with or without modification,
// is permitted in any medium without royalty.
// This software is offered as-is, without any warranty.

// Modified for my purposes (send tcp/udp with spoofed ipv6 source) by Pieter Lewyllie
// based on rfc4861

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

#ifdef __APPLE__
#include <ifaddrs.h>
#include <net/if_dl.h>
#endif /* __APPLE__ */

#include "raw_structs.h"

int main(int argc,const char* argv[]) {
    // Get interface name and target IP address from command line.
    if (argc<3) {
        fprintf(stderr,"usage: nbSol <interface> <ipv6-src> <ipv6-dst>\n");
        exit(1);
    }
    const char* if_name=argv[1];
    const char* target_ip_string=argv[2];

    // Construct IPv6 Packet
    struct ipv6hdr ip6;
    printf("sizeof ip6 %d\n", sizeof(ip6));
    memset(&ip6, 0, sizeof(ip6)); // set all to 0
    ip6.ip6_vfc = 6 << 4;
    ip6.ip6_nxt = SOL_ICMPV6;
    ip6.ip6_hlim = TTL;
    ip6.ip6_plen = htons((uint16_t) 32); // icmpv6 size

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
    adret = getaddrinfo("ff02::1", NULL, &hints, &res);
    if (adret) {
        fprintf(stderr, "%s\n", gai_strerror(adret));
        exit(0);
    }
    ip6.ip6_dst = ((struct sockaddr_in6 *) res->ai_addr)->sin6_addr;

    printf("ipv6 constructed!\n");

    // Construct NeighBor soliciation message
    struct neighbor_req nreq;
    printf("size of neighbor_req %d\n", sizeof(nreq));
    memset(&nreq, 0, sizeof(nreq)); // set all to 0

    nreq.type = 135;
    nreq.code = 0;
    nreq.sourceLL.type = 1;
    nreq.sourceLL.length = 1;
    // source ethernet in nreq is set further down

    adret = getaddrinfo(argv[3], NULL, &hints, &res);
    if (adret) {
        fprintf(stderr, "%s\n", gai_strerror(adret));
        exit(0);
    }
    //memcpy(&nreq.targetAddress, res->ai_addr->sin6_addr, sizeof(struct in6_addr));
    nreq.targetAddress = ((struct sockaddr_in6 *) res->ai_addr)->sin6_addr;
    // Write the interface name to an ifreq structure,
    // for obtaining the source MAC and IP addresses.
    struct ifreq ifr;
    size_t if_name_len=strlen(if_name);
    if (if_name_len<sizeof(ifr.ifr_name)) {
        memcpy(ifr.ifr_name,if_name,if_name_len);
        ifr.ifr_name[if_name_len]=0;
    } else {
        fprintf(stderr,"interface name is too long");
        exit(1);
    }

    printf("nreq almost constructed!\n");

    // Construct Ethernet header (except for source MAC address).
    // (Destination set to broadcast address, FF:FF:FF:FF:FF:FF.)
    struct ether_header header;
    header.ether_type=htons(ETH_IPV6);
    //memset(header.ether_dhost,0xff,sizeof(header.ether_dhost));
    header.ether_dhost[0] = 0x33;
    header.ether_dhost[1] = 0x33;
    header.ether_dhost[2] = 0x00;
    header.ether_dhost[3] = 0x00;
    header.ether_dhost[4] = 0x00;
    header.ether_dhost[5] = 0x01;
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
    if (ifr.ifr_hwaddr.sa_family!=ARPHRD_ETHER) {
        fprintf(stderr,"not an Ethernet interface");
        close(fd);
        exit(1);
    }
    const unsigned char* source_mac_addr=(unsigned char*)ifr.ifr_hwaddr.sa_data;
    close(fd);
#endif
    printf("memcpy ether s_host\n");
    memcpy(header.ether_shost,source_mac_addr,sizeof(header.ether_shost));
    printf("memcpy ether s_host out!\n");
    // source ethernet in nreq
    memcpy(nreq.sourceLL.ether_shost, source_mac_addr,sizeof(header.ether_shost));

    // checksum of nreq
    // begin by making pseudo header
    struct ipv6upper pHeader;
    pHeader.ip6_src = ip6.ip6_src;
    pHeader.ip6_dst = ip6.ip6_dst;
    pHeader.payload_len = htonl(32);
    pHeader.nextHeader = ip6.ip6_nxt;

    void* bufferChecksum = malloc(sizeof(struct ipv6upper) + sizeof(struct neighbor_req));
    memcpy(bufferChecksum, &pHeader, sizeof(struct ipv6upper));
    memcpy(bufferChecksum + sizeof(struct ipv6upper), &nreq, sizeof(struct neighbor_req));

    nreq.checksum = ~(checksum(bufferChecksum, sizeof(struct ipv6upper) + sizeof(struct neighbor_req)));

    // Combine the Ethernet header IP and ICMPv6 request into a contiguous block.
    unsigned char frame[sizeof(struct ether_header)+sizeof(struct ipv6hdr)+sizeof(struct neighbor_req)];
    memcpy(frame, &header ,sizeof(struct ether_header));
    memcpy(frame + sizeof(struct ether_header), &ip6 ,sizeof(struct ipv6hdr));
    memcpy(frame + sizeof(struct ether_header) + sizeof(struct ipv6hdr), &nreq, sizeof(struct neighbor_req));

    // Open a PCAP packet capture descriptor for the specified interface.
    char pcap_errbuf[PCAP_ERRBUF_SIZE];
    pcap_errbuf[0]='\0';
    pcap_t* pcap=pcap_open_live(if_name,96,0,0,pcap_errbuf);
    if (pcap_errbuf[0]!='\0') {
        fprintf(stderr,"%s\n",pcap_errbuf);
    }
    if (!pcap) {
        exit(1);
    }

    // Write the Ethernet frame to the interface.
    if (pcap_inject(pcap,frame,sizeof(frame))==-1) {
        pcap_perror(pcap,0);
        pcap_close(pcap);
        exit(1);
    }

    // Close the PCAP descriptor.
    pcap_close(pcap);
    return 0;
}
