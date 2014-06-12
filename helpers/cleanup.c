#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
/**
 * Helper used to clean-up IPs when they are not needed anymore
 * will remove the IP from the interface and remove the route to it
 */
int main(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, "usage: %s iface ip\n", argv[0]);
        return 1;
    }
    setuid(0);

    char* command = malloc(sizeof(char) * 2000);
    memset(command, 0, sizeof(char) * 2000);
    strcat(command, "ifconfig ");
    strcat(command, argv[1]);
#ifdef __APPLE__
    strcat(command, " inet6 remove ");
#elif __GNUC__
    strcat(command, " inet6 del ");
#endif
    strcat(command, argv[2]);
#ifndef __APPLE__
    strcat(command, "/64");
#endif
    system(command);

    // remove from routing table
#ifdef __APPLE__
    memset(command, 0, sizeof(char) * 2000);
    strcat(command, "route delete -inet6 ");
    strcat(command, argv[2]);
    strcat(command, " -ifscope ");
    strcat(command, argv[1]);
    system(command);
#endif

    return 0;
}