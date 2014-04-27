#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
  	printf("Hello world\n");

  	setuid(0);
  	printf("uid %d\n", getuid());
  	printf("eUid %d\n", geteuid());
	system("ifconfig en1 inet6 add fd3b:e180:cbaa:1:5e96:9dff:fe8a:8448");

	return 0;
}