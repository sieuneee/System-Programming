#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<dirent.h>
#include<sys/stat.h>

#define MAX_LEN 256
#define HASH_SIZE 20
#define MEMORY_SIZE 0xfffff
#define MIN(a,b) (((a)<(b)) ? (a) : (b))

typedef struct NODE_{
	char command[MAX_LEN];
	struct NODE_ *link;
}NODE;
//입력받은 명령어를 linked list 형태로 저장하기 위한 구조체

typedef struct HASH_NODE_{
	int n;
	char type[50];
	char mnemonic[50];
	struct HASH_NODE_ * link;
}Hash_node;
//opcode.txt 파일을 읽어 hash table에 linked list 형태로 저장하기 위한 구조체


//사용 함수
void insert(char *command);
void command_help(char command[]);
void command_dir(char command[]);
void command_quit(char *command);
void command_history(char *command);
void print_memory(int start, int end);
int is_hex(char num[], int type);
void command_dump(char command[]);
void command_edit(char command[]);
void command_fill(char command[]);
void command_reset(char command[]);
void insert_hash(int n, char *mnemonic, char *type);
void read_file(FILE *fp);
void command_opcode_mnemonic(char command[], FILE *fp);
void command_opcodelist(char command[], FILE *fp);
void checkCmd(char *command, FILE *fp);
