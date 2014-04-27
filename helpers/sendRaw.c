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

/**
 * usage: SOCK_TYPE IP_PROTO IPv6 
 * 
 * return : 0 = success
 *			1 = wrong arguments
 *			2 = raw socket could not be created
 *		    3 = read stderr
 */
int main(int argc, char** argv) {
	if (argc != 4) {
		return 1;
	}
	int rawsd = socket(AF_INET6, SOCK_RAW, atoi(argv[2]));
	if (rawsd < 0) {
        fprintf(stderr, "Cannot create raw socket: %s\n", strerror(errno));
        return 2;
    }

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET6;
    hints.ai_socktype = atoi(argv[1]);
    int adret = getaddrinfo(argv[3], NULL, &hints, &res);
    if (adret) {
    	fprintf(stderr, "%s\n", gai_strerror(adret));
        exit(0);
    }

    // get packet size from stdin
    char nbBuf[20];

    while (1) {
    	char c = getchar();
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
	    int packet_send_size = atoi(nbBuf) ; // tcp + payload length

	    void* packet_send = malloc(packet_send_size);
	    fread(packet_send, packet_send_size, 1, stdin);


	    int num = sendto(rawsd, packet_send, packet_send_size, 0, res->ai_addr, res->ai_addrlen);
	    if (num < 0) {
	        fprintf(stderr, "Cannot send message (error %d):  %s\n", num, strerror(errno));
	    }
    }

    freeaddrinfo(res);
}