// extern char* CUR_DIR;
extern int PORT_OFFSET;

void displayIntro();
int handle_commands(int, char*, char**);
void handle_retr(int, char*);
void handle_stor(int, char*, char**);
void handle_list(int);
