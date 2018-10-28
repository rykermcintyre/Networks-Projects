// Chat server
// Programming Assignment 3
//
// RYKER McINTYRE rmcinty3
// THOMAS PLUMMER tplumme2
// JOSHUA JOHNSON jjohns54
//
// October 28, 2018

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
#include <dirent.h>
#include <sys/stat.h>
#include <pthread.h>
#include "pg3lib.h"

// ============ CHANGE THIS TO CMD LINE ARGS LATER ============
#define SERVER_PORT 41032

// Function primitives
void *handle_client(void *);

int main() {
	
	// Socket for server
	struct sockaddr_in server_address;
	memset(&server_address, 0, sizeof(server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(SERVER_PORT);
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);
	
	// Create the socket
	int listen_sock;
	if ((listen_sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr, "Could not create server listen sock: %s\n", strerror(errno));
		return 1;
	}
	
	// Bind socket
	if ((bind(listen_sock, (struct sockaddr *)&server_address, sizeof(server_address))) < 0) {
		fprintf(stderr, "Could not bind server sock: %s\n", strerror(errno));
		return 1;
	}
	
	// Listen for incoming connections
	int wait_size = 16;
	if (listen(listen_sock, wait_size) < 0) {
		fprintf(stderr, "Could not open socket for listening: %s\n", strerror(errno));
		return 1;
	}
	
	// Accepting connections
	struct sockaddr_in client_address;
	int client_address_len = 0;
	printf("Accepting connections on port %d\n", SERVER_PORT);
	
	pthread_t threads[512]; // Able to accept 512 connections
	
	// Accept connections loop
	while (1) {
		int sock;
		if ((sock = accept(listen_sock, (struct sockaddr *)&client_address, &client_address_len)) < 0) {
			fprintf(stderr, "Could not open a socket for a client: %s\n", strerror(errno));
			return 1;
		}
		
		// Create a thread to handle the client, loop to continue listening
		int rc = pthread_create(&threads[sock-3], NULL, handle_client, (void *)&sock); // sock - 3 since sockets begin at 3
		
		if (rc != 0) {
			fprintf(stderr, "Unable to create thread: %s\n", strerror(errno));
			return 1;
		}
	}
	
	return 0;
}

void *handle_client(void *s) {
	int STATUS = 0;
	
	// Resolve socket number
	int sock = *(int*)s;
	
	// Open file descriptor for reading and appending to users.txt file
	FILE *usersfile = fopen("users.txt", "a+");
	
	// Recv username
	char username[512];
	memset(username, 0, sizeof(username));
	if (recv(sock, (char *)username, sizeof(username), 0) < 0) {
		fprintf(stderr, "Server failed to recv username: %s\n", strerror(errno));
		STATUS = 1;
		goto cleanup;
	}
	
	// Check for username in file
	bool found_user = 0;
	char user_buf[512];
	char pass_buf[512];
	char *user = user_buf;
	char *pass = pass_buf;
	char line[512];
	char *line_str = line;
	size_t line_buf_size = 512;
	memset(line, 0, sizeof(line));
	memset(user_buf, 0, sizeof(user_buf));
	memset(pass_buf, 0, sizeof(pass_buf));
	while (getline(&line_str, &line_buf_size, usersfile)) {
		user = strtok(line, " ");
		pass = strtok(NULL, "\n");
		if (strcmp(user, username) == 0) {
			found_user = 1;
			break;
		}
		memset(line, 0, sizeof(line));
		memset(user_buf, 0, sizeof(user_buf));
		memset(pass_buf, 0, sizeof(pass_buf));
	}
	
	if (found_user) {
		char *resp = "yes";
		if (send(sock, resp, strlen(resp), 0) < 0) {
			fprintf(stderr, "Server failed to ack user was found: %s\n", strerror(errno));
			STATUS = 1;
			goto cleanup;
		}
	}
	else {
		char *resp = "no";
		if (send(sock, resp, strlen(resp), 0) < 0) {
			fprintf(stderr, "Server failed to ack user was not found: %s\n", strerror(errno));
			STATUS = 1;
			goto cleanup;
		}
	}
	
	// Receive password
	char password[512];
	if (recv(sock, (char *)password, sizeof(password), 0) < 0) {
		fprintf(stderr, "Could not receive password from client: %s\n", strerror(errno));
		STATUS = 1;
		goto cleanup;
	}
	
	// If found_user is true (user exists), check passwd.
	// Else, append username/password combo to the file
	if (found_user) {
		// Check if password is right
		// Ack "yes" or "no". If "no", keep recving password until correct
		char *yes_string = "yes";
		char *no_string = "no";
		while (strcmp(password, pass) != 0) {
			memset(password, 0, sizeof(password));
			if (send(sock, no_string, strlen(no_string), 0) < 0) {	
				fprintf(stderr, "Server could not reject password: %s\n", strerror(errno));
				STATUS = 1;
				goto cleanup;
			}
			if (recv(sock, (char *)password, sizeof(password), 0) < 0) {
				fprintf(stderr, "Server could not recv new password attempt: %s\n", strerror(errno));
				STATUS = 1;
				goto cleanup;
			}
		}
		// Now that we have the right password, ack yes
		if (send(sock, yes_string, strlen(yes_string), 0) < 0) {
			fprintf(stderr, "Server could not accept password: %s\n", strerror(errno));
			STATUS = 1;
			goto cleanup;
		}
	}
	else {
		// TODO Append combo
		
		
		// TODO Ack "yes"
		
		
	}
	
	// TODO DO CRYPTOGRAPHY THINGS NOW
	
	
	// Clean up
cleanup:
	close(sock);
	pthread_exit(NULL);
	exit(STATUS);
}













