#include<stdio.h>
#include<stdlib.h>
#include "client_helper.h"
#include<string.h>

void displayIntro() {
	printf("Welcome to the FTP Client!\
		\nPlease type the commands below...\n\n");
}

int handle_commands(char* command) {
	// char* CUR_DIR = *CDIR;

	char client_command[strlen(command) + 1];
	strcpy(client_command, command);

	char* BASE_DIR = "client_dir";
	int arr_size = strlen(BASE_DIR) + strlen(CUR_DIR);
	
	char FULL_DIR[arr_size + 1];
	FULL_DIR[0] = '\0';

	sprintf(FULL_DIR, "%s%s", BASE_DIR, CUR_DIR);
	
	char* token = strtok(client_command, " ");

	if (strcmp(token, "!LIST") == 0) {
		char exec[strlen("ls ") + strlen(FULL_DIR) + 1];
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
			return 0;
		}

		char NEW_DIR[strlen(BASE_DIR) + strlen(dest) + 1];
		NEW_DIR[0] = '\0';

		sprintf(NEW_DIR, "%s%s", BASE_DIR, dest);

		// printf("New Dir: %s", NEW_DIR);
		
		char exec[strlen(NEW_DIR) + 4];
		exec[0] = '\0';

		sprintf(exec, "cd %s", NEW_DIR);

		int return_val = system(exec);
		if (return_val == 0) {
			char TEMP[strlen(dest) + 1];
			TEMP[0] = '\0';
			strcat(TEMP, dest);

			CUR_DIR = TEMP;

			printf("200 directory changed to %s.\n", CUR_DIR);

		} else {
			printf("550 No such file or directory.\n");
		}
		return 1;
	}

	return 0;
}