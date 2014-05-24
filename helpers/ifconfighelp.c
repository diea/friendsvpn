#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <signal.h>

char* command = NULL;
char* iface = NULL;
char* ip = NULL;

void sig_handler(int signal) {
    /* used on linux to delete the ip6tables rule */
    if (!command) {
        exit(-1);
    }

    char delCommand[2010];
    memset(&delCommand, 0, sizeof(delCommand));

    strcat(delCommand, "ifconfig ");
    strcat(delCommand, iface);
#ifdef __APPLE__
    strcat(delCommand, " inet6 remove ");
#elif __GNUC__
    strcat(delCommand, " inet6 del ");
#endif

    strcat(delCommand, ip);

    system(delCommand);

    exit(0);
}

  /**
   * usage: ifconfighelp iface ipv6
   *
   * return: 1 wrong arguments
   * return: 2 unable to run dmesg
   * return: 4 unable to handle signals
   * return: 0 success or duplicate
   */
int main(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, "Need interface & ip as argument\n");
        return 1;
    }
    setuid(0);

    struct sigaction sa;
    memset(&sa, '\0', sizeof(sigaction));
    sa.sa_handler = &sig_handler;
    sa.sa_flags = SA_RESTART;
    sigfillset(&sa.sa_mask);
    if ((sigaction(SIGINT, &sa, NULL)) == -1) {
        fprintf(stderr, "Cannot handle SIGINT!\n");
        return 4;
    }
    if ((sigaction(SIGTERM, &sa, NULL)) == -1) {
        fprintf(stderr, "Cannot handle SIGTERM!\n");
        return 4;
    }

    iface = malloc(sizeof(char) * strlen(argv[1]));
    strcpy(iface, argv[1]);
    ip = malloc(sizeof(char) * strlen(argv[2]));
    strcpy(ip, argv[2]);

    command = malloc(sizeof(char) * 2000);
    memset(command, 0, sizeof(char) * 2000);
    strcat(command, "ifconfig ");
    strcat(command, iface);
    strcat(command, " inet6 add ");
    strcat(command, ip);

    system(command);

    char* found = strstr(ip, "/");
    if (!found) {
        fprintf(stderr, "New IP needs a /something prefix\n");
        return 1;
    }
    char* testIp;
#ifdef __APPLE__
    *found = '\0'; // terminate the IP, don't bother the prefix for deletion
    testIp = ip; // on apple those are the same
#elif __GNUC__ /* on linux the "del" command has to specifiy ip prefix so we can't just \0 the string */
    testIp = malloc(2000);
    strncpy(testIp, ip, found - ip);
    testIp[found - ip] = '\0';
#endif

    char line[2000];
    /* Read the output a line at a time - output it. */
    FILE *fp;
    usleep(3000000); // wait 3000ms before checking 3000ms for DAD, should be enough
    fp = popen("dmesg", "r");
    if (fp == NULL) {
        fprintf(stderr, "Unable to run dmesg\n");
        return 2;
    }
    while (fgets(line, sizeof(line) - 1, fp) != NULL) {
        if ((strstr(line, "duplicate") != NULL) && (strstr(line, testIp) != NULL)) {
            sig_handler(SIGTERM);
        }
    }
    fflush(stdout);
    pclose(fp);

    getchar(); // wait before leaving!
    // got char, exit!
    sig_handler(SIGTERM);

    return 0;
}