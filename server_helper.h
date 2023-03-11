struct User {
    int state;
    char uname[20];
};
extern struct User session[100];
extern char CUR_DIR[100];

int check_user_pass(char*, char*);
void handle_commands(int, char*, char*);