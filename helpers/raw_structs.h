/**
  * This file contains the structures used by the helpers. It also contains the communication
  * headers to communicate with those helpers over stdin and stdout.
  */
#ifndef RAWSTRUCTS_H
#define RAWSTRUCTS_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pcap.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <netdb.h>
#include <ctype.h>
#include <stdbool.h>

#ifdef __APPLE__
#include <ifaddrs.h>
#include <net/if_dl.h>
#endif /* __APPLE__ */

#define ETH_IPV6 0x86dd

#define SOL_TCP    6
#define SOL_UDP    17
#define SOL_FRAG   44
#define SOL_ICMPV6 58

#define TTL 255
#define ICMPREQ_HOP 64

/* loopback header on OSX */
#define SIZE_NULL 4

/* default snap length (maximum bytes per packet to capture) */
#define SNAP_LEN 65535

/* ethernet headers are always exactly 14 bytes */
#define SIZE_ETHERNET 14
/* Ethernet addresses are 6 bytes */
#define ETHER_ADDR_LEN  6

#define ARPHRD_ETHER 1

struct pcapComHeader { /* used to communicate with main Qt app */
    char dev[10]; /* inteface name on which packet was received */
    uint32_t len; /* payload length */
    char ipSrcStr[INET6_ADDRSTRLEN]; /* the source IP from the captured packet */
    char sourceMacStr[18]; /* the source MAC from the captured packet */
} __attribute__((__packed__));

struct	ether_header {
    u_char	ether_dhost[ETHER_ADDR_LEN];
    u_char	ether_shost[ETHER_ADDR_LEN];
    u_short	ether_type;
};
struct loopbackHeader {
    uint32_t type;
} __attribute__((__packed__));

struct ipv6upper { /* IPv6 pseudo-header used for ICMPv6 checksum, described in RFC 2460 sec 8.1 */
    struct in6_addr ip6_src;    /* source address */
    struct in6_addr ip6_dst;    /* destination address */
    uint32_t payload_len;

    // 24 zero bits
    uint16_t nothing;
    uint8_t nothing1;

    uint8_t nextHeader; // only the last 8 bits
} __attribute__((__packed__));

struct ipv6hdr { /* as defined on OSX */
    union {
        struct ip6_hdrctl {
            u_int32_t ip6_un1_flow; /* 20 bits of flow-ID */
            u_int16_t ip6_un1_plen; /* payload length */
            u_int8_t  ip6_un1_nxt;  /* next header */
            u_int8_t  ip6_un1_hlim; /* hop limit */
        } ip6_un1;
        u_int8_t ip6_un2_vfc;   /* 4 bits version, top 4 bits class */
    } ip6_ctlun;
    struct in6_addr ip6_src;    /* source address */
    struct in6_addr ip6_dst;    /* destination address */
} __attribute__((__packed__));
#define ip6_vfc     ip6_ctlun.ip6_un2_vfc
#define ip6_flow    ip6_ctlun.ip6_un1.ip6_un1_flow
#define ip6_plen    ip6_ctlun.ip6_un1.ip6_un1_plen
#define ip6_nxt     ip6_ctlun.ip6_un1.ip6_un1_nxt
#define ip6_hlim    ip6_ctlun.ip6_un1.ip6_un1_hlim
#define ip6_hops    ip6_ctlun.ip6_un1.ip6_un1_hlim


/* Neighbor discovery structures */
struct sourceLLoption {
    uint8_t type;
    uint8_t length;
    u_char ether_shost[ETHER_ADDR_LEN]; // link layer addr
} __attribute__((__packed__));

struct inverseNeighbor_req { /* rfc3122 */
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint32_t reserved;
    struct sourceLLoption sourceLL;
    struct sourceLLoption destLL;
} __attribute__((__packed__));

struct neighbor_req {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint32_t reserved;
    struct in6_addr targetAddress;
    struct sourceLLoption sourceLL;
} __attribute__((__packed__));

struct icmpv6_req {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint16_t id;
    uint16_t seqnb;
} __attribute__((__packed__));

/* TCP header from sniffex.c, tcpdump.org */
typedef u_int tcp_seq;

struct sniff_tcp {
        u_short th_sport;               /* source port */
        u_short th_dport;               /* destination port */
        tcp_seq th_seq;                 /* sequence number */
        tcp_seq th_ack;                 /* acknowledgement number */
        u_char  th_offx2;               /* data offset, rsvd */
#define TH_OFF(th)      (((th)->th_offx2 & 0xf0) >> 4)
        u_char  th_flags;
        #define TH_FIN  0x01
        #define TH_SYN  0x02
        #define TH_RST  0x04
        #define TH_PUSH 0x08
        #define TH_ACK  0x10
        #define TH_URG  0x20
        #define TH_ECE  0x40
        #define TH_CWR  0x80
        #define TH_FLAGS        (TH_FIN|TH_SYN|TH_RST|TH_ACK|TH_URG|TH_ECE|TH_CWR)
        u_short th_win;                 /* window */
        u_short th_sum;                 /* checksum */
        u_short th_urp;                 /* urgent pointer */
} __attribute__((__packed__));

/* UDP header */
struct sniff_udp {
    uint16_t        sport;      /* source port */
    uint16_t        dport;      /* destination port */
    uint16_t        udp_length;
    uint16_t        udp_sum;    /* checksum */
} __attribute__((__packed__));

struct rawComHeader { /* used to communicate with main Qt app */
    union {
        struct loopbackHeader loopback;
        struct ether_header ethernet;
    } linkHeader; /* contains the link-layer header */
    struct ipv6hdr ip6; /* contains the ipv6 header */
    uint32_t payload_len; /* contains the payload length */
} __attribute__((__packed__));

struct icmpv6TooBig { /* represents an ICMP Packet Too Big as defined in RFC 4443 */
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint32_t mtu;
} __attribute__((__packed__));

struct fragHeader {
    uint8_t nextHeader;
    uint8_t reserved;
    uint16_t fragOffsetResAndM; /* 13 bit frag offset + 2 res bit + M flag */
    uint32_t identification;
} __attribute__((__packed__));

/**
 * Compute tcp/udp checksum from buffer (usually pseudoheader + ...)
 *
 * This function's code is from http://stackoverflow.com/questions/14936518/calculating-checksum-of-icmpv6-packet-in-c
 */
uint16_t checksum (void * buffer, int bytes);

#endif
