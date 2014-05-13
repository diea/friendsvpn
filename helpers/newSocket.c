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
#include <signal.h> // sigaction(), sigsuspend(), sig*()

char* ip6tablesRule = NULL;

void sig_handler(int signal) {
    /* used on linux to delete the ip6tables rule */
    if (!ip6tablesRule) return;

    ip6tablesRule[11] = 'D'; // replace append by 'D' for delete in the command
    system(ip6tablesRule);
    exit(0);
}

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
    int server = getaddrinfo(argv[4], atoi(argv[3]), &hints, &res);
    if (server) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    //int portno = atoi(argv[3]); // TODO should be htons ? and maybe not needed but put it in getaddrinfo!
    //((struct sockaddr_in6*) res->ai_addr)->sin6_port = htons(portno);

    if (bind(fd, res->ai_addr, sizeof(struct sockaddr_in6)) < 0) {
        fprintf(stderr, "error on bind %d\n", errno);
        if (errno == EADDRNOTAVAIL) { // "address" is taken, need to wait a bit more 
            return EADDRNOTAVAIL;
        }
        return 3;
    }
#ifndef __APPLE__ /* linux */
    /* on linux the "bind" trick does not work due to bind's implementation needing a "listen"
     * to accept connections, we will thus prevent the kernel from sending its RST using an ip6tables
     * rule
     * we still bind on linux to avoid the ICMPv6 address unreachable message if we begin sending packets using this new
     * IPv6 too quickly
     */
    struct sigaction sa;
    sa.sa_handler = &sig_handler;
    sa.sa_flags = SA_RESTART;
    sigfillset(&sa.sa_mask);
    if ((sigaction(SIGINT, &sa, NULL) == -1) || (sigaction(SIGTERM, &sa, NULL) == -1)) {
        fprintf(stderr, "Cannot handle SIGINT or SIGTERM!\n");
        return 4;
    }

    ip6tablesRule = malloc(400 * sizeof(char));
    sprintf(ip6tablesRule, "ip6tables -A OUTPUT -s %s -p tcp --sport %s --tcp-flags RST RST -j DROP", argv[4], argv[3]);
    system(ip6tablesRule);
#endif
    fprintf(stderr, "all is well!\n");

    getchar(); // press any key to quit
    
    return 0;
}
