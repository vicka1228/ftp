struct User {
    int state;
    char uname[20];
};
extern struct User session[100];

int check_user_pass(char*, char*);
char* handle_commands(int, char*);