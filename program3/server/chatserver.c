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
	char *yes_string = "yes";
	char *no_string = "no";
	
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
		if(user == NULL){
			break;
		}
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
	memset(password, 0, sizeof(password));
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
		// Build string and append that combo
		char append_line[1024];
		memset(append_line, 0, sizeof(append_line));
		if (sprintf((char *)append_line, "%s %s\n", username, password) < 0) {
			fprintf(stderr, "Could not build \"user pass\" string: %s\n", strerror(errno));
			STATUS = 1;
			goto cleanup;
		}
		if (fwrite(append_line, strlen(append_line), 1, usersfile) < 0) {
			fprintf(stderr, "Could not append user/pass combo to file: %s\n", strerror(errno));
			STATUS = 1;
			goto cleanup;
		}
		// Ack "yes"
		if (send(sock, yes_string, strlen(yes_string), 0) < 0) {
			fprintf(stderr, "Server could not ack that user was added: %s\n", strerror(errno));
			STATUS = 1;
			goto cleanup;
		}
		
	}
	
	// DO CRYPTOGRAPHY THINGS NOW
	// Generate a public key
	char *pubkey = getPubKey();

	// Get client's public key
	char client_key[4096];
	memset(client_key, 0, sizeof(client_key));
	if (recv(sock, (char *)client_key, sizeof(client_key), 0) < 0) {
		fprintf(stderr, "Could not recv client key: %s\n", strerror(errno));
		STATUS = 1;
		goto cleanup;
	}
	
	// Encrypt pubkey w/ client's key and send
	char *encryptedkey = encrypt(pubkey, client_key);
	if (send(sock, encryptedkey, strlen(encryptedkey), 0) < 0) {
		fprintf(stderr, "Could not send encrypted key back to client: %s\n", strerror(errno));
		STATUS = 1;
		goto cleanup;
	}
	
	// Open file descriptor for appending to active users file
	FILE *activeusersfile = fopen("activeusers.txt", "a");
	
	// Append user/socket to active users file
	char add_active[8192];
	sprintf((char *)add_active, "%s:%d:%s\n", username, sock, client_key);
	if (fwrite(add_active, strlen(add_active), 1, activeusersfile) < 0) {
		fprintf(stderr, "Could not append active user to file: %s\n", strerror(errno));
		STATUS = 1;
		fclose(activeusersfile);
		goto cleanup;
	}
	
	// Close file descriptor (will open again later for reading/writing, not appending)
	fclose(activeusersfile);
	
	// Begin while loop for receiving commands/messages from the client
	while (1) {
		// Declare buffers and reset them every loop
		char command[10];
		char message[4096];
		char userlist[4096];
		char selected_user[512];
		char receiverPubKey[4096];
		memset(receiverPubKey, 0, sizeof(receiverPubKey));
		memset(selected_user, 0, sizeof(selected_user));
		memset(userlist, 0, sizeof(userlist));
		memset(message, 0, sizeof(message));
		memset(command, 0, sizeof(command));
		
		// Receive the command from the client
		if (recv(sock, (char *)command, sizeof(command), 0) < 0) {
			fprintf(stderr, "Unable to receive command from the client: %s\n", strerror(errno));
			STATUS = 1;
			goto cleanup;
		}
		
		// Send ack that command was received
		if (send(sock, yes_string, strlen(yes_string), 0) < 0) {
			fprintf(stderr, "Unable to send ack that command was recvd: %s\n", strerror(errno));
			STATUS = 1;
			goto cleanup;
		}
		
		// Split into cases for public message, direct message, or quit
		// P: Public message
		if (strcmp(command, "P") == 0) {
			// Receive message from user
			if (recv(sock, (char *)message, sizeof(message), 0) < 0) {
				fprintf(stderr, "Unable to receive P message from user: %s\n", strerror(errno));
				STATUS = 1;
				goto cleanup;
			}
			
			// Send ack "yes" that message was received
			if (send(sock, yes_string, strlen(yes_string), 0) < 0) {
				fprintf(stderr, "Unable to send ack that P message was received: %s\n", strerror(errno));
				STATUS = 1;
				goto cleanup;
			}
			
			// TODO Loop through users in activeusers file and send
			// message to each of them in format P:<username>:<message>
			// as long as the user isn't the one sending the message
			
			
		}
		// D: Direct message
		else if (strcmp(command, "D") == 0) {
			// TODO Loop through users in activeusers file and format
			// the string userlist as "user1\nuser2\n..."
			
			// Send userlist to client
			if (send(sock, (char *)userlist, strlen(userlist), 0) < 0) {
				fprintf(stderr, "Could not send user list to client: %s\n", strerror(errno));
				STATUS = 1;
				goto cleanup;
			}
			
			// Receive username that client would like to talk to
			if (recv(sock, (char *)selected_user, sizeof(selected_user), 0) < 0) {
				fprintf(stderr, "Could not receive selected user from client: %s\n", strerror(errno));
				STATUS = 1;
				goto cleanup;
			}
			
			// TODO Loop through users in activeusers file and extract
			// the socket and the public key (receiverPubKey) for that user
			// TODO Set the receiverPubKey and receiver sock fake things
			// if user isn't in the file, but turn user_not_found to true
			bool user_not_found = 0;
			int receiver_sock;
			
			// Send receiverPubKey to the client
			if (send(sock, (char *)receiverPubKey, strlen(receiverPubKey), 0) < 0) {
				fprintf(stderr, "Could not send receiver's public key to client: %s\n", strerror(errno));
				STATUS = 1;
				goto cleanup;
			}
			
			// Receive message from the user
			if (recv(sock, (char *)message, sizeof(message), 0) < 0) {
				fprintf(stderr, "Could not recv D message from client: %s\n", strerror(errno));
				STATUS = 1;
				goto cleanup;
			}
			
			// TODO If user_not_found, send "inv" and do a CONTINUE STATEMENT
			// Else if user IS found, double check user is still active
			// If user is still active, send "yes"
			// If user is no longer active, send "inv" and do a CONTINUE STATEMENT
			
			// TODO User is active, so send message to that user
			if (send(receiver_sock, (char *)message, strlen(message), 0) < 0) {
				fprintf(stderr, "Could not send D message to other user: %s\n", strerror(errno));
				STATUS = 1;
				goto cleanup;
			}
		}
		// Q: Quit
		else if (strcmp(command, "Q") == 0) {
			// Status remains 0, just clean everything up to end user session
			goto cleanup;
		}
		// Anything else: error
		else {
			fprintf(stderr, "Invalid command received from user: not P, D, or Q.\n");
			STATUS = 1;
			goto cleanup;
		}
	}
	
	
	// Clean up
cleanup:
	// TODO Add commands to delete user from the activeusers file
	// if their name does, in fact, appear in the file
	fclose(usersfile);
	close(sock);
	pthread_exit(NULL);
	exit(STATUS);
}





