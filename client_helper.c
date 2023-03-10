#include<stdio.h>
#include<stdlib.h>
#include "client_helper.h"
#include<string.h>

void displayIntro() {
	printf("Welcome to the FTP Client!\
		\nPlease type the commands below...\n\n");
}

int handle_commands(char* command) {
	char* client_command;
	strcpy(client_command, command);

	char* token = strtok(client_command, " ");
	char* response;

	if (strcmp(token, "!LIST") == 0) {
		system("ls client_dir");
		return 1;
	} else if (strcmp(token, "!PWD") == 0) {
		system("cd client_dir && pwd");
		return 1;
	}

	return 0;
}