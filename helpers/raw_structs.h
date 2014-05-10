#define ETH_IPV6 0x86dd

#define SOL_TCP         6
#define SOL_UDP         17
#define SOL_ICMPV6 58

#define TTL 255

/* loopback header */
#define SIZE_NULL 4

/* default snap length (maximum bytes per packet to capture) */
#define SNAP_LEN 1518

/* ethernet headers are always exactly 14 bytes [1] */
#define SIZE_ETHERNET 14
/* Ethernet addresses are 6 bytes */
#define ETHER_ADDR_LEN  6


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

/**
 * Compute tcp/udp checksum from buffer (usually pseudoheader + ...)
 *
 * This function's code is from http://stackoverflow.com/questions/14936518/calculating-checksum-of-icmpv6-packet-in-c
 */
uint16_t checksum (void * buffer, int bytes) {
   uint32_t   total;
   uint16_t * ptr;
   int        words;

   total = 0;
   ptr   = (uint16_t *) buffer;
   words = (bytes + 1) / 2; // +1 & truncation on / handles any odd byte at end

   /*
    *   As we're using a 32 bit int to calculate 16 bit checksum
    *   we can accumulate carries in top half of DWORD and fold them in later
    */
   while (words--) total += *ptr++;

   /*
    *   Fold in any carries
    *   - the addition may cause another carry so we loop
    */
   while (total & 0xffff0000) total = (total >> 16) + (total & 0xffff);

   return (uint16_t) total;
}