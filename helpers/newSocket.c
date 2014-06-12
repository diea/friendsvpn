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
 * needs to be run as root on linux only
 *
 * return: 1 wrong arguments
 * return: 2 unable to create socket
 * return: 3 unable to bind
 */
int main(int argc, char** argv) {
    setuid(0);

    if (argc != 5) {
        printf("Wrong nb of args\n");
        fflush(stdout);
        return 1;
    }

    // create socket and bind for the kernel
    int fd = socket(AF_INET6, atoi(argv[1]), atoi(argv[2]));
    if (fd < 0) {
        printf("Could not create socket\n");
        fflush(stdout);
        return 2;
    }

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET6;
    hints.ai_socktype = atoi(argv[1]);

    int portno =  atoi(argv[3]);
    int server = getaddrinfo(argv[4], NULL, &hints, &res);
    if (server) {
        printf("ERROR, no such host\n");
        fflush(stdout);
        return 5;
    }
    ((struct sockaddr_in6*) res->ai_addr)->sin6_port = htons(portno);

    if (bind(fd, res->ai_addr, sizeof(struct sockaddr_in6)) < 0) {
        if (errno == EADDRNOTAVAIL) { // "address" is taken, need to wait a bit more
            printf("error on bind %d\n", errno);
            fflush(stdout);
            return EADDRNOTAVAIL;
        }
        printf("error on bind %d\n", errno);
        fflush(stdout);
        return errno;
    }

    printf("OK\n");
    fflush(stdout);

#ifdef linux /* linux */
    /* on linux the "bind" trick does not work due to bind's implementation needing a "listen"
     * to accept connections, we will thus prevent the kernel from sending its RST using an ip6tables
     * rule
     * we still bind on linux to prevent other applications from binding on the same port
     */

    char* ip6tablesRule = malloc(400 * sizeof(char));
    sprintf(ip6tablesRule, "ip6tables -A OUTPUT -s %s -p tcp --sport %s --tcp-flags RST RST -j DROP", argv[4], argv[3]);
    system(ip6tablesRule);
#endif

    getchar();

    close(fd);

#ifdef linux
    ip6tablesRule[11] = 'D'; // replace append by 'D' for delete in the command
    system(ip6tablesRule);
#endif

    return 0;
}
