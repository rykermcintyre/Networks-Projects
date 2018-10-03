// CLIENT FOR PROGRAMMING ASSIGNMENT 2
// TCP BASED SIMPLE FTP
// BASE CODE FROM BEEJ'S GUIDE
//
// RYKER McINTYRE rmcinty3
// THOMAS PLUMMER tplumme2
// JOSHUA JOHNSON jjohns54
//
// October 3, 2018


#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>

// *********** CHANGE ALL ERROR PRINTFS TO FPRINTF(STDERR,...) IF TIME

// Function primitives
int DL(char *, int);
int UP(char *, int);
int RM(char *, int);
int LS(char *, int);
int MKDIR(char *, int);
int RMDIR(char *, int);
int CD(char *, int);

int main() {
	
	// ************ CHANGE THIS TO ACCEPT CMD LINE ARGS LATER ************
	char* server_name = "student06.cse.nd.edu";
	const int server_port = 41023;

	struct sockaddr_in server_address;
	memset(&server_address, 0, sizeof(server_address));
	server_address.sin_family = AF_INET;
	
	struct hostent *hp;
	hp = (struct hostent *)gethostbyname(server_name);
	bcopy(hp->h_addr, (char *)&server_address.sin_addr, hp->h_length);

	// creates binary representation of server name
	// and stores it as sin_addr
	// http://beej.us/guide/bgnet/output/html/multipage/inet_ntopman.html
	inet_pton(AF_INET, server_name, &server_address.sin_addr);

	// htons: port in network order format
	server_address.sin_port = htons(server_port);

	// open a stream socket
	int sock;
	if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		printf("could not create socket\n");
		return 1;
	}

	// TCP is connection oriented, a reliable connection
	// **must** be established before any data is exchanged
	if (connect(sock, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
		printf("could not connect to server\n");
		printf("%s\n", strerror(errno));
		return 1;
	}
	
	printf("Connecting to %s on port %d\n", server_name, server_port);
	printf("Connection established\n");

	// sending loop (prompt user for op in client state)
	while (1) {
		printf("> ");
		char cmd[4096];
		fgets(cmd, 4096, stdin);
		if (strncmp(cmd, "DL", 2) == 0) {
			if (DL(cmd, sock) != 0) {
				fprintf(stderr, "Client command failed\n");
				return 1;
			}
			printf("Got DL\n");
		}
		else if (strncmp(cmd, "UP", 2) == 0) {
			if (UP(cmd, sock) != 0) {
				fprintf(stderr, "Client command failed\n");
				return 1;
			}
			printf("Got UP\n");
		}
		else if (strncmp(cmd, "RMDIR", 5) == 0) {
			if (RMDIR(cmd, sock) != 0) {
				fprintf(stderr, "Client command failed\n");
				return 1;
			}
			printf("Got RMDIR\n");
		}
		else if (strncmp(cmd, "LS", 2) == 0) {
			if (LS(cmd, sock) != 0) {
				fprintf(stderr, "Client command failed\n");
				return 1;
			}
			printf("Got LS\n");
		}
		else if (strncmp(cmd, "MKDIR", 5) == 0) {
			if (MKDIR(cmd, sock) != 0) {
				fprintf(stderr, "Client command failed\n");
				return 1;
			}
			printf("Got MKDIR\n");
		}
		else if (strncmp(cmd, "RM", 2) == 0) {
			if (RM(cmd, sock) != 0) {
				fprintf(stderr, "Client command failed\n");
				return 1;
			}
			printf("Got RM\n");
		}
		else if (strncmp(cmd, "CD", 2) == 0) {
			if (CD(cmd, sock) != 0) {
				fprintf(stderr, "Client command failed\n");
				return 1;
			}
			printf("Got CD\n");
		}
		else if (strncmp(cmd, "EXIT", 4) == 0) {
			printf("Got EXIT\n");
			if (send(sock, "", 0, 0) < 0) {
				printf("client send error\n");
				return 1;
			}
			return 0;
		}
		else {
			printf("Please use a valid command\n");
		}
	}

	/* **** HOW TO RECEIVE BACK FROM SERVER **** 
	// receive
	int n = 0;
	int len = 0, maxlen = 100;
	char buffer[maxlen];
	char* pbuffer = buffer;

	// will remain open until the server terminates the connection
	while ((n = recv(sock, pbuffer, maxlen, 0)) > 0) {
		pbuffer += n;
		maxlen -= n;
		len += n;

		buffer[len] = '\0';
		printf("client received: '%s'\n", buffer);
	}
	*/

	// close the socket
	close(sock);
	return 0;
}


// ***************** SIMPLE POPEN EXAMPLE FOR REFERENCE ****************
/*
FILE *in = popen("ls -l | grep client | sort", "r");
char output[4096];
printf("output is:\n");
while (fgets(output, sizeof(output), in) > 0) {
	printf("%s",output);
}
*/





int DL(char *cmd, int sock) {
	// Send command to the server
	// char *dl = strtok(cmd, " ");
	// printf("dl: |%s|\n", dl);
	// if (send(sock, dl, strlen(dl), 0) < 0) {
	// 	printf("client send error\n");
	// 	return 1;
	// }
	
	// // break apart command
	// char *file = strtok(NULL, "\n");
	// printf("dl: |%s|\nfile name: |%s|\n", dl, file);
	// short int len = htons(strlen(file));
	
	// // Send length of file name then file name
	// if (send(sock, (char *)&len, sizeof(short int), 0) < 0) {
	// 	fprintf(stderr, "client send error 1\n");
	// 	return 1;
	// }
	// printf("sent length of string: |%d|\n", (int)ntohs(len));
	// if (send(sock, file, len, 0) < 0) {
	// 	fprintf(stderr, "client send error\n");
	// 	return 1;
	// }
	// printf("sent string: |%s|\n", file);
	
	
	// return 0;
}


int UP(char *cmd, int sock) {
	if (send(sock, "UP", strlen("UP"), 0) < 0) {
		printf("client send error\n");
		return 1;
	}
	return 0;
}


int RM(char *cmd, int sock) {
	if (send(sock, "RM", strlen("RM"), 0) < 0) {
		printf("client send error\n");
		return 1;
	}
	return 0;
}


int LS(char *cmd, int sock) {
	if (send(sock, "LS", strlen("LS"), 0) < 0) {
		printf("client send error\n");
		return 1;
	}

	char * buff;
	recv(sock, buff, BUFSIZ, 0);
	printf("%s\n", buff);

	return 0;
}


int MKDIR(char *cmd, int sock) {
	if (send(sock, "MKDIR", strlen("MKDIR"), 0) < 0) {
		printf("client send error\n");
		return 1;
	}
	return 0;
}


int RMDIR(char *cmd, int sock) {
	if (send(sock, "RMDIR", strlen("RMDIR"), 0) < 0) {
		printf("client send error\n");
		return 1;
	}
	return 0;
}


int CD(char *cmd, int sock) {
	if (send(sock, "CD", strlen("CD"), 0) < 0) {
		printf("client send error\n");
		return 1;
	}
	return 0;
}























