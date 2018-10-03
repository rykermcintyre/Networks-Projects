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
#include <sys/time.h>

// *********** CHANGE ALL ERROR PRINTFS TO FPRINTF(STDERR,...) IF TIME

// Function primitives
int DL(char *, int);
int UP(char *, int);
int RM(char *, int);
int LS(char *, int);
int MKDIR(char *, int);
int RMDIR(char *, int);
int CD(char *, int);

int main(int argc, char *argv[]) {
	
	if (argc!=3) {
		printf("Enter valid arguments: ./myftp [Server_Name] [Port]\n");
		exit(EXIT_FAILURE);
	}
	char* server_name = argv[1];
	const int server_port = atoi(argv[2]);
	

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
				continue;
			}
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
	char *dl = strtok(cmd, " ");
	if (send(sock, dl, strlen(dl), 0) < 0) {
		printf("client send error\n");
		return 1;
	}
	
	// Send file name to server
	char *filename = strtok(NULL, "\n");
	if (send(sock, filename, strlen(filename), 0) < 0) {
		fprintf(stderr, "Client could not send file name: %s\n", strerror(errno));
		return 1;
	}
	
	// recv file size from server
	char szbuf[100];
	if (recv(sock, (char *)szbuf, sizeof(szbuf), 0) < 0) {
		fprintf(stderr, "Receiving file size failed\n");
		return 1;
	}
	int sz = atoi((char *)szbuf);
	
	if (sz == -1) {
		printf("File doesn't exist\n");
		return 1;
	}
	
	// recv md5 hash of file
	char md5_buf[33];
	char *md5 = md5_buf;
	if (recv(sock, md5, sizeof(md5_buf), 0) < 0) {
		fprintf(stderr, "Recving md5 failed\n");
		return 1;
	}

	
	// Make the file you're trying to download
	char touch_buf[4096];
	char *touch_cmd = touch_buf;
	strcpy(touch_cmd, "touch ");
	char *new_filename_ptr = strrchr(filename, '/');
	char new_filename_buf[4096];
	char *new_filename = new_filename_buf;
	if (new_filename_ptr == NULL) {
		strcpy(new_filename, filename);
		strcat(touch_cmd, new_filename);
	}
	else {
		strcpy(new_filename, new_filename_ptr);
		new_filename++;
		strcat(touch_cmd, new_filename);
	}
	system(touch_cmd);
	
	// Recv contents of file and write to file and calc time
	FILE *new_file = fopen(new_filename, "w");
	int f_no = fileno(new_file);
	char get_buf[256];
	void *get = get_buf;
	memset(get, '\0', 256);
	ssize_t n;
	int fsize = 0;
	struct timeval start, end;
	gettimeofday(&start, NULL);
	double start_sec = start.tv_sec;
	double start_usec = start.tv_usec;
	while ((n = recv(sock, get, 256, 0)) == 256) {
		write(f_no, get, 256);
		memset(get, '\0', 256);
		fsize += n;
	}
	write(f_no, get, (int)n);
	gettimeofday(&end, NULL);
	double end_sec = end.tv_sec;
	double end_usec = end.tv_usec;
	double elapsed = (end_sec - start_sec) + (end_usec - start_usec)/1000000;
	fsize += n;
	
	// Get new md5 hash
	char a_md5_buf[4096];
	char *md5_cmd = a_md5_buf;
 	strcpy(md5_cmd, "md5sum ");
 	strcat(md5_cmd, new_filename);
 	FILE *in = popen(md5_cmd, "r");
 	char new_md5_buf[33];
 	char *new_md5 = new_md5_buf;
 	fgets(new_md5, sizeof(new_md5_buf), in);
 	new_md5[32] = '\0';
	
	// Check md5sums
	int match = 0;
	if (strcmp(md5, new_md5) == 0) {
		match = 1;
	}
	
	// Tell user what happened
	double mbps = ((double)fsize/1000000)/elapsed;
	printf("%d bytes transferred in %.06f seconds: %.06f Megabytes/sec\n", fsize, elapsed, mbps);
	printf("\tMD5 hash: %s ", new_md5);
	if (match) printf("(matches)\n");
	else printf("(does not match)\n");
	
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
	char *token = strtok(cmd, " ");
    char * dirName = strtok(NULL, " ");
    char dirNameLen[BUFSIZ];
    short len = strlen(dirName);
    len = htons(len);
	sprintf(dirNameLen, "%d %s", len, dirName); // Read vars in buffer dirNamelen
	if (send(sock, "RM", strlen("RM"), 0) < 0) {
		printf("client send error\n");
		return 1;
	}
	if(send(sock, dirNameLen, strlen(dirNameLen), 0) < 0){
		fprintf(stderr, "client send error: %d", strerror(errno));
	}
	char returnNum[BUFSIZ];
	if(recv(sock, returnNum, BUFSIZ, 0) < 0){
		fprintf(stderr, "client recv failed: %d", strerror(errno));
	}
	int n = atoi(returnNum);
	if(n == -1){
		printf("The file does not exist on server\n");
	}
	else if(n >= 0){
		printf("Are you sure you want to delete the file? Reply Yes if sure, reply No to abandon the delete\n");
		char confirm[10];
		fgets(confirm, 10, stdin);
		if(strcmp(confirm, "Yes\n") == 0){
			if(send(sock, confirm, strlen(confirm), 0) < 0){
				fprintf(stderr, "send failed: %d\n", strerror(errno));
			}
			char successNum[BUFSIZ];
			if(recv(sock, successNum, BUFSIZ, 0) < 0){
				fprintf(stderr, "recv failed: %d\n", strerror(errno));
			}
			n = atoi(successNum);
			if(n >= 0){
				printf("File deleted!\n");
			}
			else{
				printf("Failed to delete the file\n");
			}
		}
		else{
			if(send(sock, confirm, strlen(confirm), 0) < 0){
				fprintf(stderr, "send failed: %d\n", strerror(errno));
				return 1;
			}
		}
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

    char *token = strtok(cmd, " ");
    char * dirName = strtok(NULL, " ");
    char dirNameLen[BUFSIZ];
    short len = strlen(dirName);
    len = htons(len);
	sprintf(dirNameLen, "%d %s", len, dirName); // Read vars in buffer dirNamelen
	if (send(sock, "MKDIR", strlen("MKDIR"), 0) < 0) {
		printf("client send error\n");
		return 1;
	}
	if(send(sock, dirNameLen, strlen(dirNameLen), 0) < 0){
		fprintf(stderr, "client send error: %d", strerror(errno));
	}
	char returnNum[BUFSIZ];
	if(recv(sock, returnNum, BUFSIZ, 0) < 0){
		fprintf(stderr, "client recv failed: %d", strerror(errno));
	}
	int n = atoi(returnNum);
	if(n == 1 || n == 12){
		printf("The directory was successfully made\n");
	}
	else if(n == -1){
		printf("Error in making directory\n");
	}
	else if(n == -2){
		printf("The directory already exists on the server\n");
	}
	return 0;
}


