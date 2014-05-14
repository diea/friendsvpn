/**
 * C Helper for the friendsvpn project.
 * Inspired by www.tcpdump.org/sniffex.c
 */
#include <pcap.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <net/ethernet.h>
#include <unistd.h>

#include "raw_structs.h"

int datalink; /* stores the datalink used for the capture */
char *dev = NULL;			/* capture device name */

/**
 * will print the bytes of tcp/udp header + payload so that main app can use those
 */
void print_packet(const u_char *payload, int len, char* ipSrcStr, char* sourceMacStr) {
	struct pcapComHeader pcapHeader;
	memset(&pcapHeader, 0, sizeof(struct pcapComHeader));
	strcpy(pcapHeader.dev, dev);
	pcapHeader.len = len;
	strcpy(pcapHeader.ipSrcStr, ipSrcStr);
	strcpy(pcapHeader.sourceMacStr, sourceMacStr);

	void* printBuf = malloc(len + sizeof(struct pcapComHeader));
	memcpy(printBuf, &pcapHeader, sizeof(struct pcapComHeader));
	memcpy(printBuf + sizeof(struct pcapComHeader), payload, len);
	fwrite(printBuf, 1, len + sizeof(struct pcapComHeader), stdout);
	fflush(stdout);
	free(printBuf);
}

void got_packet(u_char* args, const struct pcap_pkthdr *header, const u_char *packet) {
	/* declare pointers to packet headers */
	const struct ether_header *ethernet;  /* The ethernet header */
	const struct ipv6hdr *ip;              /* The IP header */
	const struct sniff_tcp *tcp;            /* The TCP header */
	const char *payload;                    /* Packet payload */
	
	char ipSrcStr[INET6_ADDRSTRLEN];
	char sourceMacStr[18];
	sourceMacStr[0] = '\0'; // so strlen returns 0 if not used.

	int size_datalink;
	int size_ipv6 = 40; /* ipv6 header has fixed size of 40 bytes */
	int size_tcp;
	int size_payload;

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

		/* define/compute tcp payload (segment) offset */
		payload = (u_char *)(packet + size_datalink + size_ipv6);
		
		/* compute tcp payload (segment) size */
		size_payload = ntohs(ip->ip6_plen);
		print_packet(payload, size_payload, ipSrcStr, sourceMacStr);
	} else if (ip->ip6_nxt == IPPROTO_UDP) {
		/* handle UDP TODO */
	}
}

/**
 * -1 : wrong arguments
 * -3 : pcap error
 * -4 : pcap cannot compile filter
 *
 * usage: pcapListen iface filter
 */
int main(int argc, char** argv) {
	setuid(0); // for linux

	char errbuf[PCAP_ERRBUF_SIZE];		/* error buffer */
	pcap_t *handle;				/* packet capture handle */

	char* filter_exp = NULL; //"ip6 host fd3b:e180:cbaa:1:a9ed:f7b:5cb:10bd and tcp";
	struct bpf_program fp;			/* compiled filter program (expression) */
	//int num_packets = 99999999;			/* number of packets to capture */

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

	/* cleanup */
	pcap_freecode(&fp);
	pcap_close(handle);

	fprintf(stderr, "\nCapture complete.\n");
}