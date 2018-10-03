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
#include <sys/stat.h>
#include <dirent.h>

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
	if (argc!=2) {
		printf("Enter valid arguments: ./myftpd [Port]\n");
		exit(EXIT_FAILURE);
	}

	int SERVER_PORT = atoi(argv[1]);

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
		memset(pbuffer, '\0', maxlen);
		while (n = recv(sock, pbuffer, maxlen, 0) > 0) {
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
				fprintf(stderr, "error in receiving command\n");
			}	
			memset(pbuffer, '\0', maxlen);
		}
		close(sock);
	}

	close(listen_sock);
	return 0;
}

int DL(char *cmd, int sock) {
	
	// Recv file name
	char filename_buf[4096];
	char *filename = filename_buf;
	memset(filename, '\0', 4096);
	if (recv(sock, filename, 4096, 0) < 0) {
		fprintf(stderr, "Server could not recv file name: %s\n", strerror(errno));
		return 1;
	}
	
	// Try to open file
	FILE *fp = fopen(filename, "r");
	if (fp == NULL) {
		char *neg1 = "-1";
		if (send(sock, neg1, sizeof(neg1), 0) < 0) {
			fprintf(stderr, "Could not send -1 to client\n");
			return 1;
		}
	}
	else {
		// Send size of file
		fseek(fp, 0L, SEEK_END);
		int sz = ftell(fp);
		rewind(fp);
		char szbuf[100];
		sprintf(szbuf, "%d", sz);
		if (send(sock, (char *)szbuf, sizeof(szbuf), 0) < 0) {
			fprintf(stderr, "server send error sending size of file: %s\n", strerror(errno));
			return 1;
		}
		
		// Send md5 hash of file
		char md5_buf[4096];
		char *md5_cmd = md5_buf;
		strcpy(md5_cmd, "md5sum ");
		strcat(md5_cmd, filename);
		FILE *in = popen(md5_cmd, "r");
		char new_md5_buf[33];
		char *md5 = new_md5_buf;
		fgets(md5, sizeof(new_md5_buf), in);
		fclose(in);
		md5[32] = '\0';
		if (send(sock, md5, 33, 0) < 0) {
			fprintf(stderr, "server send error: %s\n", strerror(errno));
			return 1;
		}
		
		// Start reading from file and sending
		char get_buf[256];
		void *get = get_buf;
		memset(get, '\0', 256);
		int n;
		while ((n = fread(get, 1, 256, fp)) == 256) {
			if (send(sock, get, 256, 0) < 0) {
				fprintf(stderr, "Couldn't send part of file to client: %s\n", strerror(errno));
				return 1;
			}
			
			memset(get, '\0', 256);
		}
		if (send(sock, get, n, 0) < 0) {
			fprintf(stderr, "Couldn't send part of file to client\n");
			return 1;
		}
		
	}

	
	return 0;
}


int UP(char *cmd, int sock) {
	return 0;
}


int RM(char *cmd, int sock) {
	char fileNameLen[BUFSIZ];
	if(recv(sock, fileNameLen, sizeof(fileNameLen), 0) < 0){
		fprintf(stderr, "server recv error: %d\n", strerror(errno));
	}
	char *token = strtok(fileNameLen, " ");
	char *fileName = strtok(NULL, " ");
	short int len = ntohs(atoi(token));
	fileName[len - 1] = '\0';
	char *returnNum;	

	FILE *f = fopen(fileName, "r");
	if (f){
		returnNum = "1";
		if(send(sock, returnNum, strlen(returnNum), 0) < 0){
			fprintf(stderr, "Send failed: %d\n", strerror(errno));
			return 1;
		}
		char userAnswer[BUFSIZ];
		if(recv(sock, userAnswer, BUFSIZ, 0) < 0){
			fprintf(stderr, "Recv failed: %d\n", strerror(errno));
			return 1;
		}
		if(strcmp(userAnswer, "Yes\n") == 0){
			if(remove(fileName) < 0){
				returnNum = "-1";
				if(send(sock, returnNum, strlen(returnNum), 0) < 0){
					fprintf(stderr, "Send failed: %d\n", strerror(errno));
					return 1;
				}
				return 0;
			}
			else{
				returnNum = "1";
				if(send(sock, returnNum, strlen(returnNum), 0) < 0){
					fprintf(stderr, "Send failed: %d\n", strerror(errno));
					return 0;
				}
			}
		}
		else{
			printf("Delete abandoned by the user!\n");
		}
		return 0;
	}
	else{
		fprintf(stderr, "fopen failed: %d\n", strerror(errno));
		returnNum = "-1";
		if(send(sock, returnNum, strlen(returnNum), 0) < 0){
			fprintf(stderr, "Send failed: %d\n", strerror(errno));
			return 1;
		}	
	}
		
	return 0;
}


