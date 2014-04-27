#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/un.h>
#include <sys/socket.h>

void write_text (int socket_fd, const char* text) {

   /* Write the number of bytes in the string, including 

      NUL-termination.  */ 

   int length = strlen (text) + 1; 

   write (socket_fd, &length, sizeof (length)); 

   /* Write the string.  */ 

   write (socket_fd, text, length);
} 

int main() {
	const char* const socket_name = "/tmp/rawSocketUnix"
	const char* const message = "test unix";

	int socket_fd;
	struct sockaddr_un name;
 

   /* Create the socket.  */ 

   socket_fd = socket (PF_LOCAL, SOCK_STREAM, 0); 

   /* Store the server's name in the socket address.  */ 

   name.sun_family = AF_LOCAL; 

   strcpy (name.sun_path, socket_name); 

   /* Connect the socket.   */ 

   connect (socket_fd, &name, SUN_LEN (&name)); 

   /* Write the text on the command line to the socket.   */ 

   write_text (socket_fd, message); 

   close (socket_fd); 

    /*setuid(0);
  	printf("uid %d\n", getuid());
  	printf("eUid %d\n", geteuid());*/


	return 0;
}