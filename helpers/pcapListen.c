#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>


void signal_handler(int signal) {
	FILE* fp = fopen("/Users/diea/Dropbox/CiscoVPN/app/friendsvpn/helpers/pcapHandler", "w");
	fprintf(fp, "got into pcapHandler\n");
	fclose(fp);

	printf("\nCapture complete.\n");
	fflush(stdout);
    exit(0);
}

/**
 * -1 : wrong arguments
 * -3 : pcap error
 * -4 : pcap cannot compile filter
 *
 * usage: pcapListen iface filter
 */
int main(int argc, char** argv) {
	setuid(0); // for linux

	/* set sig handler to capture SIGINT and SIGTERM */
    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = &signal_handler;
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

	getchar();

	return 0;
}