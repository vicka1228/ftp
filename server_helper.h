struct User {
    int state;      // state can be 0, 1, 2: 0-> no session, 1-> user added waiting pass, 2-> authenticated
    char uname[50];         // after adding username, store here
    char host[50];
    int port;
    int server_sd;
    char CUR_DIR[256];
};
extern struct User session[100];        // global array to store all sessions
extern char CUR_DIR[256];

int check_user_pass(char*, char*);
void handle_commands(int, char*, char*);