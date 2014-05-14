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
    if (!command) return;

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

    printf("%s \n", delCommand);
    
    system(delCommand);
    exit(0);
}

/**
 * usage: iface ipv6
 *
 * return: 1 wrong arguments
 * return: 2 unable to run dmesg
 *
 */
int main(int argc, char** argv) {
	if (argc != 3) {
		fprintf(stderr, "Need interface & ip as argument\n");
		return 1;
	}
  	setuid(0);
    
    struct sigaction sa;
    sa.sa_handler = &sig_handler;
    sa.sa_flags = SA_RESTART;
    sigfillset(&sa.sa_mask);
    if ((sigaction(SIGINT, &sa, NULL) == -1) || (sigaction(SIGTERM, &sa, NULL) == -1)) {
        fprintf(stderr, "Cannot handle SIGINT or SIGTERM!\n");
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
#if 0
	  char line[2000];
  	/* Read the output a line at a time - output it. */
  	FILE *fp;
  	int sec = 2; // check 2seconds for DAD
  	while (sec > 2) {
		fp = popen("dmesg", "r");
		if (fp == NULL) {
			fprintf(stderr, "Unable to run dmesg\n");
			return 2;
		}
  		while (fgets(line, sizeof(line) - 1, fp) != NULL) {
  			// TODO CONTINUE HERE
    		printf("%s", line);
 		}
 		fflush(stdout);
 		pclose(fp);
 		sleep(1);
 		sec--;
  	}
  	/* close */
  	pclose(fp);
#endif

  getchar(); // wait before leaving!

	return 0;
}