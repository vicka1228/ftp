#include<stdio.h>
#include<string.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include <netinet/in.h>

#include<unistd.h>
#include<stdlib.h>

#include "client_helper.h"

// char* CUR_DIR = "/";
int PORT_OFFSET = 1;

int main()
{
	displayIntro();
	// potentially change to fixed size
	char* CUR_DIR = "/";		// using CUR_DIR here because globals need to have a fixed size

	//socket
	int server_sd = socket(AF_INET,SOCK_STREAM,0);
	printf("server sd = %d \n",server_sd);
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

	//connect
    if(connect(server_sd,(struct sockaddr*)&server_addr,sizeof(server_addr))<0)
    {
        perror("connect");
        exit(-1);
    } else {
		printf("220 Service ready for new user.\n");
	}
	
	//accept
	char buffer[256];

	while(1)
	{
	   printf("ftp> ");
       fgets(buffer,sizeof(buffer),stdin);
       buffer[strcspn(buffer, "\n")] = 0;  //remove trailing newline char from buffer, fgets does not remove it

		// client_task signifier that a command is handled on client_side vs server_side
		// if 0 it's server_side, if 1 it's client_side
	   int client_task = handle_commands(server_sd, buffer, &CUR_DIR);

		// handle this in SERVER
       if(strcmp(buffer,"QUIT")==0)
        {
			if(send(server_sd,buffer,strlen(buffer),0)<0)
			{
				perror("send");
				exit(-1);
			}
			int bytes = recv(server_sd,buffer,sizeof(buffer),0);
			printf("%s\n", buffer);
        	// printf("closing the connection to server \n");
			if(strcmp(buffer, "221 Service closing control connection.")==0)
			{
				close(server_sd);
            	break;
			}
        }

		// if command is not executed in client because client_task == 1, then send to server
		if (!client_task) {
			if(send(server_sd,buffer,strlen(buffer),0)<0)
			{
				perror("send");
				exit(-1);
			}
			char* token = strtok(buffer, " ");
			printf("%s\n", token);
			int data = 0;
			if(strcmp(token, "RETR") == 0 || strcmp(token, "STOR")==0 || strcmp(token, "LIST")==0){
				data = 1;
			}
			
			printf("%d\n", data);
			if(data) {
				int status;
				int pid = fork(); //fork a child process
				if(pid == 0)   //if it is the child process
				{
					// close(server_sd);
					printf("inside child\n");
					char buffer[256];
					// recv(server_sd,buffer,sizeof(buffer),0);
					// printf("%s\n", buffer);
					// printf("inside child\n");
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

					struct sockaddr_in sa;
					socklen_t sa_len;

					sa_len = sizeof(sa);
					// get client address and port
					if (getsockname(server_sd, (struct sockaddr*)&sa, &sa_len) == -1) {
						perror("getsockname() failed");
						return 1;
					}

					printf("after getsockname\n");
					//store client data exchange address
					struct sockaddr_in cli_data_addr; 
					bzero(&cli_data_addr, sizeof(cli_data_addr)); 
					cli_data_addr.sin_family = AF_INET;
					cli_data_addr.sin_port = htons((int) ntohs(sa.sin_port) + PORT_OFFSET - 1);
					cli_data_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
					printf("%d\n", (int) ntohs(sa.sin_port) + PORT_OFFSET - 1);
					
					//bind
					if(bind(data_sd, (struct sockaddr*)&cli_data_addr,sizeof(cli_data_addr))<0)
					{
						perror("bind failed");
						close(data_sd);
						exit(-1);
					}
					printf("after bind\n");
					//listen
					if(listen(data_sd,5)<0)
					{
						perror("listen failed");
						close(data_sd);
						exit(-1);
					}
					printf("after listen\n");

					// if(send(server_sd,"ok",strlen("ok"),0)<0) //sending the message to server
					// {
					// 	perror("send");
					// 	exit(-1);
					// }

					

					struct sockaddr_in serv_data_addr;
					bzero(&serv_data_addr,sizeof(serv_data_addr));
					serv_data_addr.sin_family = AF_INET;
					serv_data_addr.sin_port = htons(9000);
					serv_data_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); //INADDR_ANY, INADDR_LOOP
					socklen_t len = sizeof(serv_data_addr); 


					// connect data exchange socket to server address for data exchange
					// if (connect(data_sd, (struct sockaddr *)&serv_data_addr, sizeof(serv_data_addr)) < 0)
					// {
					// 	perror("connect");
					// 	return -1;
					// }
					
					printf("before accept\n");
					int transfer_sd = accept(data_sd,(struct sockaddr *)&serv_data_addr,&len);
					printf("after accept\n");
					bzero(buffer,sizeof(buffer));
					recv(transfer_sd,buffer,sizeof(buffer),0);
					printf("%s\n", buffer);
			

					if(send(transfer_sd,"hi from client data",strlen("hi from client data"),0)<0) //sending the message to server
					{
						perror("send");
						exit(-1);
					}

					
					close(transfer_sd);
					close(data_sd);

					exit(0);
				}
				else{
					// close(transfer_sd);
					// close(data_sd);
					wait(&status);

				}
			}
			else{
				bzero(buffer,sizeof(buffer));	
				int bytes = recv(server_sd,buffer,sizeof(buffer),0);
				printf("%s\n", buffer);
			}
			
		}

        bzero(buffer,sizeof(buffer));			
	}

	close(server_sd);
	return 0;
}
