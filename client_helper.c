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
	int sa_len;

	sa_len = sizeof(sa);
	if (getsockname(cfd, (struct sockaddr*)&sa, &sa_len) == -1) {
		perror("getsockname() failed");
		return;
	}

	char* host = inet_ntoa(sa.sin_addr);
	int port = (int) ntohs(sa.sin_port) + PORT_OFFSET;

	for (int i = 0; i < strlen(host); i++) {
		if (host[i] == '.') host[i] = ',';
	}

	int p1 = port / 256;
	int p2 = port % 256;

	sprintf(PORT_VAL, "PORT %s,%d,%d", host,p1,p2);
}

int handle_commands(int fd, char* command, char** CDIR) {
	char* CUR_DIR = *CDIR;

	char client_command[strlen(command) + 1];
	strcpy(client_command, command);

	char* BASE_DIR = "client_dir";
	int arr_size = strlen(BASE_DIR) + strlen(CUR_DIR);
	
	char FULL_DIR[100 + 1];
	FULL_DIR[0] = '\0';

	sprintf(FULL_DIR, "%s%s", BASE_DIR, CUR_DIR);

	
	char* token = strtok(client_command, " ");

	if (strcmp(token, "!LIST") == 0) {
		// char exec[strlen("ls ") + strlen(FULL_DIR) + 1];
		char exec[200 + 1];
		exec[0] = '\0';
	
		sprintf(exec, "ls %s", FULL_DIR);
		system(exec);
		return 1;
	} else if (strcmp(token, "!PWD") == 0) {
		printf("%s\n", CUR_DIR);
		return 1;
	} else if (strcmp(token, "!CWD") == 0) {
		char* dest = strtok(NULL, "\n");
		if (dest == NULL) {
			printf("202 Command not implemented.\n");
			return 1;
		}

		// char NEW_DIR[strlen(BASE_DIR) + strlen(dest) + 1];
		char NEW_DIR[200+ 1];
		NEW_DIR[0] = '\0';

		sprintf(NEW_DIR, "%s%s", BASE_DIR, dest);
		
		char exec[strlen(NEW_DIR) + 4];
		exec[0] = '\0';

		sprintf(exec, "cd %s", NEW_DIR);

		int return_val = system(exec);
		if (return_val == 0) {
			// char TEMP[strlen(dest) + 1];
			char TEMP[100+ 1];
			TEMP[0] = '\0';
			strcat(TEMP, dest);

			*CDIR = TEMP;

			printf("200 directory changed to %s.\n", *CDIR);

		} else {
			printf("550 No such file or directory.\n");
		}
		return 1;
	} else if (strcmp(token, "RETR") == 0) {
		char PORT_VAL[50];
		bzero(PORT_VAL, sizeof(PORT_VAL));

		handle_port(fd, PORT_VAL);

		if(send(fd,PORT_VAL,strlen(PORT_VAL),0)<0)
		{
			perror("send");
			exit(-1);
		}

		char buffer[256];
		bzero(buffer,sizeof(buffer));	
		int bytes = recv(fd,buffer,sizeof(buffer),0);
		printf("%s\n", buffer);
		return 1;
	}

	return 0;
}