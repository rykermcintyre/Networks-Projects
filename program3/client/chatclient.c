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

void * handle_incoming_messages(void *s);

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char *argv[]) {
	
	if (argc != 4) {
		fprintf(stderr, "Usage: %s <server name> <port> <username>\n", argv[0]);
		return 1;
	}
	char *username = argv[3];
	int server_port = atoi(argv[2]);
	char * server_name = argv[1];
	
	// Socket
	struct sockaddr_in server_address;
	memset(&server_address, 0, sizeof(server_address));
	server_address.sin_family = AF_INET;
	
	struct hostent *hp;
	hp = (struct hostent *)gethostbyname(server_name);
	bcopy(hp->h_addr, (char *)&server_address.sin_addr, hp->h_length);
	
	inet_pton(AF_INET, server_name, &server_address.sin_addr);
	server_address.sin_port = htons(server_port);
	
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
	
	printf("Connecting to %s on port %d\n", server_name, server_port);
	
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
	pthread_mutex_unlock(&lock);
	
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
	pthread_mutex_lock(&lock);
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
	pthread_mutex_unlock(&lock);

	
	// If password is incorrect, reprompt user
	while (strcmp(correct_password, no_string) == 0) {
		printf("Incorrect password. Try again: ");
		memset(password, 0, sizeof(password));
		fgets((char *)password, sizeof(password), stdin);
		while (strlen(password) < 1) {
			fgets((char *)password, sizeof(password), stdin);
		}
		password[strlen(password) - 1] = '\0';
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
	pthread_t pt;
	int rc = pthread_create(&pt, NULL, handle_incoming_messages, (void *)&sock);
	if(rc != 0){
		fprintf(stderr, "Unable to create thread: %s\n", strerror(errno));
		return 1;
	}

	// Main thread: Handle normal client ops
	
	while(1){
		// Initialize buffers, always reset each loop
		char buffer[10];
		char ack[10];
		char msg[4096];
		char confirm[10];
		char userList[4096];
		char username[512];
		char *invalid_user = "inv";
		char receiverPubKey[4096];
		char encrypted_msg_buff[4096];
		char *encrypted_msg = encrypted_msg_buff;
		memset(encrypted_msg_buff, 0, sizeof(encrypted_msg_buff));
		memset(receiverPubKey, 0, sizeof(receiverPubKey));
		memset(username, 0, sizeof(username));
		memset(userList, 0, sizeof(userList));
		memset(confirm, 0, sizeof(confirm));
		memset(buffer, 0, sizeof(buffer));
		memset(ack, 0, sizeof(ack));
		memset(msg, 0, sizeof(msg));
		
		// Read input command
		printf("Please enter a command: P for public messages, D for direct messages, or Q to quit.\n");
		printf(">>> ");
		fgets((char *)buffer, sizeof(buffer), stdin);
		buffer[strlen(buffer) - 1] = '\0';
		
		// Make sure user enters proper command
		while (strcmp((char *)buffer, "P") != 0 && strcmp((char *)buffer, "D") != 0 && strcmp((char *)buffer, "Q") != 0){
			memset(buffer, 0, sizeof(buffer));
			printf("Invalid command. Please enter P, D, or Q.\n>>> ");
			fgets((char *)buffer, sizeof(buffer), stdin);
			buffer[strlen(buffer) - 1] = '\0';
		}
		
		pthread_mutex_lock(&lock);
		// Send command
		if(send(sock, (char *)buffer, strlen(buffer), 0) < 0){
			fprintf(stderr, "Unable to send command to server: %s\n", strerror(errno));
			return 1;
		}
		
		printf("Sent command\n");
		
		// Recv ack that command was sent
		if(recv(sock, (char *)ack, sizeof(ack), 0) < 0){
			fprintf(stderr, "Unable to receive ack from server: %s\n", strerror(errno));
			return 1;
		}
		pthread_mutex_unlock(&lock);
		printf("received ack\n");
		printf("ack: %s\n", ack);	
		
		// Error if ack wasn't "yes"
		if(strcmp(ack, yes_string) != 0){
			fprintf(stderr, "Invalid acknowledgement received.\n");
			return 1;
		}
		// Command is P: Public message
		if(strcmp(buffer, "P") == 0){
			// Get message from user
			printf("Enter message\n>>> ");
			fgets((char *)msg, sizeof(msg), stdin);
			msg[strlen(msg) - 1] = '\0';
			
			// Send message to server
			pthread_mutex_lock(&lock);
			if(send(sock, (char *)msg, strlen(msg), 0) < 0){
				fprintf(stderr, "Unable to send message to server: %s\n", strerror(errno));
				return 1;
			}
			
			// Recv confirmation that message was recvd
			if(recv(sock, (char *)confirm, sizeof(confirm), 0) < 0){
				fprintf(stderr, "Unable to receive confirmation of sent message from server: %s\n", strerror(errno));
				return 1;
			}
			pthread_mutex_unlock(&lock);

			// If message was not "yes", error
			if(strcmp(confirm, yes_string) != 0){
				fprintf(stderr, "Invalid confirmation receieved\n");
				return 1;
			}
		}
		// Command is D: Direct message
		else if(strcmp(buffer, "D") == 0){
			// Get the user list from the server
			pthread_mutex_lock(&lock);
			if(recv(sock, (char *)userList, sizeof(userList), 0) < 0){
				fprintf(stderr, "Unable to receive list of users from server: %s\n", strerror(errno));
				return 1;
			}
			pthread_mutex_unlock(&lock);
			
			// Print the list of users
			printf("Users online:\n%s\n", userList);
			printf("Which user would you like to talk to?\n");
			printf(">>> ");
			
			// Read in the user the client would like to talk to
			fgets((char *)username, sizeof(username), stdin);
			username[strlen(username) - 1] = '\0';
			
			// Send this username to the server
			pthread_mutex_lock(&lock);
			if(send(sock, (char *)username, strlen(username), 0) < 0){
				fprintf(stderr, "Unable to send username message to server: %s\n", strerror(errno));
				return 1;
			}
			
			// Receive the public key for that user
			if(recv(sock, (char *)receiverPubKey, sizeof(receiverPubKey), 0) < 0){
				fprintf(stderr, "Unable to receive public key of user from server: %s\n", strerror(errno));
				return 1;
			}
			pthread_mutex_unlock(&lock);
			
			// Read in message to be sent to that user
			printf("Enter message\n>>> ");
			fgets((char *)msg, sizeof(msg), stdin);
			msg[strlen(msg) - 1] = '\0';
			
			// Encrypt message and send to server
			encrypted_msg = encrypt((char *)msg, receiverPubKey);
			pthread_mutex_lock(&lock);
			if(send(sock, (char *)encrypted_msg, strlen(encrypted_msg), 0) < 0){
				fprintf(stderr, "Unable to send message to server: %s\n", strerror(errno));
				return 1;
			}
			
			// Receive confirmation that user is online ("yes" or "inv")
			if(recv(sock, (char *)confirm, sizeof(confirm), 0) < 0){
				fprintf(stderr, "Unable to receive confirmation of sent message from server: %s\n", strerror(errno));
				return 1;
			}
			pthread_mutex_unlock(&lock);
			if(strcmp(confirm, invalid_user) == 0){
				printf("User doesn't exist/isn't online\n");
			}
			else if(strcmp(confirm, yes_string) != 0){
				fprintf(stderr, "Invalid confirmation receieved\n");
				return 1;
			}
		}
		// Command is Q: quit
		else{
			close(sock);
			return 0;
		}
	}
	
}

// Second thread: Handle incoming messages
void * handle_incoming_messages(void *s){
	int STATUS = 0;
	int sock = *(int*)s;
	char message[4096];
	memset(message, 0, sizeof(message));
	char type_buff[10];
	char sender_buff[512];
	char output_buff[4096];
	memset(type_buff, 0, sizeof(type_buff));
	memset(sender_buff, 0, sizeof(sender_buff));
	memset(output_buff, 0, sizeof(output_buff));
	
	char* type = type_buff;
	char* sender = sender_buff;
	char* output = output_buff;

	while(1){
		if(recv(sock, (char *)message, sizeof(message), 0) < 0){
			fprintf(stderr, "Unable to receive from server: %s\n", strerror(errno));
			exit(1);
		}
		if (message[0] != 'P' && message[0] != 'D') {
			memset(type, 0, sizeof(type));
			memset(sender, 0, sizeof(sender));
			memset(output, 0, sizeof(output));
			memset(message, 0, sizeof(message));
			continue;
		}
		type = strtok(message, ":");
		sender = strtok(NULL, ":");
		output = strtok(NULL, "\n");
		if(strcmp(type, "P") == 0){
			printf("\n****Incoming Public message from %s****\n", sender);
			printf("%s\n", output);
		}
		else{
			printf("\n****Direct message from %s****\n", sender);
			// TODO Create a struct to pass multiple args into this
			// function, so that pubkey can be sent into this function
			// and we can use it to decrypt direct messages
			char *decrypted_output = decrypt(output);
			printf("%s\n", decrypted_output);
		}
		printf(">>> ");
		fflush(stdout);
		memset(type, 0, sizeof(type));
		memset(sender, 0, sizeof(sender));
		memset(output, 0, sizeof(output));
		memset(message, 0, sizeof(message));
	}
}















