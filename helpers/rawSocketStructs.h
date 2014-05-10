/* default snap length (maximum bytes per packet to capture) */
#define SNAP_LEN 1518

/* ethernet headers are always exactly 14 bytes [1] */
#define SIZE_ETHERNET 14

/* Ethernet addresses are 6 bytes */
#define ETHER_ADDR_LEN	6

struct sniff_loopback {
    u_char family; /* not sure if works but not used */
};
/* Ethernet header */
struct sniff_ethernet {
    u_char  ether_dhost[ETHER_ADDR_LEN];    /* destination host address */
    u_char  ether_shost[ETHER_ADDR_LEN];    /* source host address */
    u_short ether_type;                     /* IP? ARP? RARP? etc */
};

/* IPv6 header. RFC 2460, section3. Reading /usr/include/netinet/ip6.h is
 * interesting */
struct sniff_ipv6 {
    uint32_t        ip_vtcfl;   /* version << 4 then traffic class and flow label */
    uint16_t        ip_len;     /* payload length */
    uint8_t         ip_nxt;     /* next header (protocol) */
    uint8_t         ip_hopl;    /* hop limit (ttl) */
    struct in6_addr ip_src, ip_dst;     /* source and dest address */
};
#define IPV6_VERSION(ip)          (ntohl((ip)->ip_vtcfl) >> 28)

/* TCP header */
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
};

/* UDP header */
struct sniff_udp {
    uint16_t        sport;      /* source port */
    uint16_t        dport;      /* destination port */
    uint16_t        udp_length;
    uint16_t        udp_sum;    /* checksum */
};