int LS(char *cmd, int sock) {
	DIR *d;
  struct dirent *dir;
	char *curDir;
	char *anything;
	anything = getcwd(curDir,BUFSIZ);
	d = opendir(anything);

    if (d)
    {
    	char buff1[BUFSIZ];
    	char buff2[BUFSIZ];
    	int size = 0;
    	struct stat fileStat;

        while ((dir = readdir(d)) != NULL)
        {
        	if (strcmp(dir->d_name,".")!=0 && strcmp(dir->d_name,"..")!=0) {
	            
			    if(stat(dir->d_name,&fileStat) < 0)    
			        return 1;

			    size+=fileStat.st_size;

				  strcat(buff1, (S_ISDIR(fileStat.st_mode)) ? "d" : "-");
			    strcat(buff1, (fileStat.st_mode & S_IRUSR) ? "r" : "-");
			    strcat(buff1, (fileStat.st_mode & S_IWUSR) ? "w" : "-");
			    strcat(buff1, (fileStat.st_mode & S_IXUSR) ? "x" : "-");
			    strcat(buff1, (fileStat.st_mode & S_IRGRP) ? "r" : "-");
			    strcat(buff1, (fileStat.st_mode & S_IWGRP) ? "w" : "-");
			    strcat(buff1, (fileStat.st_mode & S_IXGRP) ? "x" : "-");
			    strcat(buff1, (fileStat.st_mode & S_IROTH) ? "r" : "-");
			    strcat(buff1, (fileStat.st_mode & S_IWOTH) ? "w" : "-");
			    strcat(buff1, (fileStat.st_mode & S_IXOTH) ? "x " : "- ");

	            strcat(buff1, dir->d_name);
	            strcat(buff1, "\n");
	            
	        }
        }

        sprintf(buff2,"%d",size);
        strcat(buff2,"\n");
        strcat(buff2,buff1);
        send(sock, buff2, sizeof(buff2), 0);
        closedir(d);
      
		memset(buff1, 0, sizeof(buff1));
		memset(buff2, 0, sizeof(buff2));
		//buff1="";
		//buff2="";
    }
	return 0;
}


int MKDIR(char *cmd, int sock) {
	char dirNameLen[BUFSIZ];
	if(recv(sock, dirNameLen, sizeof(dirNameLen), 0) < 0){
		fprintf(stderr, "server recv error: %d\n", strerror(errno));
	}
	char *token = strtok(dirNameLen, " ");
	char *dirName = strtok(NULL, " ");
	short int len = ntohs(atoi(token));
	printf("len: %d", len);
	dirName[len - 1] = '\0';
	char *returnNum;	
	printf("%s\n", dirName);
	DIR *dir = opendir(dirName);
	if (dir){
		closedir(dir);
		returnNum = "-2";
		if(send(sock, returnNum, strlen(returnNum), 0) < 0){
			fprintf(stderr, "Send failed: %d\n", strerror(errno));
			return 1;
		}
		return 0;
	}
	else if (ENOENT == errno){
	    /* Directory does not exist. */
			mkdir(dirName, 0700);
			returnNum = "1";
			if(send(sock, returnNum, strlen(returnNum), 0) < 0){
				fprintf(stderr, "Send failed: %d\n", strerror(errno));
				return 1;
			}	
	}
	else{
		fprintf(stderr, "opendir failed: %d\n", strerror(errno));
		returnNum = "-1";
		if(send(sock, returnNum, strlen(returnNum), 0) < 0){
			fprintf(stderr, "Send failed: %d\n", strerror(errno));
			return 1;
		}	
	}
		
	return 0;
}


