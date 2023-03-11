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
	char* CUR_DIR = "/";

	//socket
	int server_sd = socket(AF_INET,SOCK_STREAM,0);
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
	server_addr.sin_port = htons(5000);
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

	   int client_task = handle_commands(server_sd, buffer, &CUR_DIR);

       if(strcmp(buffer,"QUIT")==0)
        {
        	printf("closing the connection to server \n");
        	close(server_sd);
            break;
        }

		// if command is not executed in client, then send to server
		if (!client_task) {
			if(send(server_sd,buffer,strlen(buffer),0)<0)
			{
				perror("send");
				exit(-1);
			}
			
			bzero(buffer,sizeof(buffer));	
			int bytes = recv(server_sd,buffer,sizeof(buffer),0);
			printf("%s\n", buffer);
		}

        bzero(buffer,sizeof(buffer));			
	}

	return 0;
}
