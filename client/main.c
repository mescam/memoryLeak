#include <ctype.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>


#define BUFSIZE 1024

void
get_file(char *filename, int sck)
{
	char buffer[BUFSIZE]; // buffer for incoming data
	memset(buffer, 0, BUFSIZE);
	
	// build command
	char *cmd = malloc((strlen(filename) + 6) * sizeof(char));
	memset(cmd, 0, (strlen(filename) + 6));
	strcpy(cmd, "GET ");
	strcat(cmd, filename);
	printf("cmd: %s\n", cmd);
	// send command and get output
	write(sck, cmd, strlen(cmd) + 1);
	free(cmd);
	// get output
	char r;
	int i = 0;
	do {
		read(sck, &r, 1);
		buffer[i++] = r;
	} while (r != '\n');

	if (buffer[0] == 'O') { // it's OK
		unsigned long long bytes;
		sscanf(buffer + 3, "%llu\n", &bytes);
		int fd = open(filename, O_WRONLY | O_CREAT);
		int n;
		while (bytes > 0) {
			n = read(sck, buffer, BUFSIZE);
			bytes -= n;
			write(fd, buffer, n);
		}
		printf("%s saved\n", filename);
	} else {
		printf("%s not found!\n", filename);
	}

}

int 
main(int argc, char **argv)
{
	if (argc < 4) {
		printf("Usage: ./memClient address port file [file...]\n");
		return 1;
	}

	struct sockaddr_in sck_addr;
	int sck; // socket fd
	int i;
	
	memset (&sck_addr, 0, sizeof sck_addr);
	sck_addr.sin_family = AF_INET;
	inet_aton (argv[1], &sck_addr.sin_addr);
	sck_addr.sin_port = htons(atoi(argv[2]));

	if ((sck = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		perror ("Cannot create socket");
		return 2;
	}

	if (connect (sck, (struct sockaddr*) &sck_addr, sizeof sck_addr) < 0) {
		perror ("Cannot connect to socket");
		return 3;
	}

	// at this moment we are connected to server
	for (i = 3; i < argc; ++i) {
		// get file
		get_file(argv[i], sck);
	}
	close(sck);

	return 0;
}