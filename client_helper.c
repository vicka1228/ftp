#include<stdio.h>
#include<stdlib.h>
#include "client_helper.h"
#include<string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

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

int handle_commands(int fd, char* command) {

	// copying command into a modifiable variable for convenience
	char client_command[strlen(command) + 1];
	strcpy(client_command, command);

	// strtok command gets the value up until the delimiter: the main command
	char* token = strtok(client_command, " ");

	if (strcmp(token, "!LIST") == 0) {
		// char array to store the command to run on the system
		char exec[201];
		exec[0] = '\0';
	
		sprintf(exec, "ls %s", CUR_DIR);
		system(exec);		// this runs the actual command like in terminal
		return 1;			// return 1 to denote that this command is taken care of and no need to send to server
	} else if (strcmp(token, "!PWD") == 0) {
		printf("%s\n", CUR_DIR);		// CUR_DIR is the whole path except root which is CLIENT_DIR
		return 1;
	} else if (strcmp(token, "!CWD") == 0) {
		char* dest = strtok(NULL, "\n");		// get rest of the path
		if (dest == NULL) {
			printf("501 Syntax error in parameters or arguments.\n");
			return 1;
		}

		char NEW_DIR[401];			// the full_dir path where you need to go
		NEW_DIR[0] = '\0';

		if(dest[0]=='/'){
			while(dest[0]=='/'){
				int i;
				for (i = 0; dest[i] != '\0'; i++) {
					dest[i] = dest[i+1];
				}
			}
			
		}
		int len = strlen(dest);
		if (len > 0) {
			if(dest[len-1]=='/'){
				dest[len-1] = '\0';
			}        
		}

		if(strcmp(dest, "..")==0){
			printf("504 Command not implemented for that parameter.\n");
			return 1;
		}	


		sprintf(NEW_DIR, "%s%s", CUR_DIR, dest);
		printf("%s\n", NEW_DIR);
		
		char exec[strlen(NEW_DIR) + 4];			// the command to be run
		exec[0] = '\0';

		sprintf(exec, "cd %s", NEW_DIR);

		int return_val = system(exec);			// check if the terminal command was successful
		if (return_val == 0) {

			char TEMP[101];
			TEMP[0] = '\0';
			strcat(TEMP, dest);

			if(strcmp(dest,".")!=0){

				strcpy(CUR_DIR, NEW_DIR);
				strcat(CUR_DIR, "/");
			
			}

			printf("200 directory changed to %s\n", NEW_DIR);

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
	// char* BASE_DIR = "client_dir";
	char FILE_DIR[1024];
	FILE_DIR[0] = '\0';

	sprintf(FILE_DIR, "%s%s", CUR_DIR, filename);
	// printf("file path: %s\n", FILE_DIR);
	int bytes_read;
	char buffer[1024];
	bzero(buffer, sizeof(buffer));
	// Open file for writing
    FILE* file = fopen(FILE_DIR, "wb");
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

    return;
}

void handle_stor(int transfer_sd, char* filename) {
	
	char FILE_DIR[1024];
	FILE_DIR[0] = '\0';

	sprintf(FILE_DIR, "%s%s", CUR_DIR, filename);
	// printf("file path: %s\n", FILE_DIR);
	// Open file for reading

	int bytes_read;
	char buffer[1024];
	bzero(buffer, sizeof(buffer));
	
    FILE* file = fopen(FILE_DIR, "rb");
	// FILE* file = fopen(filename, "rb");
    if (file == NULL) {
		// perror("fopen");
		printf("550 No such file or directory, creating one.\n");
		exit(-1);
		// return;
    }


    // Read data from file and send to server
    while ((bytes_read = fread(buffer, sizeof(char), 1024, file)) > 0) {
		
        if (send(transfer_sd, buffer, bytes_read, 0) == -1) {
            perror("send");
            exit(-1);
        }
    }

	
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