int RMDIR(char *cmd, int sock) {
	char dirNameLen[BUFSIZ];
	if(recv(sock, dirNameLen, sizeof(dirNameLen), 0) < 0){
		fprintf(stderr, "server recv error: %d\n", strerror(errno));
	}
	char *token = strtok(dirNameLen, " ");
	char *dirName = strtok(NULL, " ");
	short int len = ntohs(atoi(token));
	dirName[len - 1] = '\0';
	char *returnNum;	

	DIR *dir = opendir(dirName);
	struct dirent *d;
	int count = 0; //every "empty" directory contains only 2 things
	if (dir){
		while((d = readdir(dir)) != NULL){
			if(++count > 2){
				break;
			}
		}
		if(count > 2){
			returnNum = "-2";
			if(send(sock, returnNum, strlen(returnNum), 0) < 0){
				fprintf(stderr, "Send failed: %d\n", strerror(errno));
				return 1;
			}
			return 0;
		}
		returnNum = "1";
		if(send(sock, returnNum, strlen(returnNum), 0) < 0){
			fprintf(stderr, "Send failed: %d\n", strerror(errno));
			return 1;
		}
		char userAnswer[BUFSIZ];
		if(recv(sock, userAnswer, BUFSIZ, 0) < 0){
			fprintf(stderr, "Recv failed: %d\n", strerror(errno));
			return 1;
		}
		if(strcmp(userAnswer, "Yes\n") == 0){
			if(rmdir(dirName) < 0){
				returnNum = "-1";
				if(send(sock, returnNum, strlen(returnNum), 0) < 0){
					fprintf(stderr, "Send failed: %d\n", strerror(errno));
					return 1;
				}
				return 0;
			}
			else{
				returnNum = "1";
				if(send(sock, returnNum, strlen(returnNum), 0) < 0){
					fprintf(stderr, "Send failed: %d\n", strerror(errno));
					return 0;
				}
			}
		}
		else{
			printf("Delete abandoned by the user!\n");
		}
		return 0;
	}
	else if (ENOENT == errno){
	    /* Directory does not exist. */
			returnNum = "-1";
			if(send(sock, returnNum, strlen(returnNum), 0) < 0){
				fprintf(stderr, "Send failed: %d\n", strerror(errno));
				return 1;
			}	
	}
	else{
		fprintf(stderr, "opendir failed: %d\n", strerror(errno));
		returnNum = "-1";
		if(send(sock, returnNum, strlen(returnNum), 0) < 0){
			fprintf(stderr, "Send failed: %d\n", strerror(errno));
			return 1;
		}	
	}
		
	return 0;
}


int CD(char *cmd, int sock) {
	
	printf("in server CD func\n"); //////////////////

	char dirNameLen[BUFSIZ];
	if(recv(sock, dirNameLen, sizeof(dirNameLen), 0) < 0){
		fprintf(stderr, "server recv error: %d\n", strerror(errno));
	}
	char *token = strtok(dirNameLen, " ");
	char *dirName = strtok(NULL, " ");
	short int len = ntohs(atoi(token));
	dirName[len - 1] = '\0';
	char *returnNum;	
	printf("%s\n", dirName); ///////////////////
	if (chdir(dirName) == 0) {
		returnNum = "1";
		if(send(sock, returnNum, strlen(returnNum), 0) < 0){
			fprintf(stderr, "Send failed: %d\n", strerror(errno));
			return 1;
		}
	} else if (errno==ENOENT) {
		returnNum="-2";
		if(send(sock, returnNum, strlen(returnNum), 0) < 0){
			fprintf(stderr, "Send failed: %d\n", strerror(errno));
			return 1;
		}
	} else {
		returnNum="-1";
		if(send(sock, returnNum, strlen(returnNum), 0) < 0){
			fprintf(stderr, "Send failed: %d\n", strerror(errno));
			return 1;
		}
	}
	return 0;
}

