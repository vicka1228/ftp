#include<stdio.h>
#include<stdlib.h>
#include "server_helper.h"
#include<string.h>
#include <dirent.h>

#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<sys/select.h>
#include<unistd.h>

#define BUFFER_SIZE 1024

char* handle_messages(int code) {
	char* code_message = malloc(BUFFER_SIZE);
	// bzero(code_message, sizeof(code_message));
	bzero(code_message, BUFFER_SIZE);

	switch (code)
	{
	case 200:
		strcpy(code_message,"200 directory changed to ");
		break;
	case 201:			// although message is 200, changing it to 201 to avoid conflict
		strcpy(code_message,"200 PORT command successful.");
		break;
	case 202:
		strcpy(code_message,"202 Command not implemented.");
		break;
	case 226:
		strcpy(code_message,"226 Transfer completed.");
		break;
	case 230:
		strcpy(code_message,"230 User logged in, proceed.");
		break;
	case 257:
		strcpy(code_message,"257 ");
		break;
	case 331:
		strcpy(code_message,"331 Username OK, need password.");
		break;
	case 501:
		strcpy(code_message,"501 Syntax error in parameters or arguments.");
		break;
	case 503:
		strcpy(code_message,"503 Bad sequence of commands.");
		break;
	case 504:
		strcpy(code_message,"504 Command not implemented for that parameter.");
		break;
	case 530:
		strcpy(code_message,"530 Not Logged In");
		break;
	case 550:
		strcpy(code_message,"550 No such file or directory.");
		break;
	case 551:
		strcpy(code_message,"551 Requested action aborted: page type unknown.");
		break;
	default:
		strcpy(code_message,"Unexpected Error");
		break;
	}

	return code_message;
}

char* handle_user(int fd) {
	char* username = strtok(NULL, " ");
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
		// store directory for the client 
		sprintf(session[fd].CUR_DIR, "server_dir/%s/", session[fd].uname);
	}

	return response;
}

char* handle_port(int fd) {
	
	char* host_id = strtok(NULL, " ");
	
	// printf("%s\n", host_id);

	//parsing
	char* h1 = strtok(host_id, ",");
	char* h2 = strtok(NULL, ",");
	char* h3 = strtok(NULL, ",");
	char* h4 = strtok(NULL, ",");
	char* p1 = strtok(NULL, ",");
	char* p2 = strtok(NULL, ",");

	// STORE THE PORT FOR THE CLIENT
	session[fd].port = (atoi(p1)*256)+atoi(p2);

	char host[50];
	sprintf(host, "%s.%s.%s.%s", h1, h2, h3, h4);
	// store host for client
	strcpy(session[fd].host, host);
	
	char* response = handle_messages(201);
	return response;
}

char* handle_stor(int fd, int data_sd) {
	char buffer[BUFFER_SIZE];
	bzero(buffer, sizeof(buffer));
	printf("Buffer: %s\n", buffer);
    int bytes_read;
    FILE* file;

	char* response = handle_messages(551);
	char* filename = strtok(NULL, "\n");

	char FILE_DIR[BUFFER_SIZE];
	FILE_DIR[0] = '\0';
	
	sprintf(FILE_DIR, "%s%s", session[fd].CUR_DIR, filename);
	// printf("file path: %s\n", FILE_DIR);
    // Open file for writing
    file = fopen(FILE_DIR, "wb"); 
    if (file == NULL) {
        return response;
    }
	
    // Read data from client and write to file
    while ((bytes_read = recv(data_sd, buffer, BUFFER_SIZE, 0)) > 0) {
		// printf("write\n");
        fwrite(buffer, sizeof(char), bytes_read, file);
		// printf("after fwrite\n");
    }
	

	// printf("after bytes read\n");
    // Close file
    fclose(file);
	bzero(response, strlen(response));
	response = handle_messages(226);
	return response;
	// return;
}

char* handle_retr(int fd, int data_sd) {
	char buffer[BUFFER_SIZE];
	bzero(buffer, sizeof(buffer));
    int bytes_read;
    FILE* file;

	char* response = handle_messages(550);
	// printf("%s\n", response);
	char* filename = strtok(NULL, "\n");
	// printf("%s\n", filename);
	
	char FILE_DIR[BUFFER_SIZE];
	FILE_DIR[0] = '\0';

	sprintf(FILE_DIR, "%s%s", session[fd].CUR_DIR, filename);
	
    // Open file for reading
	 if (access(FILE_DIR, F_OK) == 0) {
		file = fopen(FILE_DIR, "rb");
    	if (file == NULL) {
			return response;
		}
	 }
    else {
		return response;
	}

    // Read data from file and send to client
    while ((bytes_read = fread(buffer, sizeof(char), BUFFER_SIZE, file)) > 0) {
        if (send(data_sd, buffer, bytes_read, 0) == -1) {
            fclose(file);
			response = handle_messages(551);
            return response;
        }
    }

    // Close file
    fclose(file);
	response = handle_messages(226);
    return response;
}

