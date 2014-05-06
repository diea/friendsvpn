#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <strings.h>

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
    struct sockaddr_in6 serv_addr;
    int fd = socket(AF_INET6, atoi(argv[1]), atoi(argv[2]));
    if (fd < 0) {
        return 2;
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    int portno = atoi(argv[3]);
    inet_pton(AF_INET6, (const char *) &argv[4], &(serv_addr.sin6_addr));
    serv_addr.sin6_family = AF_INET6;
    serv_addr.sin6_port = htons(portno);
    if (bind(fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        fprintf(stderr, "error on bind\n");
        return 3;
    }
    fprintf(stderr, "all is well!\n");

    getchar(); // press any key to quit
    
    return 0;
}