int RMDIR(char *cmd, int sock) {
	char *token = strtok(cmd, " ");
    char * dirName = strtok(NULL, " ");
    char dirNameLen[BUFSIZ];
    short len = strlen(dirName);
    len = htons(len);
	sprintf(dirNameLen, "%d %s", len, dirName); // Read vars in buffer dirNamelen
	if (send(sock, "RMDIR", strlen("RMDIR"), 0) < 0) {
		printf("client send error\n");
		return 1;
	}
	if(send(sock, dirNameLen, strlen(dirNameLen), 0) < 0){
		fprintf(stderr, "client send error: %d", strerror(errno));
	}
	char returnNum[BUFSIZ];
	if(recv(sock, returnNum, BUFSIZ, 0) < 0){
		fprintf(stderr, "client recv failed: %d", strerror(errno));
	}
	int n = atoi(returnNum);
	if(n == -2){
		printf("The directory is not empty\n");
	}
	else if(n == -1){
		printf("The directory does not exist on server\n");
	}
	else if(n >= 0){
		printf("Are you sure you want to delete the directory? Reply Yes if sure, reply No to abandon the delete\n");
		char confirm[10];
		fgets(confirm, 10, stdin);
		if(strcmp(confirm, "Yes\n") == 0){
			if(send(sock, confirm, strlen(confirm), 0) < 0){
				fprintf(stderr, "send failed: %d\n", strerror(errno));
			}
			char successNum[BUFSIZ];
			if(recv(sock, successNum, BUFSIZ, 0) < 0){
				fprintf(stderr, "recv failed: %d\n", strerror(errno));
			}
			n = atoi(successNum);
			if(n >= 0){
				printf("Directory deleted!\n");
			}
			else{
				printf("Failed to delete directory\n");
			}
		}
		else{
			if(send(sock, confirm, strlen(confirm), 0) < 0){
				fprintf(stderr, "send failed: %d\n", strerror(errno));
				return 1;
			}
		}
	}
	return 0;
}


int CD(char *cmd, int sock) {
	char *token = strtok(cmd, " ");
    char * dirName = strtok(NULL, " ");
    char dirNameLen[BUFSIZ];
    short len = strlen(dirName);
    len = htons(len);
	sprintf(dirNameLen, "%d %s", len, dirName); // Read vars in buffer dirNamelen
	if (send(sock, "CD", strlen("CD"), 0) < 0) {
		printf("client send error\n");
		return 1;
	}
	printf("%s\n", dirName); ////////////////
	if(send(sock, dirNameLen, strlen(dirNameLen), 0) < 0){
		fprintf(stderr, "client send error: %d", strerror(errno));
	}
	printf("sent that mutha fucka\n");
	char returnNum[BUFSIZ];
	if(recv(sock, returnNum, BUFSIZ, 0) < 0){
		fprintf(stderr, "client recv failed: %d", strerror(errno));
	}
	int n = atoi(returnNum);
	if(n == 1){
		printf("Changed current directory\n");
	}
	else if(n == -1){
		printf("Error in changing directory\n");
	}
	else{
		printf("The directory does not exist on server\n");
	}
	return 0;
}

