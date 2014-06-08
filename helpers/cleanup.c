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

    char* iface = malloc(sizeof(char) * strlen(argv[1]));
    strcpy(iface, argv[1]);
    printf("%s\n", argv[2]);
    char* ip = malloc(sizeof(char) * strlen(argv[2]));
    strcpy(ip, argv[2]);

    char* command = malloc(sizeof(char) * 2000);
    memset(command, 0, sizeof(char) * 2000);
    strcat(command, "ifconfig ");
    strcat(command, iface);
#ifdef __APPLE__
    strcat(command, " inet6 remove ");
#elif __GNUC__
    strcat(command, " inet6 del ");
#endif
    strcat(command, ip);
    strcat(command, "\0");
    printf("%s\n", command);
    system(command);

    // remove from routing table
#ifdef __APPLE__
    memset(command, 0, sizeof(char) * 2000);
    strcat(command, "route delete -inet6 ");
    strcat(command, ip);
    strcat(command, " -ifscope ");
    strcat(command, iface);
    system(command);
#endif

    return 0;
}