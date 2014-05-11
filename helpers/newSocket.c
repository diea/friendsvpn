#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <strings.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>

/**
 * usage: sockType ipProto port addr
 *
 * return: 1 wrong arguments
 * return: 2 unable to create socket
 * return: 3 unable to bind
 */
int main(int argc, char** argv) {
    setuid(0);

    if (argc != 5) return 1;

    // create socket and bind for the kernel
    int fd = socket(AF_INET6, atoi(argv[1]), atoi(argv[2]));
    if (fd < 0) {
        return 2;
    }

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET6;
    hints.ai_socktype = atoi(argv[1]);
    int server = getaddrinfo(argv[4], NULL, &hints, &res);
    if (server) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    int portno = atoi(argv[3]);
    ((struct sockaddr_in6*) res->ai_addr)->sin6_port = htons(portno);

    if (bind(fd, res->ai_addr, sizeof(struct sockaddr_in6)) < 0) {
        fprintf(stderr, "error on bind %d\n", errno);
        if (errno == EADDRNOTAVAIL) { // "address" is taken, need to wait a bit more 
            return EADDRNOTAVAIL;
        }
        return 3;
    }
    fprintf(stderr, "all is well!\n");

    getchar(); // press any key to quit
    
    return 0;
}
