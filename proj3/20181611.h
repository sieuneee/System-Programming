#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<dirent.h>
#include<sys/stat.h>

#define MAX_LEN 256
#define HASH_SIZE 20
#define MEMORY_SIZE 0xfffff
#define LABEL_LEN 32
#define MIN(a,b) (((a)<(b)) ? (a) : (b))

#define n_flag 32
#define i_flag 16
#define x_flag 8
#define b_flag 4
#define p_flag 2
#define e_flag 1

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
Hash_node *table[HASH_SIZE];

typedef struct symtab{
	char label[LABEL_LEN];
	int location;
}SYMTAB;
//SYMTAB을 위한 구조체
SYMTAB symbol_table[MAX_LEN];
SYMTAB symbol_saved[MAX_LEN];

typedef struct info{
	int line;
	int loc;
	char object_code[MAX_LEN];
	int type;
	int nixbpe;
	char opcode[MAX_LEN];
	char label[MAX_LEN];
	char mnemonic[LABEL_LEN];
	char operand[LABEL_LEN];
	char operand2[LABEL_LEN];
	struct info *link;
}INFO;

typedef struct estab_{
	char control_section[MAX_LEN];
	char symbol_name[MAX_LEN];
	int address;
	int length;
	struct estab_ *link;
}ESTAB;
//object file을 읽어 외부 심볼 테이블인 estab에 linked list 형태로 저장하기 위한 구조체

ESTAB estab[3]; //파일 개수가 최대 3개이므로

typedef struct bp{
	int break_point;
	struct bp *link;
	int check;
}BP; 
//breakpoint 저장 및 체크하기 위한 구조체
//linked list 형태

unsigned char MEMORY[MEMORY_SIZE];


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

int register_number(char *reg);
void copy();
void command_assemble_filename(char command[]);
int find_symtab(char *label);
int mnemonic_find_type(char *mnemonic);
void insert_info(int line, int loc, int object_code, int type, char *label, char *mnemonic, char *operand, char *operand2);
int pass1(FILE *fp);
int pass2(FILE *lst, FILE *obj);
void command_symbol(char command[]);
void command_type_filename(char command[]);
int make_object_file(FILE *obj);

void command_progaddr(char command[]);
void command_loader(char command[]);
int loader_pass1(FILE *fp, int type, int file_num);
void insert_estab(char *control_section, char *symbol_name, int address, int length, int file_num);
int loader_pass2(FILE *fp, int type, int file_num);
int find_estab(char symbol[], int type);
void command_bp(char command[]);
void command_bp_clear(char command[]);
void command_bp_addr(char command[]);
void command_run(char command[]);
void print_register();
