#include "raw_structs.h"

void print_bytes(const void *object, size_t size)
{
  size_t i;

  printf("[ ");
  for(i = 0; i < size; i++)
  {
    printf("%02x ", ((const unsigned char *) object)[i] & 0xff);
  }
  printf("]\n");
}

void print_usage() {
    fprintf(stderr,"usage: sendRaw <interface>\n");
    exit(1);
}

int main(int argc,const char* argv[]) {
    setuid(0);
    if (argc < 2) {
        print_usage();
    }
    const char* if_name = argv[1];
    // Open a PCAP packet capture descriptor for the specified interface.
    char pcap_errbuf[PCAP_ERRBUF_SIZE];
    pcap_errbuf[0]='\0';
    pcap_t* pcap = pcap_open_live(if_name,96,0,20,pcap_errbuf);
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

    while (1) {
        struct rawComHeader header;
        int res = fread(&header, sizeof(struct rawComHeader), 1, stdin);
        if (res == 0) {
            exit(4);
        }
        void* packet = malloc(header.payload_len);

        size_t linkHeaderSize;
        if (datalink == DLT_EN10MB) {
            linkHeaderSize = sizeof(struct ether_header);
        } else {
            linkHeaderSize = sizeof(struct loopbackHeader);
        }

        unsigned char frame[linkHeaderSize + sizeof(struct ipv6hdr) + header.payload_len];
        memcpy(frame, &header.linkHeader, linkHeaderSize);
        memcpy(frame + linkHeaderSize, &header.ip6 ,sizeof(struct ipv6hdr));

        fread(frame + linkHeaderSize + sizeof(struct ipv6hdr), header.payload_len, 1, stdin); 

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
