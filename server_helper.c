#include<stdio.h>
#include<stdlib.h>
#include "server_helper.h"
#include<string.h>

#define BUFFER_SIZE 1024

char* handle_messages(int code) {
	switch (code)
	{
	case 200:
		return "200 directory changed to ";
		break;
	case 202:
		return "202 Command not implemented.";
		break;
	case 230:
		return "230 User logged in, proceed.";
		break;
	case 257:
		return "257 ";
		break;
	case 331:
		return "331 Username OK, need password.";
		break;
	case 503:
		return "503 Bad sequence of commands.";
		break;
	case 530:
		return "530 Not Logged In";
		break;
	case 550:
		return "550 No such file or directory.";
		break;
	default:
		return "Unexpected Error";
		break;
	}
}

char* handle_user(int fd) {
	char* username = strtok(NULL, " ");
	printf("Handle User %s\n", username);
	char* response = handle_messages(530);

	if (username != NULL) {
		if (check_user_pass(username, NULL) == 1) {
			response = handle_messages(331);
			session[fd].state = 1;
			strcpy(session[fd].uname, username);
		}
	}

	return response;
}

char* handle_pass(int fd) {
	char* password = strtok(NULL, " ");
	char* response = handle_messages(530);

	if (check_user_pass(session[fd].uname, password) == 1) {
		response = handle_messages(230);
		session[fd].state = 2;			// state == 2 means user is now authenticated
	}

	return response;
}

char* handle_stor(int fd) {
	return "NA";
}

char* handle_retr(int fd) {
	return "NA";
}

char* handle_list(int fd) {
	char command[100];
	command[0] = '\0';
	char buffer[BUFFER_SIZE];
    char output[BUFFER_SIZE];
    FILE* fp;

	strcat(command, "ls ");

    fp = popen(command, "r");
    if (fp == NULL) {
        fprintf(stderr, "Error executing command\n");
        exit(1);
    }

    while (fgets(buffer, BUFFER_SIZE, fp) != NULL) {
        strcat(output, buffer);
    }

    pclose(fp);

	printf("Output:\n%s\n", output);
	return "NA";
}

char* handle_cwd(int fd) {
	char* BASE_DIR = "server_dir";
	char* dest = strtok(NULL, "\n");

	if (dest == NULL) {
		return handle_messages(202);
	}

	char NEW_DIR[BUFFER_SIZE];
	NEW_DIR[0] = '\0';

	sprintf(NEW_DIR, "%s%s", BASE_DIR, dest);

	char exec[strlen(NEW_DIR) + 4];
	exec[0] = '\0';

	sprintf(exec, "cd %s", NEW_DIR);

	int return_val = system(exec);

	if (return_val == 0) {
		char TEMP[50];
		TEMP[0] = '\0';
		strcat(TEMP, dest);
		strcpy(CUR_DIR, TEMP);
		
		char* response =  handle_messages(200);
		char* TEMP_RES = malloc(BUFFER_SIZE);
		bzero(TEMP_RES, sizeof(TEMP_RES));
		strcpy(TEMP_RES, response);

		strcat(TEMP_RES, CUR_DIR);
		return TEMP_RES;	
		
	} else {
		return handle_messages(550);
	}
}

char* handle_pwd(int fd) {
	char* response = handle_messages(257);

	char* TEMP = malloc(BUFFER_SIZE);
	bzero(TEMP, sizeof(TEMP));
	strcpy(TEMP, response);

	strcat(TEMP, CUR_DIR);
	return TEMP;	
}

char* handle_quit(int fd) {
	return "NA";
}

char* handle_commands(int fd, char* command) {
	char* token = strtok(command, " ");
	char* response;
	int authenticated = session[fd].state;

	if(strcmp(token, "USER") == 0) {
		// state == 2 means the user is fully authenticated.
		response = authenticated != 2 ? handle_user(fd) : handle_messages(503);

	} else if (strcmp(token, "PASS") == 0) {
		// state == 1 means the user has entered valid username, but password hasn't been entered yet.	
		response = authenticated == 1 ? handle_pass(fd) : handle_messages(503);
	} else if (strcmp(token, "STOR") == 0) {
		response = authenticated == 2 ? handle_stor(fd) : handle_messages(503);
	} else if (strcmp(token, "RETR") == 0) {
		response = authenticated == 2 ? handle_retr(fd) : handle_messages(503);
	} else if (strcmp(token, "LIST") == 0) {
		response = authenticated == 2 ? handle_list(fd) : handle_messages(503);
	} else if (strcmp(token, "CWD") == 0) {
		response = authenticated == 2 ? handle_cwd(fd) : handle_messages(503);
	} else if (strcmp(token, "PWD") == 0) {
		response = authenticated == 2 ? handle_pwd(fd) : handle_messages(503);
	} else if (strcmp(token, "QUIT") == 0) {		// QUIT handled server side
		response = authenticated == 2 ? handle_cwd(fd) : handle_messages(503);
	} else {
		response = handle_messages(202);
	}

	return response;
}

// Send password for PASS command to check for username and password
// Send NULL for USER command to only check for user
int check_user_pass(char* username, char* password) {
	FILE* user_file = fopen("users.txt", "r");
	char user_pass[100];

	if (user_file == NULL)
	{
		printf("No such file");
		return 1;
	}
	
	char* upass_ptr;
	int return_num = 0;

	while (fgets(user_pass, sizeof user_pass, user_file) != NULL)
	{
		char* uname = strtok(user_pass, ",");
		char* upass = strtok(NULL, "\n");

		if (strcmp(uname, username) == 0) {
			if (password == NULL) {
				return_num = 1;
				break;
			} else {
				if (strcmp(upass, password) == 0) {
					return_num = 1;
					break;
				}
			}
		}
	}        

	fclose(user_file);
	return return_num;
}