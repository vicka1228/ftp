// extern char* CUR_DIR;
extern char CUR_DIR[256];
extern int PORT_OFFSET;

void displayIntro();
int handle_commands(int, char*);
void handle_retr(int, char*);
void handle_stor(int, char*);
void handle_list(int);
