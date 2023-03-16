//============================================================================
// Name         : Chat Server using Select()
// Description  : This Program will receive messages from several clients using
//                select() system class
//============================================================================
#include<stdio.h>
#include<string.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<sys/select.h>
#include<sys/stat.h>
#include<unistd.h>
#include<stdlib.h>

#include "server_helper.h"

#define MAX_USERS 100

// defining the global extern variables
struct User session[MAX_USERS];
char CUR_DIR[256];

int main()
{
	bzero(CUR_DIR, sizeof(CUR_DIR));
	// storing / as the cur directory in CUR_DIR global variable
	// CUR_DIR[0] = '/';
	// CUR_DIR[1] = '\0';

	strcpy(CUR_DIR, "server_dir/");
	printf("%s\n", CUR_DIR);

	bzero(session, sizeof(session));

	FILE *fp;
    char line[1024];
    char *username;
    char dir_path[1024];

    // Open the users.txt file for reading
    fp = fopen("users.txt", "r");
    if (fp == NULL) {
        perror("Failed to open users.txt");
        exit(EXIT_FAILURE);
    }

    // Parse each line of the file
    while (fgets(line, 1024, fp) != NULL) {
        // Get the username (the first token)
        username = strtok(line, ",");
        if (username == NULL) {
            fprintf(stderr, "Invalid line in users.txt\n");
            continue;
        }

        // Create a directory with the username under /server_dir if it doesn't already exist
        snprintf(dir_path, 1024, "server_dir/%s", username);
        if (access(dir_path, F_OK) != 0) {
            if (mkdir(dir_path, 0755) != 0) {
                perror("Failed to create directory");
                exit(EXIT_FAILURE);
            }
            printf("Created directory: %s\n", dir_path);
        }

		}

    // Close the file
    fclose(fp);

	//socket
	int server_sd = socket(AF_INET,SOCK_STREAM,0);
	printf("Server fd = %d \n",server_sd);
	if(server_sd<0)
	{
		perror("socket:");
		exit(-1);
	}
	//setsock
	int value  = 1;
	setsockopt(server_sd,SOL_SOCKET,SO_REUSEADDR,&value,sizeof(value)); //&(int){1},sizeof(int)
	struct sockaddr_in server_addr;
	bzero(&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(7000);
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); //INADDR_ANY, INADDR_LOOP

	//bind
	if(bind(server_sd, (struct sockaddr*)&server_addr,sizeof(server_addr))<0)
	{
		perror("bind failed");
		exit(-1);
	}
	//listen
	if(listen(server_sd,5)<0)
	{
		perror("listen failed");
		close(server_sd);
		exit(-1);
	}
	
	fd_set full_fdset;
	fd_set read_fdset;
	fd_set write_fdset;

	FD_ZERO(&full_fdset);

	int max_fd = server_sd;

	FD_SET(server_sd,&full_fdset);

	printf("Server is listening...\n");
	while(1)
	{
		printf("max fd = %d \n",max_fd);
		read_fdset = full_fdset;

		if(select(max_fd+1,&read_fdset,&write_fdset,NULL,NULL)<0)
		{
			perror("select");
			exit (-1);
		}

		for(int fd = 3 ; fd<=max_fd; fd++)
		{
			int read_set = FD_ISSET(fd,&read_fdset);

			if(read_set)
			{
				if(fd==server_sd)
				{
					int client_sd = accept(server_sd,0,0);
					printf("Client Connected fd = %d \n",client_sd);
					session[fd].server_sd = server_sd; 
					// printf("here\n");
					
					FD_SET(client_sd,&full_fdset);
					
					if(client_sd>max_fd)	
						max_fd = client_sd;
					
				}
				else
				{
					char buffer[256];		// variable to store incoming message
					
					bzero(buffer,sizeof(buffer));
					int bytes = recv(fd,buffer,sizeof(buffer),0); 
					// also potentially reset client session here
					if(bytes==0) // to handle Ctrl+C from client side
					{
						bzero(&session[fd], sizeof(session[fd]));
						close(fd);
						printf("connection closed from client\n");
						FD_CLR(fd,&full_fdset);
						if(fd==max_fd)
						{
							for(int i=max_fd; i>=3; i--)
								if(FD_ISSET(i,&full_fdset))
								{
									max_fd =  i;
									break;
								}
						}
					}
					// also have to abort data connection here as well potentially
					if(strcmp(buffer, "QUIT")==0)   //client wants to close the connection
					{
						char* msg = "221 Service closing control connection.";
						if(send(fd, msg, strlen(msg), 0)<0)
						{
							perror("send");
							exit(-1);
						}
						bzero(&session[fd], sizeof(session[fd]));
						close(fd);
						printf("connection closed from client\n");
						FD_CLR(fd,&full_fdset);
						if(fd==max_fd)
						{
							for(int i=max_fd; i>=3; i--)
								if(FD_ISSET(i,&full_fdset))
								{
									max_fd =  i;
									break;
								}
						}
					} else {
						char* msg = malloc(1024);		// char array to store server responses
						// bzero(msg, sizeof(msg));
						bzero(msg, 1024);
						handle_commands(fd, buffer, msg);		// handle server commands

						if(send(fd, msg, strlen(msg), 0)<0)
						{
							perror("send");
							exit(-1);
						}

						free(msg);
					}
				}
			}
		}

	}

	//close
	close(server_sd);
	return 0;
}
