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

#ifndef __APPLE__ /* linux */
    /* on linux the "bind" trick does not work due to bind's implementation needing a "listen"
     * to accept connections, we will thus prevent the kernel from sending its RST using an ip6tables
     * rule
     * we still bind on linux to prevent other applications from binding on the same port
     */

    char* ip6tablesRule = malloc(400 * sizeof(char));
    sprintf(ip6tablesRule, "ip6tables -A OUTPUT -s %s -p tcp --sport %s --tcp-flags RST RST -j DROP", argv[2], argv[1]);
    system(ip6tablesRule);
#endif

    return 0;
}
