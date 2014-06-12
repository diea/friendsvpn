#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
  /**
   * usage: ifconfighelp iface ipv6
   *
   * return: 1 wrong arguments
   * return: 2 unable to run dmesg
   * return: 0 success or duplicate
   */
int main(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, "Need interface & ip as argument\n");
        return 1;
    }
    setuid(0);

    char* iface = malloc(sizeof(char) * strlen(argv[1]));
    strcpy(iface, argv[1]);
    char* ip = malloc(sizeof(char) * strlen(argv[2]));
    strcpy(ip, argv[2]);

    char* command = malloc(sizeof(char) * 2000);
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
    *found = '\0'; // terminate the IP, don't bother the prefix for deletion

#if 0 /* DAD disabled for tests */
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
        if ((strstr(line, "duplicate") != NULL) && (strstr(line, ip) != NULL)) {
            return 3;
        }
    }
    fflush(stdout);
    pclose(fp);
#endif
    return 0;
}