// Chat client

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
#include <sys/time.h>
#include <pthread.h>
#include "pg3lib.h"

// ============== CHANGE THESE TO CMD LINE ARGS LATER ============
#define SERVER_NAME "student06.cse.nd.edu"
#define SERVER_PORT 41032

int main(int argc, char *argv[]) {
	
	// Handle cmd line args ++++++++++++++ FIX THIS!!! +++++++++++++++
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <server name> <port> <username>", argv[0]);
		return 1;
	}
	char *username = argv[1];
	
	// Socket
	struct sockaddr_in server_address;
	memset(&server_address, 0, sizeof(server_address));
	server_address.sin_family = AF_INET;
	
	struct hostent *hp;
	hp = (struct hostent *)gethostbyname(SERVER_NAME);
	bcopy(hp->h_addr, (char *)&server_address.sin_addr, hp->h_length);
	
	inet_pton(AF_INET, SERVER_NAME, &server_address.sin_addr);
	server_address.sin_port = htons(SERVER_PORT);
	
	// Create socket
	int sock;
	if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr, "Could not create client socket: %s\n", strerror(errno));
		return 1;
	}
	
	// Connect to server
	if (connect(sock, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
		fprintf(stderr, "Could not connect to the server: %s\n", strerror(errno));
		close(sock);
		return 1;
	}
	
	printf("Connecting to %s on port %d\n", SERVER_NAME, SERVER_PORT);
	
	// Send username
	if (send(sock, username, strlen(username), 0) < 0) {
		fprintf(stderr, "Client could not send username: %s\n", strerror(errno));
		close(sock);
		return 1;
	}
	
	// Receive if user exists
	char existsbuf[10];
	if (recv(sock, (char *)existsbuf, sizeof(existsbuf), 0) < 0) {
		fprintf(stderr, "Client could not recv if user exists: %s\n", strerror(errno));
		close(sock);
		return 1;
	}
	
	// If user exists (yes) then prompt for password
	// If user doesn't exist, prompt to create password
	char *yes_string = "yes";
	char *no_string = "no";
	if (strcmp(existsbuf, no_string) == 0) {
		printf("Creating New User\nEnter Password: ");
	}
	else if (strcmp(existsbuf, yes_string) == 0) {
		printf("Enter Password: ");
	}
	else {
		fprintf(stderr, "Invalid received user acknowledgement: %s\n", existsbuf);
		close(sock);
		return 1;
	}
	
	// Get password from stdin
	char password[512];
	fgets((char *)password, sizeof(password), stdin);
	while (strlen(password) < 1) {
		fgets((char *)password, sizeof(password), stdin);
	}
	password[strlen(password) - 1] = '\0';
	
	// Send the password
	if (send(sock, password, strlen(password), 0) < 0) {
		fprintf(stderr, "Client could not send password: %s\n", strerror(errno));
		close(sock);
		return 1;
	}
	
	// Receive acknowledgement (password correct? yes/no. Always yes if new password)
	char correct_password[10];
	memset(correct_password, 0, sizeof(correct_password));
	if (recv(sock, (char *)correct_password, sizeof(correct_password), 0) < 0) {
		fprintf(stderr, "Could not receive password acknowledgement: %s\n", strerror(errno));
		close(sock);
		return 1;
	}
	
	// If password is incorrect, reprompt user
	while (strcmp(correct_password, no_string) == 0) {
		printf("Incorrect password. Try again: ");
		memset(password, 0, sizeof(password));
		fgets((char *)password, sizeof(password), stdin);
		while (strlen(password) < 1) {
			fgets((char *)password, sizeof(password), stdin);
		}
		if (send(sock, password, strlen(password), 0) < 0) {
			fprintf(stderr, "Client could not send password: %s\n", strerror(errno));
			close(sock);
			return 1;
		}
		memset(correct_password, 0, sizeof(correct_password));
		if (recv(sock, (char *)correct_password, sizeof(correct_password), 0) < 0) {
			fprintf(stderr, "Could not receive password acknowledgement: %s\n", strerror(errno));
			close(sock);
			return 1;
		}
	}
	if (strcmp(correct_password, yes_string) != 0) {
		fprintf(stderr, "Invalid received password acknowledgement: %s\n", correct_password);
		close(sock);
		return 1;
	}
	
	// Generate public key, send to server
	char *pubkey = getPubKey();
	int key_len = strlen(pubkey);
	if (send(sock, pubkey, key_len, 0) < 0) {
		fprintf(stderr, "Could not send public key to server: %s\n", strerror(errno));
		close(sock);
		return 1;
	}
	
	// Get server's encrypted public key, decrypt it
	char encryptedkey[4096];
	memset(encryptedkey, 0, sizeof(encryptedkey));
	if (recv(sock, (char *)encryptedkey, sizeof(encryptedkey), 0) < 0) {
		fprintf(stderr, "Couldn not recv host's public key: %s\n", strerror(errno));
		close(sock);
		return 1;
	}
	char *hostkey = decrypt(encryptedkey);
	
	// Split threads
	// Main thread: Handle normal client ops
	// Second thread: Handle incoming messages
	
}


















