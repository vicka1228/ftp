ftp: server.c server_helper.c client.c client_helper.c
	gcc -o c client.c client_helper.c && gcc -o s server.c server_helper.c