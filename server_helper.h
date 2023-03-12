struct User {
    int state;      // state can be 0, 1, 2: 0-> no session, 1-> user added waiting pass, 2-> authenticated
    char uname[50];         // after adding username, store here
    char host[50];
    int port;
};
extern struct User session[100];        // global array to store all sessions
extern char CUR_DIR[100];

int check_user_pass(char*, char*);
void handle_commands(int, char*, char*);