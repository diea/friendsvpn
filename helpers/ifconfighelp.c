#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>


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

  	char command[2000];
  	strcat(command, "ifconfig \0");
  	strcat(command, argv[1]);
  	strcat(command, " inet6 add \0");
  	strcat(command, argv[2]);

  	printf("%s\n", command);

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

	return 0;
}