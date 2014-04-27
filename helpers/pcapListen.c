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
#include "rawSocketStructs.h"

int datalink; /* stores the datalink used for the capture */

/**
 * will print the bytes of tcp/udp header + payload so that main app can use those
 */
void print_packet(const u_char *payload, int len) {
	printf("[%d]", len); // print length
	fwrite(payload, 1, len, stdout);
	fflush(stdout);
}

void got_packet(u_char* args, const struct pcap_pkthdr *header, const u_char *packet) {
	/* declare pointers to packet headers */
	const struct sniff_ethernet *ethernet;  /* The ethernet header [1] */
	const struct sniff_loopback *loopback;
	const struct sniff_ipv6 *ip;              /* The IP header */
	const struct sniff_tcp *tcp;            /* The TCP header */
	const char *payload;                    /* Packet payload */
	
	int size_datalink;
	int size_ipv6 = 40; /* ipv6 header has fixed size of 40 bytes */
	int size_tcp;
	int size_payload;

	if (datalink == DLT_EN10MB) {
		/* define ethernet header */
		ethernet = (struct sniff_ethernet*)(packet);
		size_datalink = SIZE_ETHERNET;
	} else {
		loopback = (struct sniff_loopback*)(packet);
		size_datalink = SIZE_NULL;
	}

	/* define/compute ip header offset */
	ip = (struct sniff_ipv6*)(packet + size_datalink);

	if (ip->ip_nxt == IPPROTO_TCP) {
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
		size_payload = ntohs(ip->ip_len);
		print_packet(payload, size_payload);
	} else if (ip->ip_nxt == IPPROTO_UDP) {
		/* handle UDP */
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

	char *dev = NULL;			/* capture device name */
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
		return -1;
	}

	/* open capture device */
	handle = pcap_open_live(dev, SNAP_LEN, 1, 1000, errbuf);
	if (handle == NULL) {
		fprintf(stderr, "Couldn't open device %s: %s\n", dev, errbuf);
		return -3;
	}
	/* make sure we're capturing on an Ethernet device [2] */
	datalink = pcap_datalink(handle);
	if (datalink != DLT_EN10MB && datalink != DLT_NULL) { // only ethernet or loopback
		fprintf(stderr, "%s is not an Ethernet or loopback\n", dev);
		return -3;
	}

	/* compile the filter expression */
	if (pcap_compile(handle, &fp, filter_exp, 0, 0) == -1) {
		fprintf(stderr, "Couldn't parse filter %s: %s\n",
		    filter_exp, pcap_geterr(handle));
		return -4;
	}

	/* apply the compiled filter */
	if (pcap_setfilter(handle, &fp) == -1) {
		fprintf(stderr, "Couldn't install filter %s: %s\n",
		    filter_exp, pcap_geterr(handle));
		return -3;
	}

	/* now we can set our callback function */
	pcap_loop(handle, 0, got_packet, NULL);

	/* cleanup */
	pcap_freecode(&fp);
	pcap_close(handle);

	fprintf(stderr, "\nCapture complete.\n");
}