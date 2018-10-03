// SERVER FOR PROGRAMMING ASSIGNMENT 2
// TCP BASED SIMPLE FTP
// BASE CODE FROM BEEJ'S GUIDE
//
// RYKER McINTYRE rmcinty3
// THOMAS PLUMMER tplumme2
// JOSHUA JOHNSON jjohns54
//
// October 3, 2018


#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

/**
 * TCP Uses 2 types of sockets, the connection socket and the listen socket.
 * The Goal is to separate the connection phase from the data exchange phase.
 * */

int DL(char *, int);
int UP(char *, int);
int RM(char *, int);
int LS(char *, int);
int MKDIR(char *, int);
int RMDIR(char *, int);
int CD(char *, int);


int main(int argc, char *argv[]) {
	// port to start the server on
	// ************* CHANGE TO ACCEPT CMD LINE ARG LATER **************
	int SERVER_PORT = 41032;

	// socket address used for the server
	struct sockaddr_in server_address;
	memset(&server_address, 0, sizeof(server_address));
	server_address.sin_family = AF_INET;

	// htons: host to network short: transforms a value in host byte
	// ordering format to a short value in network byte ordering format
	server_address.sin_port = htons(SERVER_PORT);

	// htonl: host to network long: same as htons but to long
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);

	// create a TCP socket, creation returns -1 on failure
	int listen_sock;
	if ((listen_sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		printf("could not create listen socket\n");
		return 1;
	}

	// bind it to listen to the incoming connections on the created server
	// address, will return -1 on error
	if ((bind(listen_sock, (struct sockaddr *)&server_address, sizeof(server_address))) < 0) {
		fprintf(stderr, "could not bind socket: %s\n", strerror(errno));
		return 1;
	}

	int wait_size = 16;  // maximum number of waiting clients, after which
	                     // dropping begins
	if (listen(listen_sock, wait_size) < 0) {
		printf("could not open socket for listening\n");
		return 1;
	}

	// socket address used to store client address
	struct sockaddr_in client_address;
	int client_address_len = 0;
	
	// Accepting connections message
	printf("Accepting connections on port %d\n", SERVER_PORT);

	// run indefinitely
	while (true) {
		// open a new socket to transmit data per connection
		int sock;
		if ((sock = accept(listen_sock, (struct sockaddr *)&client_address, &client_address_len)) < 0) {
			printf("could not open a socket to accept data\n");
			return 1;
		}

		int n = 0;
		int len = 0, maxlen = 4096;
		char buffer[maxlen];
		char *pbuffer = buffer;
		
		printf("Connection established\n");

		// keep running as long as the client keeps the connection open
		while (1) {
			// Don't know what the following three lines of code are for,
			// but they mess it up I think.  Gonna leave them commented for now.
			//pbuffer += n;
			//maxlen -= n;
			//len += n;
			
			// The following lines were from a simple server DELETE LATER
			/*
			printf("server received: '%s'\n", buffer);

			// echo received content back
			if (send(sock, buffer, len, 0) < 0) {
				printf("server echo error\n");
				return 1;
			}*/
			
			// REceive the command
			memset(pbuffer, '\0', maxlen);
			if ((n = recv(sock, pbuffer, maxlen, 0)) < 0) {
				fprintf(stderr, "recv failed in the server: %s\n", strerror(errno));
				return 1;
			}
			else if (n == 0) {
				break; // client ended connection
			}
			
			// All cases, call functions
			if (strncmp(pbuffer, "DL", 2) == 0) {
				if (DL(pbuffer, sock) != 0) {
					fprintf(stderr, "Server command handling failed\n");
					return 1;
				}
			}
			else if (strncmp(pbuffer, "UP", 2) == 0) {
				if (UP(pbuffer, sock) != 0) {
					fprintf(stderr, "Server command handling failed\n");
					return 1;
				}
				printf("Got UP\n");
			}
			else if (strncmp(pbuffer, "RMDIR", 5) == 0) {
				if (RMDIR(pbuffer, sock) != 0) {
					fprintf(stderr, "Server command handling failed\n");
					return 1;
				}
				printf("Got RMDIR\n");
			}
			else if (strncmp(pbuffer, "LS", 2) == 0) {
				if (LS(pbuffer, sock) != 0) {
					fprintf(stderr, "Server command handling failed\n");
					return 1;
				}
				printf("Got LS\n");
			}
			else if (strncmp(pbuffer, "MKDIR", 5) == 0) {
				if (MKDIR(pbuffer, sock) != 0) {
					fprintf(stderr, "Server command handling failed\n");
					return 1;
				}
				printf("Got MKDIR\n");
			}
			else if (strncmp(pbuffer, "RM", 2) == 0) {
				if (RM(pbuffer, sock) != 0) {
					fprintf(stderr, "Server command handling failed\n");
					return 1;
				}
				printf("Got RM\n");
			}
			else if (strncmp(pbuffer, "CD", 2) == 0) {
				if (CD(pbuffer, sock) != 0) {
					fprintf(stderr, "Server command handling failed\n");
					return 1;
				}
				printf("Got CD\n");
			}
			else {
				continue;
			}
			
			memset(pbuffer, '\0', maxlen);
		}
		close(sock);
	}

	close(listen_sock);
	return 0;
}

int DL(char *cmd, int sock) {
	
	// Receive len of filename and filename
	char *lens;
	if (recv(sock, lens, 4096, 0) < 0) {
		fprintf(stderr, "server recv error\n");
		return 1;
	}
	short int len = (short int)atoi(lens);
	printf("length is %d\n", (int)len);
	char filebuf[len];
	char *file = filebuf;
	if (recv(sock, file, len, 0) < 0) {
		fprintf(stderr, "server recv error\n");
		return 1;
	}
	file[len - 1] = '\0';
	
	// Check if file exists
	printf("file: |%s|...\n", file);
	FILE *fp = fopen(file, "r");
	int neg1 = htonl(-1);
	if (!fp) { // if not, then send -1
		if (send(sock, (char *)&neg1, sizeof(int), 0) < 0) {
			fprintf(stderr, "server send error: %s\n", strerror(errno));
			return 1;
		}
	}
	else { // otherwise, send size of file and md5, then sends file size and file
		// size
		fseek(fp, 0L, SEEK_END);
		int sz = htonl(ftell(fp));
		rewind(fp);
		printf("before send\n");
		if (send(sock, (char *)&sz, sizeof(int), 0) < 0) {
			fprintf(stderr, "server send error: %s\n", strerror(errno));
			return 1;
		}
		
		// md5
		char md5sumbufcmd[4096];
		char *md5sum_cmd = md5sumbufcmd;
		strcpy(md5sum_cmd, "md5sum ");
		strcat(md5sum_cmd, file);
		FILE *in = popen(md5sum_cmd, "r");
		char md5_buf[33];
		char *md5 = md5_buf;
		fgets(md5, sizeof(md5_buf), in);
		md5[32] = '\0';
		printf("md5sum from server: %s\n", md5);
		if (send(sock, md5, 33, 0) < 0) {
			fprintf(stderr, "server send error: %s\n", strerror(errno));
			return 1;
		}
		
	}
	
	return 0;
}


int UP(char *cmd, int sock) {
	return 0;
}


int RM(char *cmd, int sock) {
	return 0;
}


int LS(char *cmd, int sock) {
	return 0;
}


int MKDIR(char *cmd, int sock) {
	return 0;
}


int RMDIR(char *cmd, int sock) {
	return 0;
}


int CD(char *cmd, int sock) {
	return 0;
}



























