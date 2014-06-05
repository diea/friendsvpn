#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <signal.h>

#include "raw_structs.h"

int datalink;
char* dev = NULL;
pcap_t* handle = NULL;
struct bpf_program fp;

/**
 * will print the bytes of tcp/udp header + payload so that main app can use those
 */
void print_packet(const u_char *payload, uint32_t len, char* ipSrcStr, char* sourceMacStr) {
    struct pcapComHeader pcapHeader;
    memset(&pcapHeader, 0, sizeof(struct pcapComHeader));
    strcpy(pcapHeader.dev, dev);
    pcapHeader.len = len;
    strcpy(pcapHeader.ipSrcStr, ipSrcStr);
    strcpy(pcapHeader.sourceMacStr, sourceMacStr);

    void* printBuf = malloc(len + sizeof(struct pcapComHeader));
    memcpy(printBuf, &pcapHeader, sizeof(struct pcapComHeader));
    memcpy(printBuf + sizeof(struct pcapComHeader), payload, len);
    int fwriteRet = fwrite(printBuf, len + sizeof(struct pcapComHeader), 1, stdout);

    FILE* fp;
    char name[200];
    sprintf(name, "fwrite_val_%d", fwriteRet);
    fp = fopen(name, "w");
    fprintf(fp, "Write return value%d\n", fwriteRet);
    fclose(fp);

    if (len == 0) {
        FILE* fp;
        fp = fopen("got_empty_on_helper", "w");
        fclose(fp);
    }

    FILE* fp;
    sprintf(name, "pcap_helper_capture_%d", pcapHeader.len);
    fp = fopen(name, "w");
    fwrite(payload, 1, len, fp);
    fclose(fp);


    fflush(stdout);
    free(printBuf);
}

void got_packet(u_char* args, const struct pcap_pkthdr *header, const u_char *packet) {
    /* declare pointers to packet headers */
    const struct ether_header *ethernet;  /* The ethernet header */
    const struct ipv6hdr *ip;              /* The IP header */
    const struct sniff_tcp *tcp;            /* The TCP header */
    const u_char *payload;                    /* Packet payload */

    char ipSrcStr[INET6_ADDRSTRLEN];
    char sourceMacStr[18];
    sourceMacStr[0] = '\0'; // so strlen returns 0 if not used.

    int size_datalink;
    int size_ipv6 = 40; /* ipv6 header has fixed size of 40 bytes */
    int size_tcp;
    uint32_t size_payload;

    if (datalink == DLT_EN10MB) {
        /* define ethernet header */
        ethernet = (struct ether_header*)(packet);
        sprintf(sourceMacStr, "%02x:%02x:%02x:%02x:%02x:%02x",
            ethernet->ether_shost[0], ethernet->ether_shost[1], ethernet->ether_shost[2],
            ethernet->ether_shost[3], ethernet->ether_shost[4], ethernet->ether_shost[5]);
        size_datalink = SIZE_ETHERNET;
    } else {
        size_datalink = SIZE_NULL;
    }

    /* define/compute ip header offset */
    ip = (struct ipv6hdr*)(packet + size_datalink);

    struct in6_addr ipSrc = ip->ip6_src;
    inet_ntop(AF_INET6, &(ipSrc), ipSrcStr, INET6_ADDRSTRLEN);

    if (ip->ip6_nxt == IPPROTO_TCP) {
        /* handle TCP */
        /* define/compute tcp header offset */
        tcp = (struct sniff_tcp*)(packet + size_datalink + size_ipv6);
        size_tcp = TH_OFF(tcp) * 4;

        if (size_tcp < 20) {
            fprintf(stderr, "   * Invalid TCP header length: %u bytes\n", size_tcp);
            return;
        }
    }
    /* payload is transport header and its data */
    payload = (u_char *)(packet + size_datalink + size_ipv6);

    /* compute tcp payload (segment) size */
    size_payload = ntohs(ip->ip6_plen);
    print_packet(payload, size_payload, ipSrcStr, sourceMacStr);
}

void sig_handler(int sig) {
    pcap_freecode(&fp);
    if (handle)
        pcap_close(handle);
    exit(0);
}

int main(int argc, char** argv) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = &sig_handler;
    sa.sa_flags = SA_RESTART;
    sigfillset(&sa.sa_mask);

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    char errbuf[PCAP_ERRBUF_SIZE];      /* error buffer */

    char* filter_exp = NULL; //"ip6 host fd3b:e180:cbaa:1:a9ed:f7b:5cb:10bd and tcp";

    if (argc == 3) {
        dev = argv[1];
        filter_exp = argv[2];
    } else {
        fprintf(stderr, "Need to specify interface and filter!\n");
        return 1;
    }

    /* open capture device */
    handle = pcap_open_live(dev, SNAP_LEN, 0, 3, errbuf);
    if (handle == NULL) {
        fprintf(stderr, "Couldn't open device %s: %s\n", dev, errbuf);
        return 3;
    }

    /* make sure we're capturing on an Ethernet device [2] */
    datalink = pcap_datalink(handle);
    if (datalink != DLT_EN10MB && datalink != DLT_NULL) { // only ethernet or loopback
        fprintf(stderr, "%s is not an Ethernet or loopback\n", dev);
        return 4;
    }

    /* compile the filter expression */
    if (pcap_compile(handle, &fp, filter_exp, 0, 0) == -1) {
        fprintf(stderr, "Couldn't parse filter %s: %s\n",
            filter_exp, pcap_geterr(handle));
        return 4;
    }

    /* apply the compiled filter */
    if (pcap_setfilter(handle, &fp) == -1) {
        fprintf(stderr, "Couldn't install filter %s: %s\n",
            filter_exp, pcap_geterr(handle));
        return 3;
    }

    /* now we can set our callback function */
    pcap_loop(handle, 0, got_packet, NULL);

    getchar();
    return 0;
}