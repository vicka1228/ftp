#include<stdio.h>
#include<stdlib.h>
#include "client_helper.h"
#include<string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void displayIntro() {
	printf("Welcome to the FTP Client!\
		\nPlease type the commands below...\n\n");
}

void handle_port(int cfd, char* PORT_VAL) {
	struct sockaddr_in sa;
	socklen_t sa_len;

	sa_len = sizeof(sa);
	// get client address and port
	if (getsockname(cfd, (struct sockaddr*)&sa, &sa_len) == -1) {
		perror("getsockname() failed");
		return;
	}

	char* host = inet_ntoa(sa.sin_addr);
	int port = (int) ntohs(sa.sin_port) + PORT_OFFSET;

	// replace dots with commas
	for (int i = 0; i < strlen(host); i++) {
		if (host[i] == '.') host[i] = ',';
	}

	// separate port into two values
	int p1 = port / 256;
	int p2 = port % 256;

	sprintf(PORT_VAL, "PORT %s,%d,%d", host,p1,p2);
}

int handle_commands(int fd, char* command, char** CDIR) {
	char* CUR_DIR = *CDIR;

	// copying command into a modifiable variable for convenience
	char client_command[strlen(command) + 1];
	strcpy(client_command, command);

	// this is the base directory starts from on client side
	char* BASE_DIR = "client_dir";
	
	char FULL_DIR[101];
	FULL_DIR[0] = '\0';			// to signify char array as empty string, I can put in null character in the beginning

	// sprintf helps concatenate like printf
	sprintf(FULL_DIR, "%s%s", BASE_DIR, CUR_DIR);		// full dir built up using the base dir which never changes and CUR_DIR which changes with !CWD

	// strtok command gets the value up until the delimiter: the main command
	char* token = strtok(client_command, " ");

	if (strcmp(token, "!LIST") == 0) {
		// char array to store the command to run on the system
		char exec[201];
		exec[0] = '\0';
	
		sprintf(exec, "ls %s", FULL_DIR);
		system(exec);		// this runs the actual command like in terminal
		return 1;			// return 1 to denote that this command is taken care of and no need to send to server
	} else if (strcmp(token, "!PWD") == 0) {
		printf("%s\n", CUR_DIR);		// CUR_DIR is the whole path except root which is CLIENT_DIR
		return 1;
	} else if (strcmp(token, "!CWD") == 0) {
		char* dest = strtok(NULL, "\n");		// get rest of the path
		if (dest == NULL) {
			printf("202 Command not implemented.\n");
			return 1;
		}

		char NEW_DIR[201];			// the full_dir path where you need to go
		NEW_DIR[0] = '\0';

		sprintf(NEW_DIR, "%s%s", BASE_DIR, dest);
		
		char exec[strlen(NEW_DIR) + 4];			// the command to be run
		exec[0] = '\0';

		sprintf(exec, "cd %s", NEW_DIR);

		int return_val = system(exec);			// check if the terminal command was successful
		if (return_val == 0) {
			// potential to change to strcpy after CDIR is fixed
			char TEMP[101];
			TEMP[0] = '\0';
			strcat(TEMP, dest);

			// potentially change to strcpy
			*CDIR = TEMP;

			printf("200 directory changed to %s.\n", *CDIR);

		} else {
			printf("550 No such file or directory.\n");
		}
		return 1;
	} else if (strcmp(token, "RETR") == 0 || strcmp(token, "STOR")==0 || strcmp(token, "LIST")==0) {
		char PORT_VAL[50];
		bzero(PORT_VAL, sizeof(PORT_VAL));

		// function to generate the PORT command and store in PORT_VAL
		handle_port(fd, PORT_VAL);

		if(send(fd,PORT_VAL,strlen(PORT_VAL),0)<0)
		{
			perror("send");
			exit(-1);
		}

		char buffer[256];
		bzero(buffer,sizeof(buffer));	
		int bytes = recv(fd,buffer,sizeof(buffer),0);
		PORT_OFFSET+=1;
		printf("%s\n", buffer);

		return 0; //because it needs to be handled on server after this
	}

	return 0;
}

void handle_retr(int transfer_sd, char* filename) {
	int bytes_read;
	char buffer[1024];
	bzero(buffer, sizeof(buffer));
	// Open file for writing
    FILE* file = fopen(filename, "wb");
    if (file == NULL) {
        perror("fopen");
        exit(-1);
    }

    // Receive data from server and write to file
    while ((bytes_read = recv(transfer_sd, buffer, 1024, 0)) > 0) {
        fwrite(buffer, sizeof(char), bytes_read, file);
    }

    // Close file
    fclose(file);

    // Receive response from server
    // if ((bytes_read = recv(transfer_sd, buffer, BUFFER_SIZE, 0)) == -1) {
    //     perror("recv");
    //     exit(EXIT_FAILURE);
    // }
    // buffer[bytes_read] = '\0';
    // printf("%s\n", buffer);

    // Close socket
    // close(sockfd);

    return;
}

void handle_stor(int transfer_sd, char* filename, char** CDIR) {
	char* CUR_DIR = *CDIR;

	// this is the base directory starts from on client side
	char* BASE_DIR = "client_dir";
	
	char FULL_DIR[101];
	FULL_DIR[0] = '\0';			// to signify char array as empty string, I can put in null character in the beginning

	// sprintf helps concatenate like printf
	sprintf(FULL_DIR, "%s%s%s", BASE_DIR, CUR_DIR, filename);		// full dir built up using the base dir which never changes and CUR_DIR which changes with !CWD
	printf("file path: %s\n", FULL_DIR);
	// Open file for reading
	printf("inside handle_stor\n");
	int bytes_read;
	char buffer[1024];
	bzero(buffer, sizeof(buffer));
	printf("Buffer: %s\n", buffer);
    FILE* file = fopen(FULL_DIR, "rb");
	// FILE* file = fopen(filename, "rb");
    if (file == NULL) {
		printf("inside file null\n");
        // perror("fopen");
        // exit(-1);
		printf("550 No such file or directory.\n");
		exit(-1);
    }

    // Read data from file and send to server
    while ((bytes_read = fread(buffer, sizeof(char), 1024, file)) > 0) {
		printf("inside bytes read\n");
        if (send(transfer_sd, buffer, bytes_read, 0) == -1) {
            perror("send");
            exit(-1);
        }
    }

	// if (send(transfer_sd, "end", strlen("end"), 0) == -1) {
    //         perror("send");
    //         exit(-1);
    // }
	printf("after bytes read\n");
	// char msg[256];
	// char* msg = malloc(256);
	// bzero(msg, 256);
	// recv(transfer_sd,msg,256,0);
	// printf("msg: %s\n", msg);
	// free(msg);
	// bzero(buffer,sizeof(buffer));
	// recv(transfer_sd,buffer,sizeof(buffer),0);
	// printf("%s\n", buffer);
	return;
}

void handle_list(int transfer_sd) {
	int bytes_read;
	char buffer[1024];
	bzero(buffer, sizeof(buffer));
	// Receive directory listing from server and print to console
    while ((bytes_read = recv(transfer_sd, buffer, 1024, 0)) > 0) {
        fwrite(buffer, sizeof(char), bytes_read, stdout);
    }
}