char* handle_list(int fd, int data_sd) {

	char buffer[BUFFER_SIZE];
    int bytes_written;
    DIR* dir;
    struct dirent* entry;

    // Open directory
    dir = opendir(session[fd].CUR_DIR);
	char* response = handle_messages(550);
    if (dir == NULL) {
        return response;
    }

    // Read directory entries and send to client
    while ((entry = readdir(dir)) != NULL) {
		if (entry->d_name[0] != '.') { // exclude hidden files
			snprintf(buffer, BUFFER_SIZE, "%s\n", entry->d_name);
			bytes_written = send(data_sd, buffer, strlen(buffer), 0);
			if (bytes_written == -1) {
				closedir(dir);
				response = handle_messages(551);
				return response;
			}
		}
    }

    // Close directory
    closedir(dir);

    response = handle_messages(226);
    return response;
}

char* handle_data(int fd, int token) {

	char* response = malloc(BUFFER_SIZE);
	bzero(response, BUFFER_SIZE);
	int pid = fork(); //fork a child process
	int status;
	if(pid == 0)   //if it is the child process
	{
		// close(session[fd].server_sd);
		//potentially check file status here?
		if(send(fd,"150 File status okay; about to open data connection.",strlen("150 File status okay; about to open data connection."),0)<0) //sending the message to client
        {
            perror("send");
            exit(-1);
        }

		char buffer[256];
		recv(fd,buffer,sizeof(buffer),0);
		
		int data_sd = socket(AF_INET,SOCK_STREAM,0);
		printf("Data sd = %d \n",data_sd);
		if(data_sd<0)
		{
			perror("socket:");
			exit(-1);
		}
		//setsock for data connection
		int value  = 1;
		setsockopt(data_sd,SOL_SOCKET,SO_REUSEADDR,&value,sizeof(value)); //&(int){1},sizeof(int)


		struct sockaddr_in serv_data_addr;
		bzero(&serv_data_addr,sizeof(serv_data_addr));
		serv_data_addr.sin_family = AF_INET;
		serv_data_addr.sin_port = htons(9000);
		serv_data_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); //INADDR_ANY, INADDR_LOOP
		
		// printf("before bind\n");
		//bind
		if(bind(data_sd, (struct sockaddr*)&serv_data_addr,sizeof(serv_data_addr))<0)
		{
			perror("bind failed");
			exit(-1);
		}

		//store client data exchange address
		struct sockaddr_in cli_data_addr; 
		bzero(&cli_data_addr, sizeof(cli_data_addr)); 
		cli_data_addr.sin_family = AF_INET;
		// printf("before port\n");
		// printf("%d\n", session[fd].port);
		// printf("%s\n", session[fd].host);
		cli_data_addr.sin_port = htons(session[fd].port);
		cli_data_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); //potentially change
		socklen_t len = sizeof(serv_data_addr); 
		// connect data exchange socket to client address for data exchange

		// printf("before connect\n");
		if (connect(data_sd, (struct sockaddr*)&cli_data_addr, sizeof(cli_data_addr)) < 0)
		{
			perror("connect");
			exit(-1);
		}
		// printf("after connect\n");

		if(send(data_sd,"hi from server data",strlen("hi from server data"),0)<0) //sending the message to server
        {
            perror("send");
            exit(-1);
        }

		bzero(buffer, sizeof(buffer));
		recv(data_sd,buffer,sizeof(buffer),0);
		// printf("%s\n", buffer);

		//stor
		if (token == 0) {

			response = handle_stor(fd, data_sd);
			// printf("response in token 0: %s\n", response);
			if(send(fd,response,strlen(response),0)<0) //sending the message to client
			{
				perror("send");
				exit(-1);
			}
			
		}
		// retr
		else if (token == 1) {
			response = handle_retr(fd, data_sd);
			// printf("response in token 1: %s\n", response);
			close(data_sd);
			if(send(fd,response,strlen(response),0)<0) //sending the message to client
			{
				perror("send");
				exit(-1);
			}
		}
		//list
		else if (token == 2) {
			response = handle_list(fd, data_sd);
			// printf("response in token 2: %s\n", response);
			close(data_sd);
			if(send(fd,response,strlen(response),0)<0) //sending the message to client
			{
				perror("send");
				exit(-1);
			}
		}
	
		close(data_sd);
		exit(0);
	}
	else {
		wait(&status);
		return response;
	}


}




char* handle_cwd(int fd) {
	
	char* dest = strtok(NULL, "\n");
	if (dest == NULL) {
		return handle_messages(501); 
	}
	
	char NEW_DIR[BUFFER_SIZE];
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
	//not implementing so that you cannot switch user directories
	if(strcmp(dest, "..")==0){
		return handle_messages(504);
	}

	sprintf(NEW_DIR, "%s%s", session[fd].CUR_DIR, dest);
	printf("%s\n", NEW_DIR);

	char exec[strlen(NEW_DIR) + 4];
	exec[0] = '\0';

	sprintf(exec, "cd %s", NEW_DIR);

	int return_val = system(exec);

	if (return_val == 0) {
		char TEMP[101];
		TEMP[0] = '\0';
		strcat(TEMP, dest);
		// strcpy(CUR_DIR, TEMP);
		if(strcmp(dest,".")!=0){

			strcpy(session[fd].CUR_DIR, NEW_DIR);
				// sprintf(CUR_DIR, "%s/", CUR_DIR);
				strcat(session[fd].CUR_DIR, "/");
			
		}
	
		char* response =  handle_messages(200);
		char* TEMP_RES = malloc(BUFFER_SIZE);
		// bzero(TEMP_RES, sizeof(TEMP_RES));
		bzero(TEMP_RES, BUFFER_SIZE);
		strcpy(TEMP_RES, response);

		strcat(TEMP_RES, NEW_DIR);
		return TEMP_RES;	
		
	} else {
		return handle_messages(550);
	}
}

char* handle_pwd(int fd) {
	char* response = handle_messages(257);

	char* TEMP = malloc(BUFFER_SIZE);
	// bzero(TEMP, sizeof(TEMP));
	bzero(TEMP, BUFFER_SIZE);
	strcpy(TEMP, response);
	strcat(TEMP, session[fd].CUR_DIR);
	return TEMP;	
}

char* handle_quit(int fd) {
	return "NA";
}

void handle_commands(int fd, char* command, char* message) {
	char* token = strtok(command, " ");			// get the string before space, the command
	char* response;			// a temporary char array to store the response
	int authenticated = session[fd].state;			// value to denote authentication for session

	if(strcmp(token, "USER") == 0) {
		// state == 2 means the user is fully authenticated.
		response = authenticated != 2 ? handle_user(fd) : handle_messages(503);
	} else if (strcmp(token, "PASS") == 0) {
		// state == 1 means the user has entered valid username, but password hasn't been entered yet.	
		response = authenticated == 1 ? handle_pass(fd) : handle_messages(503);
	} else if (strcmp(token, "PORT") == 0) {
		response = authenticated == 2 ? handle_port(fd) : handle_messages(503);
	} else if (strcmp(token, "STOR") == 0) {
		response = authenticated == 2 ? handle_data(fd, 0) : handle_messages(503);
	} else if (strcmp(token, "RETR") == 0) {
		response = authenticated == 2 ? handle_data(fd, 1) : handle_messages(503);
	} else if (strcmp(token, "LIST") == 0) {
		response = authenticated == 2 ? handle_data(fd, 2) : handle_messages(503);
	} else if (strcmp(token, "CWD") == 0) {
		response = authenticated == 2 ? handle_cwd(fd) : handle_messages(503);
	} else if (strcmp(token, "PWD") == 0) {
		response = authenticated == 2 ? handle_pwd(fd) : handle_messages(503);
	} else if (strcmp(token, "QUIT") == 0) {		// QUIT handled server side
		response = authenticated == 2 ? handle_cwd(fd) : handle_messages(503);
	} else {
		response = handle_messages(202);
	}

	strcpy(message, response);
	bzero(response, strlen(response));
	// free(response);
}

// Send password for PASS command to check for username and password
// Send NULL for USER command to only check for user
int check_user_pass(char* username, char* password) {
	FILE* user_file = fopen("users.txt", "r");
	char user_pass[100];			// array to store the lines from the file

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