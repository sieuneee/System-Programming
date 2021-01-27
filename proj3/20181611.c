#include "20181611.h"

NODE *head=NULL, *temp=NULL; //history 저장할 떄 사용할 linked list 포인터
INFO *head_asm=NULL, *tail_asm=NULL; //asm파일을 읽고 linked list 형태로 내용을 저장하기 위한 포인터

int last_address=-1; //메모리 출력 시 이전 주소가 저장된 변수
int quit=0; //q[uit] command가 입력되었을 때 값이 1로 바뀐다.
int sym_idx=0;
int sym_idx_saved=0;
int LOCCTR=0;
char program_name[MAX_LEN];
//int RETADR=0;
char base_register[MAX_LEN];
int base_loc=0;
int start_loc=0;
int end_loc=0;

int PROGADDR=0;
int CSADDR=0;
int CSLTH=0;
int EXECADDR=0;

int total_len=0;

int run_start=0;
int run_end=0;
int run=0;
int load_file=0;

int R[10];
//R[0]:A, R[1]:X, R[2]:L, R[3]:B, R[4]:S, R[5]:T, R[6]:F, R[8]:PC, R[9]:SW

char R_record[MAX_LEN][MAX_LEN];

BP *head_bp=NULL, *tail_bp=NULL;

ESTAB *es_tail[3];

//linked list 형태로 명령어를 history에 저장하는 함수이다.
//각 명령어가 옳은 입력일 때에만 해당 함수에서 호출된다.
void insert(char *command){
	NODE *new;
	new = (NODE*)malloc(sizeof(NODE));
	new->link = NULL;
	strcpy(new->command, command);

	if(!head) {
		head=new;
		temp=new;
	}
	else{
		temp->link = new;
		temp = new;
	}
}

//h[elp] 명령어가 입력된 경우에 가능한 명령어를 출력하는 함수이다.
void command_help(char command[]){
	printf("h[elp]\nd[ir]\nq[uit]\nhi[story]\ndu[mp] [start, end]\ne[dit] address, value\nf[ill] start, end, value\nreset\nopcode mnemonic\nopcodelist\nasssemble filename\ntype filename\nsymbol\nprogaddr [address]\nloader [object filename1] [object filename2] [...]\nbp [address]\nbp clear\nbp\nrun\n");
	insert(command);
}

//dir 명령어가 입력된 경우에 디렉터리에 있는 파일들을 출력하는 함수이다.
void command_dir(char command[]){
	DIR *dir_info=opendir(".");
	struct dirent *dir_entry=NULL;
	struct stat buf;

	if(dir_info==NULL){
		printf("Error\n");
		return;
	}
	while((dir_entry=readdir(dir_info))!=NULL){
		stat(dir_entry->d_name, &buf);
		if(S_ISDIR(buf.st_mode)) printf("%s/\t", dir_entry->d_name); //디렉터리는 /표시와 함께 출력한다.
		else if((buf.st_mode & S_IXUSR) || (buf.st_mode & S_IXGRP) || (buf.st_mode & S_IXOTH))
			printf("%s*\t", dir_entry->d_name); //실행파일은 파일 이름 옆에 * 표시를 한다.
		else printf("%s\t", dir_entry->d_name);
	}
	printf("\n");
	closedir(dir_info);
	insert(command); //옳은 입력이 들어와서 함수를 수행한 경우에만 명령어가 history에 insert된다.
}

//q[uit] 명령어를 입력했을 때 호출되는 함수이다.
//동적할당한 부분을 free 해주고, 전역변수 quit를 1로 바꿔준다.
void command_quit(char *command){
	NODE *ptr;
	Hash_node *ptr2;
	ESTAB *ptr3, *tmp;
	BP *ptr4;

	while(head!=NULL){
		ptr=head;
		head = head->link;
		free(ptr);
		ptr=NULL;
	}

	for(int i=0;i<HASH_SIZE;i++){
		ptr2 = table[i];
		while(table[i]!=NULL){
			ptr2 = table[i];
			table[i] = table[i]->link;
			free(ptr2);
			ptr2=NULL;
		}
	}

	for(int i=0;i<3;i++){
		tmp=estab[i].link;
		while(tmp!=NULL){
			ptr3 = tmp;
			tmp = tmp->link;
			free(ptr3);
			ptr3=NULL;
		}
	}

	while(head_bp!=NULL){
		ptr4 = head_bp;
		head_bp = head_bp->link;
		free(ptr4);
		ptr4 = NULL;
	}


	quit=1;
	insert(command);
}

//linked list에 저장된 history를 출력해주는 함수이다.
//hi[story] 명령어가 입력되었을 때 호출된다.
void command_history(char *command){
	NODE *tmp;
	int num=1;
	tmp=head;

	insert(command);

	for(tmp=head;tmp;tmp=tmp->link){
		printf("%-3d  %s\n", num, tmp->command);
		num++;
	}

}

//메모리를 출력하는 함수이다.
void print_memory(int start, int end){
	int h_start, h_end;
	int i, j;

	h_start = start/16*16;
	h_end = end/16*16;

	for(i=h_start ; i<=h_end;i+=16){
		printf("%05X ", i);
		for(j=0;j<16;j++){
			if(start <=i+j && i+j <=end) printf("%02X ", MEMORY[i+j]);
			else printf("   ");
		}
		printf("; ");

		for(j=0;j<16;j++){
			if(i+j >=start && i+j<=end){
				if(MEMORY[i+j] >=0x20 && MEMORY[i+j] <=0x7E){
					printf("%c", MEMORY[i+j]);
				} //범위안에 있는 값인 경우 그대로 출력해주고, 범위를 벗어나는경우 .으로 출력한다.
				else printf(".");
			}
			else printf(".");
		}
		printf("\n");
	}

}

//dump, edit, fill 명령어 처리 시, 입력받은 값이 16진수인지 판별하는 함수이다.
//type==1이면 뒤에 쉼표가 붙은 값(뒤에 다른 값이 입력된 경우)이고, type==0은 그렇지 않은 경우이다.
int is_hex(char num[], int type){
	if(type==1){
		if(num[strlen(num)-1]==',') num[strlen(num)-1]='\0';
		else return -1;
	}
	for(int i=0;i<strlen(num); i++){
		if(!((num[i]>=48&&num[i]<=57)||(num[i]>=65&&num[i]<=70)))
			return -1;
	} //값이 0~9 또는 A~F 인지 확인한다.
	return 1;
}

//du[mp] 명령어 입력 시 수행하는 함수이다.
//할당된 메모리의 내용을 print_memory 함수를 호출하여 형식에 맞게 출력해준다.
void command_dump(char command[]){
	int start=0, end=0;
	int max_size = MEMORY_SIZE;
	char input_command[MAX_LEN];
	char *input_start, *input_end;
	int du_case=0; //dump 뒤에 start값과 end값이 있는지 구분해주는 변수이다.

	start = (last_address+1)%(max_size+1);
	end = MIN(start+159, max_size);

	strcpy(input_command, command);
	command = strtok(command, " ");
	input_start = strtok(NULL, " ");
	if(input_start==NULL) du_case=0;
	else {
		input_end = strtok(NULL, " ");
		if(input_end==NULL) 
			du_case=1;
		else du_case=2;
	}
	if(du_case == 1){ //start값만 주어진 경우
		if(is_hex(input_start, 0)==-1) {
			printf("ERROR\n"); return; //16진수가 아닌경우 에러메시지를 출력하고 함수를 return한다.
		}
		start = strtol(input_start, NULL, 16);
		end = MIN(start+159, max_size);
	}
	if(du_case == 2){ //start, end값 모두 주어진 경우
		if(is_hex(input_start, 1)==-1 || is_hex(input_end, 0)==-1){
			printf("ERROR\n"); return;
		}
		start = strtol(input_start, NULL, 16);
		end = strtol(input_end, NULL, 16);
	}

	if(start>MEMORY_SIZE || end<0 || start>end) {printf("ERROR\n"); return;}
	else print_memory(start, end);
	last_address = end; //마지막 주소값을 last_address 변수(전역변수)에 저장한다.

	insert(input_command); //입력이 옳은 형식일 경우에만 linked list에 추가한다.
}

//e[dit] 명령어를 입력받았을 때 수행되는 함수이다.
//메모리의 address 번지의 값을 value에 지정된 값으로 변경한다.
void command_edit(char command[]){
	char input_command[MAX_LEN];
	char *address, *value;
	int	add_int, val_int;

	strcpy(input_command, command);
	command = strtok(command, " ");
	address = strtok(NULL, " ");
	value = strtok(NULL, " ");

	if(address==NULL || value==NULL){
		printf("ERROR\n");
		return;
	}

	add_int = strtol(address, NULL, 16);
	val_int = strtol(value, NULL, 16);

	if(is_hex(address, 1)==-1 || is_hex(value, 0)==-1){
		printf("ERROR\n");
		return;
	}
	if(!(add_int >=0 && add_int<=MEMORY_SIZE)){
		printf("ERROR\n");
		return;
	}
	if(val_int<0 || val_int>255){
		printf("ERROR\n");
		return;
	}

	//주소값이 메모리 범위를 벗어나는 경우, value 값이 범위를 벗어나는 경우 에러메시지를 출력하고 함수를 return한다.

	MEMORY[add_int] = (char)val_int;

	insert(input_command); //옳은 명령일 경우 해당 주소를 입력받은 값으로 변경해주고 명령어를 linked list에 저장한다.

}

//fill 명령어를 입력 받았을 때 수행되는 함수이다.
//메모리의 start 번지부터 end 번지까지의 값을 value에 지정된 값으로 변경해준다.
void command_fill(char command[]){
	char input_command[MAX_LEN]; 
	char *start, *end, *value;
	int start_int, end_int, value_int;

	strcpy(input_command, command);
	command = strtok(command, " ");
	start = strtok(NULL, " ");
	end = strtok(NULL, " ");
	value = strtok(NULL, " ");

	if(start==NULL || end==NULL || value==NULL) {
		printf("ERROR\n");
		return;
	}

	start_int = strtol(start, NULL, 16);
	end_int = strtol(end, NULL, 16);
	value_int = strtol(value, NULL, 16);

	if(is_hex(start, 1)==-1 || is_hex(end, 1)==-1 || is_hex(value, 0)==-1){
		printf("ERROR\n");
		return;
	}
	if(start_int>MEMORY_SIZE || end_int>MEMORY_SIZE || end_int<0 || start_int>end_int){
		printf("ERROR\n");
		return;
	}
	if(value_int<0 || value_int>255){
		printf("ERROR\n");
		return;
	}

	//주소값이 메모리 범위를 벗어나는 경우, value 값이 범위를 벗어나는 경우 에러메시지를 출력하고 함수를 return한다.

	for(int i=start_int;i<=end_int;i++) MEMORY[i] = (char)value_int;

	insert(input_command);

}

//reset 명령어를 입력받았을 때 수행하는 함수이다.
//메모리 전체를 0으로 변경한다.
void command_reset(char command[]){
	for(int i=0;i<MEMORY_SIZE;i++)
		MEMORY[i] = 0;

	insert(command);
}

//메모리를 초기화하는 함수이다. 파일을 처음 실행할 때 호출된다.
void init(){
	for(int i=0;i<HASH_SIZE;i++)
		table[i] = NULL;
	for(int i=0;i<MEMORY_SIZE;i++)
		MEMORY[i]=0;
}

//mnemonic을 hash table에 저장하는 hash function이다.
//값을 linked list 형태로 저장한다.
void insert_hash(int n, char *mnemonic, char *type){
	Hash_node *new;
	Hash_node *temp;
	int key=0;

	new = (Hash_node *)malloc(sizeof(Hash_node));
	new->n = n;
	strcpy(new->mnemonic, mnemonic);
	strcpy(new->type, type);
	new->link = NULL;

	for(int i=0;i<strlen(mnemonic);i++) key += mnemonic[i];

	key = key%HASH_SIZE;

	//mnemonic의 값을 모두 더해서 HASH_SIZE (=20) 으로 나누어서 key값을 정해주었다.

	temp = table[key];
	if(temp!= NULL) new->link = temp;
	table[key] = new;

}

//opcode.txt 파일을 읽는 함수이다.
void read_file(FILE *fp){
	char cmd[MAX_LEN];
	char *input_mnemonic, *input_type, *input_n;
	int n;

	for(int i=0;i<HASH_SIZE;i++) table[i]=NULL;

	while(fgets(cmd, MAX_LEN, fp)!=NULL){
		input_n = strtok(cmd, " \t\n");
		input_mnemonic = strtok(NULL, " \t\n");
		input_type = strtok(NULL, " \t\n");

		n = strtol(input_n, NULL, 16);
		insert_hash(n, input_mnemonic, input_type); //읽어낸 값을 저장하기위해 insert_hash 함수를 호출한다.
	}
}

//opcode mnemonic 명령어가 입력되었을 때 수행되는 함수이다.
//hash table에서 해당 opcode를 찾아 출력한다. 없을 경우 ERROR 메시지를 출력하고 return 한다.
void command_opcode_mnemonic(char command[], FILE* fp){
	char input_command[MAX_LEN];
	char *mnemonic;
	int ret=0;
	strcpy(input_command, command);

	command = strtok(command, " ");
	mnemonic = strtok(NULL, " ");
	if(mnemonic==NULL) {printf("ERROR\n"); return;}

	Hash_node *temp;
	for(int i=0;i<HASH_SIZE;i++){
		for(temp=table[i];temp!=NULL;temp=temp->link){
			if(!strcmp(temp->mnemonic,mnemonic)){
				printf("opcode is %X\n", temp->n);
				ret=1;
			}
		}
	}
	if(ret==1) insert(input_command);
	else printf("Not exist\n");

}

//opcodelist 명령어를 입력했을 때 opcode hash table 내용을 출력해주는 함수이다.
void command_opcodelist(char command[], FILE *fp){
	Hash_node *temp;
	for(int i=0;i<HASH_SIZE;i++){
		printf("%d : ", i);
		for(temp=table[i];temp!=NULL;temp=temp->link){
			printf("[%s,%X]  ", temp->mnemonic, temp->n);
			if(temp->link!=NULL) printf("-> ");
		}
		printf("\n");
	}
	insert(command);		
}

//입력된 명령어를 실행 가능한 명령어와 비교해서 해당 함수를 호출해주는 함수이다.
void checkCmd(char *command, FILE *fp){

	if(!strcmp(command, "h") || !strcmp(command, "help")) command_help(command);
	else if(!strcmp(command, "d") || !strcmp(command, "dir")) command_dir(command);
	else if(!strcmp(command, "q") || !strcmp(command, "quit")) command_quit(command);
	else if(!strcmp(command, "hi") || !strcmp(command, "history")) command_history(command);
	else if(!strncmp(command, "dump", 4) || !strncmp(command, "du", 2)) command_dump(command);
	else if(!strncmp(command, "edit", 4) || !strncmp(command, "e", 1)) command_edit(command);
	else if(!strncmp(command, "fill", 4) || !strncmp(command, "f", 1)) command_fill(command);
	else if(!strcmp(command, "reset")) command_reset(command);
	else if(!strcmp(command, "opcodelist")) command_opcodelist(command, fp);
	else if(!strncmp(command, "opcode", 6)) command_opcode_mnemonic(command, fp);
	else if(!strncmp(command, "type", 4)) command_type_filename(command);
	else if(!strncmp(command, "assemble", 8)) command_assemble_filename(command);
	else if(!strcmp(command, "symbol")) command_symbol(command);
	else if(!strncmp(command, "progaddr", 8)) command_progaddr(command);
	else if(!strncmp(command, "loader", 6)) command_loader(command);
	else if(!strcmp(command, "bp")) command_bp(command);
	else if(!strcmp(command, "bp clear")) command_bp_clear(command);
	else if(!strncmp(command, "bp", 2)) command_bp_addr(command);
	else if(!strcmp(command, "run")) command_run(command);
	else{
		printf("ERROR\n");
		return;
	}
}

int main(){
	char arr[MAX_LEN];
	int i;
	init(); //초기화

	FILE *fp = fopen("opcode.txt", "r");
	read_file(fp);

	while(1){
		printf("sicsim> ");

		fgets(arr, sizeof(arr), stdin);
		arr[strlen(arr)-1]='\0';
		for(i=strlen(arr)-1;i>0;i--){
			if(arr[i]==' ') arr[i]='\0';
			else break;
		} //명령어 뒤에 공백이 추가된 경우 제거한다.
		if(strlen(arr)==0) continue;
		checkCmd(arr, fp);
		if(quit==1) break; //q[uit] 명령어가 입력되었을 떄, quit 변수를 1로 바꾸어 while문을 빠져나온다.

	}

	fclose(fp);
	return 0;
}




//assembler - PROJECT 2


//assemble 명령을 수행할 때, object code를 생성하기 위해 해당 레지스터 번호를 알려주는 함수
//함수의 인자로 전달받은 문자열 reg와 SIC/XE 머신의 9개의 레지스터(A, X, L, B, S, T, F, PC, SW)를 비교하여 해당 레지스터의 번호를 출력해준다. 만약 9개 레지스터 중 일치하는 레지스터가 없으면 -1을 리턴해준다.
int register_number(char *reg){
	if(!strcmp(reg, "A")) return 0;
	else if(!strcmp(reg, "X")) return 1;
	else if(!strcmp(reg, "L")) return 2;
	else if(!strcmp(reg, "B")) return 3;
	else if(!strcmp(reg, "S")) return 4;
	else if(!strcmp(reg, "T")) return 5;
	else if(!strcmp(reg, "F")) return 6;
	else if(!strcmp(reg, "PC")) return 8;
	else if(!strcmp(reg, "SW")) return 9;
	else return -1;
}

//symbol 명령을 수행할 때 가장 최근에 assemble 된 파일에 대한 symbol을 출력해야하므로
//assemble에 성공했을 때 symbol_table에 저장된 값을 symbol_table_saved에 복사해준다.
void copy(){
	int i;
	for(i=0;i<sym_idx;i++){
		strcpy(symbol_saved[i].label, symbol_table[i].label);
		symbol_saved[i].location = symbol_table[i].location;
	}
}


//assemble filename 명령어가 입력된 경우 호출되어 수행되는 함수
//filename이 디렉터리에 있는 파일인지 확인하여 파일이 존재하면 pass1 함수와 pass2 함수를 차례로 호출하고 파일이 존재하지 않으면 파일이 없단는 메시지를 출력한 뒤 리턴한다.
void command_assemble_filename(char command[]){
	char *filename;
	char input_command[MAX_LEN];
	char obj_filename[MAX_LEN], lst_filename[MAX_LEN];
	FILE *fp, *lst, *obj;

	strcpy(input_command, command);
	filename = strtok(command, " ");
	filename = strtok(NULL, " ");
	fp = fopen(filename, "r");
	if(!fp){
		printf("ERROR - invaild file\n");
		return;
	}
	//파일이 존재하지 않는 경우 
	filename[strlen(filename)-4]='\0';
	strcpy(obj_filename, filename);
	strcpy(lst_filename, filename);
	strcat(obj_filename, ".obj");
	strcat(lst_filename, ".lst");
	//filename에 .obj, .lst를 붙여서 object파일과 리스팅파일 이름 생성 

	if(pass1(fp)==-1){
		printf("(: PASS1 ERROR)\n");
		return;
	}
	fclose(fp);
	//pass1
	lst = fopen(lst_filename, "w+");
	obj = fopen(obj_filename, "w+");
	if(pass2(obj, lst)==-1){
		printf("(: PASS2 ERROR)\n");
		remove(lst_filename);
		remove(obj_filename);
		return;
	}
	else{
		fclose(lst);
		fclose(obj);
		sym_idx_saved = sym_idx;
		copy();
		printf("Successfully assemble %s\n", filename);
	}
	//pass2

	head_asm=NULL;
	tail_asm=NULL;
	insert(input_command);
	//assemble에 성공했을때만 command를 history에 추가
}

//symbol_table에서 해딩 label의 location을 찾아 리턴하는 함수
//label이 symbol_table에 존재하지 않으면 -1을 리턴해준다.
int find_symtab(char *label){
	int i;
	for(i=0;i<sym_idx;i++){
		if(!strcmp(label, symbol_table[i].label))
			return symbol_table[i].location;
	}
	return -1;
}

//mnemonic의 type을 찾아 리턴하는 함수
//opcode.txt 파일을 저장한 hash table에서 찾아 결과값을 리턴해준다.
//만약 해당 mnemonic에 대한 정보가 없으면 -1을 리턴해준다.
int mnemonic_find_type(char *mnemonic){
	Hash_node *temp;
	int type=-1, i;

	for(i=0;i<HASH_SIZE;i++){
		for(temp=table[i];temp!=NULL;temp=temp->link){
			if(!strcmp(temp->mnemonic, mnemonic)){
				type = atoi(temp->type);
			}
		}
	}
	return type;
}

//assemble 명령을 수행했을 때 파일을 읽어 데이터에 linked list 형태로 저장하기 위한 함
void insert_info(int line, int loc, int object_code, int type, char *label, char *mnemonic, char *operand, char *operand2){
	INFO *new_info;
	Hash_node *temp;
	int i;


	new_info = (INFO*)malloc(sizeof(INFO));
	new_info->line = line;
	new_info->loc = loc;

	new_info->type = type;
	strcpy(new_info->object_code, "-");
	strcpy(new_info->label, label);
	strcpy(new_info->mnemonic, mnemonic);
	strcpy(new_info->operand, operand);
	strcpy(new_info->operand2, operand2);
	if(type==1 || type==2 || type==3 || type==4){
		for(i=0;i<HASH_SIZE;i++){
			for(temp=table[i];temp!=NULL;temp=temp->link){
				if(!strcmp(temp->mnemonic, mnemonic)){
					sprintf(new_info->opcode, "%X", temp->n); 
					//opcode table에서 mnemonic에 해당하는 opcode를 찾아서 저장
				}
			}
		}
	}
	else new_info->type=-1;
	new_info->link=NULL;

	if(!head_asm){
		head_asm=new_info;
		tail_asm=new_info;
	}
	else{
		tail_asm->link = new_info;
		tail_asm = new_info;
	}
	//INFO *head, INFO *tail을 사용하여 linked list 형태로 저장

}


//assembler 의 pass1을 수행하기 위한 함수
//asm 파일을 읽어서, symbol_table을 만들어준다.
//또한 line number와 location 값을 결정해서 insert_info 함수를 호출하여 inked list 형태로 데이터를 저장한다.
//중간에 에러가 발생하면 에러가 발생한 line과 에러 메시지를 출력하고 리턴한다
int pass1(FILE *fp){
	char buffer[MAX_LEN];
	char *label;
	char *mnemonic;
	char *operand, *operand2;
	int type=0;
	int line_num=5;
	int len, j;
	int label_exist=0;
	int next_loc=0;
	sym_idx=0;

	for(int i=0;i<MAX_LEN;i++)
		symbol_table[i].label[0] = '\0';

	strcpy(program_name, "-");

	fgets(buffer, MAX_LEN, fp);
	while(buffer[0]=='.') {
		fgets(buffer, MAX_LEN, fp);
	} //파일 앞부분에 주석이 나오는 경우 제외
	label = strtok(buffer, " ");
	mnemonic = strtok(NULL, " ");
	operand = strtok(NULL, " ");
	operand[strlen(operand)-1]='\0';
	if(!strcmp(mnemonic, "START")){
		if(label!=NULL) strcpy(program_name, label); //파일 이름 저장
	}
	else {
		printf("ERROR - not exist \'START\' label");
		//주석 제외하고 파일 첫부분에 START 없으면 에러
		return -1;
	}

	LOCCTR = strtol(operand, NULL, 16); //주소 배정을 위한 포인터에 주소 저장
	start_loc = LOCCTR; //시작주소 저장

	insert_info(line_num, LOCCTR, 0, -1, label, mnemonic, operand, "-");//linked list에 추가
	line_num+=5;
	while(fgets(buffer, MAX_LEN, fp)!=NULL){
		if(buffer[0]=='.') {
			buffer[strlen(buffer)-1]='\0';
			insert_info(line_num, 0, 0, -1, buffer, "-", "-", "-");
			line_num+=5; continue;
		}//주석인경우
		label = NULL;
		mnemonic = NULL;
		operand = NULL;
		operand2 = NULL;
		label_exist=0;
		next_loc=0;
		type=0;

		if(buffer[0]==' ' || buffer[0] == '\t'){
			mnemonic = strtok(buffer, " \t");
			operand = strtok(NULL, " \t");
			operand2 = strtok(NULL, " \t");
			if(operand!=NULL) operand[strlen(operand)-1] = '\0';
			if(operand==NULL) mnemonic[strlen(mnemonic)-1] = '\0';
			if(operand2!=NULL) operand2[strlen(operand2)-1] = '\0';
		}
		else{
			label_exist=1;
			label = strtok(buffer, " \t");
			mnemonic = strtok(NULL, " \t");
			operand = strtok(NULL, " \t");
			operand2 = strtok(NULL, " \t");
			if(operand!=NULL) operand[strlen(operand)-1] = '\0';
			if(operand==NULL) mnemonic[strlen(mnemonic)-1] = '\0';
			if(operand2!=NULL) operand2[strlen(operand2)-1] = '\0';
			if(find_symtab(label)==-1){
				strcpy(symbol_table[sym_idx].label, label);
				symbol_table[sym_idx].location = LOCCTR;
				sym_idx++;
			}
			else{
				printf("%d line ERROR - duplicate symbol", line_num);
				//symbol table에 이미 해당 label이 있는 경우 에러
				return -1;
			}
		}

		if(!strcmp(mnemonic, "END")) {
			end_loc = LOCCTR;
			insert_info(line_num, LOCCTR, 0, -1, "-", mnemonic, operand, "-");
			break;
		}//END 나오면 종료

		if(mnemonic[0]=='+') {
			len = strlen(mnemonic);
			for(j=0;j<len;j++) mnemonic[j] = mnemonic[j+1];
			mnemonic[j] = '\0';
			type=mnemonic_find_type(mnemonic);
			//'+' 뒤에 나오는 mnemonic이 opcode table에 존재하는지 확인
			if(type==-1){
				printf("%d line ERROR - invalid operation code", line_num); return -1;
			}
			else type=4;
		}
		else {
			type = mnemonic_find_type(mnemonic); //mnemonic의 type check
		}

		if(type!=-1){
			next_loc=type; //다음 주소 정할때 type만큼 증가시킴
		}
		else{
			if(!strcmp(mnemonic, "WORD")) next_loc=3;
			else if(!strcmp(mnemonic, "RESW")) next_loc = 3*(atoi(operand));
			else if(!strcmp(mnemonic, "RESB")) next_loc = atoi(operand);
			else if(!strcmp(mnemonic, "BASE")) {
				strcpy(base_register, operand); //base register의 operand 저장
				insert_info(line_num, 0, 0, type, "-", mnemonic, operand, "-"); //linked list에 저장
				line_num+=5;
				continue;
			}
			else if(!strcmp(mnemonic, "NOBASE")) next_loc=0;
			else if(!strcmp(mnemonic, "BYTE")){
				if(operand[0]=='C'){
					len = strlen(operand)-3;
					next_loc = len;
				}
				else if(operand[0]=='X') next_loc = 1;
			}
			else{
				printf("%d line ERROR - invalid operation code", line_num);
				return -1;
			}//mnemonic이 opcode table에도 없고, WORD, RESW, RESB, BASE, NOBASE, BYTE도 아닌경우 에러
		}


		if(label_exist==1){
			if(operand!=NULL){
				if(operand2!=NULL) insert_info(line_num, LOCCTR, 0, type, label, mnemonic, operand, operand2);
				else insert_info(line_num, LOCCTR, 0, type, label, mnemonic, operand, "-");
			}
			else insert_info(line_num, LOCCTR, 0, type, label, mnemonic, "-", "-");
		}
		else{
			if(operand!=NULL){
				if(operand2!=NULL) insert_info(line_num, LOCCTR, 0, type, "-", mnemonic, operand, operand2);
				else insert_info(line_num, LOCCTR, 0, type, "-", mnemonic, operand, "-");
			}
			else insert_info(line_num, LOCCTR, 0, type, "-", mnemonic, "-", "-");
		}
		//linked list에 저장

		LOCCTR += next_loc; //LOCCTR 증가시킴
		line_num += 5; //line number 5씩 증가시킴

	}
	return 1;
}


//assembler의 pass2를 수행하기 위한 함수
//linked list 형태로 저장된 데이터를 읽어 object code를 생성하고 저장헤준다.
//중간에 에러가 발생하면 에러가 발생한 Line과 에러 메시지를 출력하고 리턴한다.
int pass2(FILE *obj, FILE *lst){
	INFO *tmp;
	tmp = head_asm;
	int x=0, y=0;
	int ta, pc;
	char str1[MAX_LEN];
	int i, k, s;
	int yy, int_obcode;
	int byte_num=0;
	char byte_str[MAX_LEN];

	base_loc = find_symtab(base_register);


	for(tmp=head_asm;tmp;tmp=tmp->link){ //linked list 끝까지

		x=y=ta=pc=0; //초기화
		tmp->nixbpe=0; //초기화

		if(tmp->type==1 || tmp->type==2 || tmp->type==3 || tmp->type==4){

			if(strlen(tmp->opcode)==1){
				tmp->opcode[1] = tmp->opcode[0];
				tmp->opcode[0] = '0';
			}
			for(i=0;i<2;i++){
				if(tmp->opcode[i] >= '0' && tmp->opcode[i] <= '9')
					x = (x|(tmp->opcode[i]-'0'));
				else x = (x|(tmp->opcode[i] - 55));

				if(tmp->type==4){
					if(i==0) x = x<<4;
					else x = x<<24;
				}
				else{
					if(i==0) x = x<<4;
					else x = x<<8*((tmp->type)-'0'-1);
				}
			}
			//비트연산을 통해 object code에 opcode 저장
			y=x;
			x=0;

			if((tmp->type)==3 || (tmp->type)==4){
				if(strcmp(tmp->operand, "-")){
					if(tmp->operand[0]=='#')
						tmp->nixbpe |= i_flag; //immediate addresssing
					else if(tmp->operand[0]=='@')
						tmp->nixbpe |= n_flag; //indirect addressing
					else{
						tmp->nixbpe |= n_flag;
						tmp->nixbpe |= i_flag; //simple addressing
					}

					if(strcmp(tmp->operand2, "-")){
						tmp->nixbpe |= x_flag;
					}


					if(tmp->type==4) {
						tmp->nixbpe |= e_flag;
					}
					else{
						if(tmp->operand[0]=='@'){
							strcpy(str1, tmp->operand);
							for(k=0;k<strlen(str1);k++) str1[k]=str1[k+1];
							str1[k]='\0';
						}

						else if(tmp->operand[0]=='#'){
							strcpy(str1, tmp->operand);
							for(k=0;k<strlen(str1);k++) str1[k]=str1[k+1];
							str1[k]='\0';
						}

						else {
							strcpy(str1, tmp->operand);
						}
						ta = find_symtab(str1); //operand가 symbol table에 있는지 확인
						if(ta == -1){
							if(tmp->operand[0]!='#'){
								printf("%d line ERROR - undefined symbol", tmp->line);
								return -1; //없으면 에러
							}
						}
						pc = tmp->loc + tmp->type;
						if(ta!=-1 || tmp->operand[0]!='#'){
							if(ta-pc>=-2048 && ta-pc <=2047) {
								tmp->nixbpe |= p_flag; //pc relative
							}
							else tmp->nixbpe |= b_flag;//base relative
						}
					}

					x = tmp->nixbpe;
					if(tmp->type==4) { //4형식인경우
						x = x<<20;
					}
					else x = x<<12;
					y|=x;

					int aa;
					if(tmp->operand[0]=='#'){ //immediate addressing
						strcpy(str1, tmp->operand);
						for(k=0;k<strlen(str1);k++) str1[k] = str1[k+1];
						str1[k]='\0';
						if(find_symtab(str1) != -1) {
							aa = find_symtab(str1);
							aa = aa-(tmp->loc+tmp->type);
							y |= aa;
						}
						else{
							y |= atoi(str1);
						}
					}
					else{
						if(tmp->type==4 && strcmp(tmp->operand, "-")){
							aa = find_symtab(tmp->operand);
							if(aa!=-1)
								y |= aa;
						}
					}
					int bb=0;
					if(tmp->nixbpe & p_flag){
						if(ta>=pc) y |= ta-pc;
						else y |= ((ta-pc)&0x00000FFF); //음수인경우
					}
					else if(tmp->nixbpe & b_flag){
						bb = find_symtab(tmp->operand);
						if(bb>=base_loc) y|= bb-base_loc;
						else y |= ((bb-base_loc)&0x00000FFF); //음수인경우
					}
				}
				else{
					if(!strcmp(tmp->mnemonic, "RSUB")){ //mnemonic이 "RSUB"이면
						tmp->nixbpe |= n_flag;
						tmp->nixbpe |= i_flag;
						x = tmp->nixbpe << 12;
						y |= x;
					}
				}
			}
			else if(tmp->type==2){ //2형식인경우
				int reg_num;
				if(strcmp(tmp->operand, "-")){
					reg_num = register_number(tmp->operand);
					x |= reg_num;
					x = x<<4;
				}

				if(strcmp(tmp->operand2, "-")){
					reg_num = register_number(tmp->operand2);
					x |= reg_num;
				}
				y|=x;

			}

			yy=0;
			int_obcode=y;
			if(tmp->type==4) yy=7;
			else yy = 2*(tmp->type)-1;
			for(;yy>=0;yy--){
				if((int_obcode&15)>=10) tmp->object_code[yy] = ((int_obcode&15)+55);
				else if((int_obcode&15)<=9 && (int_obcode&15)>=0) tmp->object_code[yy] = ((int_obcode&15)+'0');
				int_obcode = int_obcode>>4;
			}
		}
		else{
			if(!strcmp(tmp->mnemonic, "BYTE")){
				if(tmp->operand[0]=='C'){
					byte_num=0;
					tmp->object_code[0] = '\0';
					for(k=2;k<strlen(tmp->operand)-1;k++){
						byte_num = tmp->operand[k];
						sprintf(byte_str, "%02X", byte_num);
						strcat(tmp->object_code, byte_str);
					}

				}
				else if(tmp->operand[0]=='X'){
					for(k=2, s=0; ; k++, s++){
						if(tmp->operand[k]=='\'') break;
						tmp->object_code[s] = tmp->operand[k];
					}
				}
			}
			else if(!strcmp(tmp->mnemonic, "WORD")){
				sprintf(tmp->object_code, "%06X", atoi(tmp->operand));
				//strcpy(tmp->object_code, "00000");
			}
		}



		//리스팅 파일에 쓰기
		fprintf(lst, "%3d\t ", tmp->line);
		if((!strcmp(tmp->mnemonic, "BASE")) || tmp->label[0]=='.' || (!strcmp(tmp->mnemonic, "END"))) fprintf(lst, "\t ");
		else fprintf(lst, "%04X\t ", tmp->loc);

		if(tmp->label[0]=='.') fprintf(lst, "%s", tmp->label);
		else if(strcmp(tmp->label, "-")) fprintf(lst, "%s\t ", tmp->label);
		else fprintf(lst, "\t ");
		fprintf(lst, "\t");
		if(tmp->label[0]!='.') {
			if(tmp->type==4) fprintf(lst, "+");
			fprintf(lst, "%s\t ", tmp->mnemonic);
		}
		if(strcmp(tmp->operand, "-")) {
			if(strcmp(tmp->operand2, "-")) fprintf(lst, "%s, %s\t\t ", tmp->operand, tmp->operand2);
			else
				fprintf(lst, "%s\t\t ", tmp->operand);
		}
		else fprintf(lst, "\t\t ");
		int len = strlen(tmp->operand)+strlen(tmp->operand2);
		if(strcmp(tmp->operand2, "-")) len+=2;
		if(len<8)
			fprintf(lst, "\t ");
		if(strcmp(tmp->object_code, "-")) fprintf(lst, "%s\n", tmp->object_code);
		else fprintf(lst, "\n");


	}
	if(make_object_file(obj)==-1) {printf("%d line ERROR ", tmp->line); return -1;}
	//오브젝트 파일 생성(에러 발생 시 리턴)
	return 1;
}

//symbol 명령을 수행하기 위한 함수
//구조체 배열 symbol_saved에 저장되어있는 데이터를 symbol을 기준으로 내림차순 정렬 하여 출력해준다.
//symbol을 출력한 후, command를 history에 추가해준다.
void command_symbol(char command[]){
	int i=0, j;
	SYMTAB temp;

	for(i=0;i<sym_idx_saved-1;i++){
		for(j=0;j<sym_idx_saved-i-1;j++){
			if(strcmp(symbol_saved[j].label, symbol_saved[j+1].label)>0){
				strcpy(temp.label, symbol_saved[j].label);
				temp.location = symbol_saved[j].location;
				strcpy(symbol_saved[j].label, symbol_saved[j+1].label);
				symbol_saved[j].location = symbol_saved[j+1].location;
				strcpy(symbol_saved[j+1].label, temp.label);
				symbol_saved[j+1].location = temp.location;
			}
		}
	}	//bubble sort를 이용하여 symbol 기준으로 내림차순 정렬


	for(i=0;i<sym_idx_saved;i++){
		printf("\t %s\t  %04X\n", symbol_saved[i].label, symbol_saved[i].location);
	}
	//출력

	insert(command); //history에 추가
}


//filename에 해당하는 파일을 읽어 화면에 출력해주는 함수
//디렉터리에 파일이 존재하지 않으면 에러 메시지를 출력하고 리턴한다.
//파일이 존재하여 정상적으로 출력한 경우에만 command를 history에 출력한다.
void command_type_filename(char command[]){
	char *filename;
	char input_command[MAX_LEN];
	FILE *pfile;
	char str[MAX_LEN];

	strcpy(input_command, command);
	command = strtok(command, " ");
	filename = strtok(NULL, " ");

	pfile = fopen(filename, "r+");

	if(!pfile){
		printf("Not exist \'%s\'\n", filename);
		return;
	}//파일이 없는 경우 리턴
	while(fgets(str, sizeof(str), pfile))
		printf("%s", str);


	fclose(pfile);
	insert(input_command);
}

//object 파일을 생성하기 위한 함수
//Linked list 형태로 저장된 데이터를 읽어서 형식에 맞게 object file을 생성한다. 
int make_object_file(FILE *obj){
	INFO *tmp = head_asm;
	int start_address=0, last_address=0;
	char arr[MAX_LEN]; //text record 출력시 임시로 저장해 놓을 공간
	int len=0;

	//header record
	if(strcmp(tmp->mnemonic, "START")) return -1;
	else {
		if(!strcmp(program_name, "-")) fprintf(obj, "H\t");
		else fprintf(obj, "H%s", program_name);

		fprintf(obj, "\t%06X%06X\n", start_loc, end_loc-start_loc);
	}

	arr[0] = '\0';

	tmp = tmp->link;

	//text record
	start_address=tmp->loc;
	len += strlen(tmp->object_code);
	while(strcmp(tmp->mnemonic, "END")){
		if(len+strlen(tmp->object_code)>58) {
			//최대 길이 넘으면 줄바꿈
			len=0;
			last_address=tmp->loc;
			fprintf(obj, "T%06X%02X%s\n",start_address, last_address-start_address, arr);
			//arr에 임시로 저장해두었던 값 출력
			arr[0] = '\0';//arr 초기화
		}
		else{
			if(!strcmp(tmp->object_code, "-")) {//object가 존재하지 않는 경우 넘어감
				tmp=tmp->link;
				continue;
			}
			else {
				if(len==0) start_address=tmp->loc;
				strcat(arr, tmp->object_code); //arr 뒤에 tmp->object_code를 이어붙임
				tmp = tmp->link;
				len += strlen(tmp->object_code);
			}
			if(!strcmp(tmp->mnemonic, "RESB") || !strcmp(tmp->mnemonic, "RESW") || !strcmp(tmp->mnemonic, "RESD")){ 
				//변수 만나면 한번 끊어줌
				last_address=tmp->loc;
				fprintf(obj, "T%06X%02X%s\n", start_address, last_address-start_address, arr);
				len=0;
				arr[0] = '\0';
			}
		}

	}
	last_address = tmp->loc;
	fprintf(obj, "T%06X%02X%s\n", start_address, last_address-start_address, arr);


	//modification record
	tmp=head_asm;
	while(strcmp(tmp->mnemonic, "END")){
		if(tmp->type==4){
			if(find_symtab(tmp->operand)!=-1)
				fprintf(obj, "M%06X05\n", tmp->loc+1);
			tmp=tmp->link;
		}
		tmp = tmp->link;
	}

	//End record
	fprintf(obj, "E%06X\n", start_loc);

	return 1;
}


// PROJECT 3

//입력으로 progaddr [address]가 들어왔을 때, PROGADDR을 지정해주는 함수이다.
//loader 또는 run 명령어를 수행할 때 시작하는 주소를 지정한다.
void command_progaddr(char command[]){
	char *addr_s;
	char cmd_s[MAX_LEN];

	strcpy(cmd_s, command);
	command = strtok(command, " ");
	addr_s = strtok(NULL, " ");
	if(!addr_s) {printf("ERROR (:No address entered)\n"); return;}
	PROGADDR = strtol(addr_s, NULL, 16); //PROGADDR 설정
	
	if(PROGADDR<0 || PROGADDR>0xFFFFF) {printf("ERROR (: PROGADDR range error)\n"); return;}
	//입력받은 PROGADDR의 값이 범위를 벗어나는 경우 에러 메시지 출력하고 return

	load_file=0;
	run=0;
	CSLTH=0;
	CSADDR=0;
	EXECADDR=0;
	total_len=0;
	for(int i=0;i<10;i++) R[i]=0;
	//loader 또는 run을 위해 레지스터, CSLTH, EXECADDR, CSADDR 등 초기화

	insert(cmd_s); //history에 추가
}

//입력으로 loader [object filename1] [object filename2] [...] 가 들어왔을 때 object file을 읽어서 linking 작업을 수행 후, 가상 메모리에 결과를 기록하기 위한 함수이다.
//함수 안에서 loader_pass1, loader_pass2를 호출하여 수행한다.
//파일 개수는 최대 3개까지 고려한다.
void command_loader(char command[]){
	int type;
	char *ob1, *ob2, *ob3;
	FILE *fp1, *fp2, *fp3;
	char cmd_s[MAX_LEN];
	int ret1, ret2;
	BP *tmp;
	char obj_s[4] = ".obj";
	strcpy(cmd_s, command);


	for(int i=1;i<=3;i++){
		estab[i].link = NULL;
		estab[i].length = 0;
		estab[i].address = 0;
		estab[i].control_section[0]='\0';
	}
	//ESTAB 초기화

	ret1=0;
	ret2=0;
	type=0;
	command = strtok(command, " ");
	ob1 = strtok(NULL, " ");
	if(ob1==NULL) { printf("ERROR (:No file entered)\n"); return; }
	fp1 = fopen(ob1, "r");
	if(!fp1){
		printf("[%s] ", ob1);
		ret1=1;
	}
	else{
		for(int i=3;i>=0;i--){
			if(obj_s[i]!=ob1[strlen(ob1)-4+i]) {
				ret2=1;
				printf("[%s] ", ob1);
				break;
			}
		}
	}
	ob2 = strtok(NULL, " ");
	if(ob2==NULL) type=1;
	else {
		fp2 = fopen(ob2, "r");
		if(!fp2){
			printf("[%s] ", ob2);
			ret1=1;
		}
		else{
			for(int i=3;i>=0;i--){
				if(obj_s[i]!=ob2[strlen(ob2)-4+i]) {
					ret2=1;
					printf("[%s] ", ob2);
					break;
				}
			}
		}
		ob3 = strtok(NULL, " ");
		if(ob3==NULL) type=2;
		else {
			fp3 = fopen(ob3, "r");
			if(!fp3){
				printf("[%s] ", ob3);
				ret1=1;
			}
			else{
				for(int i=3;i>=0;i--){
					if(obj_s[i]!=ob3[strlen(ob3)-4+i]) {
						ret2=1;
						printf("[%s] ", ob3);
						break;
					}
				}
			}
			type=3;
		}
	}
	//파일 열고 파일 개수 몇개인지 type에 저장

	if(ret1==1){
		printf(" : not exist file\n");
		return;
	} //존재하지 않는 파일인경우 에러 출력하고 return

	if(ret2==1){
		printf(" : not '.obj' file\n");
		return;
	} //object file 이 아닌 경우 에러 출력하고 return

	CSADDR = PROGADDR;
	int ret=0;
	if(type==1){
		ret = loader_pass1(fp1, type, 1);
		if(ret==-1) return;
	}
	else if(type==2){
		ret = loader_pass1(fp1, type, 1);
		if(ret==-1) return;
		ret = loader_pass1(fp2, type, 2);
		if(ret==-1) return;
	}
	else if(type==3){
		ret = loader_pass1(fp1, type, 1);
		if(ret==-1) return;
		ret = loader_pass1(fp2, type, 2);
		if(ret==-1) return;
		ret = loader_pass1(fp3, type, 3);
		if(ret==-1) return;
	}
	//loader_pass1 함수 호출하여 pass1 수행

	total_len=0;
	ESTAB *ptr;
	printf("control	symbol	address	length\n");
	printf("section	name\n");
	printf("----------------------------------\n");
	for(int i=1;i<=type;i++){
		printf("%s\t\t%04X\t%04X\n", estab[i].control_section, estab[i].address, estab[i].length);
		total_len +=estab[i].length;
		for(ptr=estab[i].link;ptr;ptr=ptr->link)
			printf("\t%s\t%04X\n", ptr->symbol_name, ptr->address);
	}
	printf("----------------------------------\n");
	printf("\ttotal length\t%04X\n", total_len);
	//load map 출력

	CSADDR = PROGADDR;
	EXECADDR = PROGADDR;
	if(type==1){
		ret = loader_pass2(fp1, type, 1);
		if(ret==-1) return;
	}
	else if(type==2){
		ret = loader_pass2(fp1, type, 1);
		if(ret==-1) return;
		ret = loader_pass2(fp2, type, 2);
		if(ret==-1) return;
	}
	else if(type==3){
		ret = loader_pass2(fp1, type, 1);
		if(ret==-1) return;
		ret = loader_pass2(fp2, type, 2);
		if(ret==-1) return;
		ret = loader_pass2(fp3, type, 3);
		if(ret==-1) return;
	}
	//loader_pass2 함수 호출하여 pass2 수행

	for(tmp=head_bp;tmp;tmp=tmp->link)
		tmp->check=0;

	for(int j=0;j<10;j++) R[j]=0;
	//레지스터 초기화

	load_file=1; //파일 로드에 성공했을 경우
	run_start = EXECADDR; //run_start를 EXECADDR로 설정
	run=0;

	insert(cmd_s); //history에 추가
}

//linking loader pass1 함수
//외부 심볼에 대한 주소 지정
int loader_pass1(FILE *fp, int type, int file_num){
	char buffer[MAX_LEN];
	char *prog_name;
	char *temp;
	char *sym, addr[6];
	char *str;
	int i, j;
	char start_addr_s[7], prog_len_s[7];
	ESTAB *ptr;
	
	fgets(buffer, sizeof(buffer), fp);
	if(buffer[0]=='H'){
		prog_name = strtok(buffer, " \t");
		for(i = 0;i<strlen(prog_name)-1;i++){
			prog_name[i] = prog_name[i+1];
		}
		prog_name[i] = '\0';

		temp = strtok(NULL, " \t");
		strncpy(start_addr_s, temp, 6);
		start_addr_s[7] ='\0';
		strncpy(prog_len_s, temp+6, 6);
		prog_len_s[7] = '\0';

		CSLTH = strtol(prog_len_s, NULL, 16);

		if(!strcmp(estab[0].control_section, prog_name) || !strcmp(estab[1].control_section, prog_name) || !strcmp(estab[2].control_section, prog_name)) {
			printf("ERROR : duplicate external symbol\n");
			return -1;
		} //estab에 이미 control section name이 존재하는 경우 에러 출력하고 return
		else{
			strcpy(estab[file_num].control_section, prog_name);
			estab[file_num].address = CSADDR+strtol(start_addr_s, NULL, 16);
			estab[file_num].length = CSLTH;
		} //없는경우 추가 
	}
	
	while(fgets(buffer, sizeof(buffer), fp)!=NULL){
		if(buffer[0]=='E') break;

		if(buffer[0]=='D'){
			sym=strtok(buffer, " \t");
			for(i=0;i<strlen(buffer)-1;i++){
				sym[i] = sym[i+1];
			}
			sym[i] = '\0';
			
			str = strtok(NULL, " \t");
			while(strlen(str)>7){
				strncpy(addr, str, 6);
				for(j=0;j<=file_num;j++){
					for(ptr=estab[j].link;ptr;ptr=ptr->link){
						if(!strcmp(ptr->symbol_name, sym)){
							printf("ERROR : duplicate external symbol\n");
							return -1;
						} //이미 estab에 해당 symbol이 존재하는 경우 에러 출력하고 return
					}
				}
				insert_estab("-", sym, strtol(addr, NULL, 16), 0, file_num);
				strncpy(sym, str+6, strlen(str)-6);
				sym[strlen(str)-6]='\0';
				str = strtok(NULL, " \t");
			}
			strncpy(addr, str, 6);
			insert_estab("-", sym, strtol(addr, NULL, 16), 0, file_num); //estab에 저장
		}
	
	}
	CSADDR+=CSLTH;

	return 1;

}

//loader_pass1에서 외부 기호들을 저장하기 위해 사용하는 함수이다.
//전달받은 파라미터들을 저장하고 linked list 형태로 estab[3]에 연결한다.
//load map 출력을 위한 linked list
void insert_estab(char *control_section, char *symbol_name, int addr, int len, int file_num){
	ESTAB *new;


	new = (ESTAB*)malloc(sizeof(ESTAB));
	
	strcpy(new->symbol_name, symbol_name);
	new->address = addr+CSADDR;

	if(estab[file_num].link==NULL){
		estab[file_num].link = new;
		es_tail[file_num] = new;
	}
	else{
		es_tail[file_num]->link = new;
		es_tail[file_num] = new;
	}
	//linked list 형태로 estab[3]에 저장
}

//linking loader pass2 함수이다.
//linkng 과 loading을 수행하고, 가상 메모리에 load 한다.
int loader_pass2(FILE* fp, int type, int file_num){
	char buffer[MAX_LEN];
	char t_start[7], t_len[3];
	char data[MAX_LEN];
	char specified_addr[MAX_LEN];
	char *r_tmp, r_sym[MAX_LEN], r_num[MAX_LEN];
	char m_addr[7], m_len[3], m_sym_num[3];
	int m_addr_int, m_len_int;
	int m_sym_addr;
	int i, j;

	fseek(fp, 0, SEEK_SET);

	CSLTH = estab[file_num].length;

	strcpy(R_record[1], estab[file_num].control_section);

	while(fgets(buffer, sizeof(buffer), fp)!=NULL){
		t_start[0] = '\0';
		t_len[0] = '\0';
		if(buffer[0]=='.') continue;
		else if(buffer[0]=='R'){
			buffer[strlen(buffer)-1]='\0';
			r_tmp = strtok(buffer, " ");			
			for(i = 0;i<strlen(r_tmp)-1;i++)
				r_tmp[i] = r_tmp[i+1];
			r_tmp[i] = '\0';
			for(j=0;j<2;j++) r_num[j] = r_tmp[j];
			r_num[2] = '\0';
			for(j=0;j<strlen(r_tmp)-2;j++) r_sym[j] = r_tmp[j+2];
			r_sym[j] = '\0';

			strcpy(R_record[strtol(r_num, NULL, 16)], r_sym);
			r_tmp = strtok(NULL, " ");
			while(r_tmp!=NULL){
				for(j=0;j<2;j++) r_num[j] = r_tmp[j];
				r_num[2] = '\0';
				for(j=0;j<strlen(r_tmp)-2;j++) r_sym[j] = r_tmp[j+2];
				r_sym[j] = '\0';
				strcpy(R_record[strtol(r_num, NULL, 16)], r_sym);
				r_tmp = strtok(NULL, " ");
			}
		}
		else if(buffer[0]=='T'){ //object file의 line 첫 글자가 'T'인 경우 : Text Record
			strncpy(t_start, buffer+1, 6);
			t_start[6] = '\0';
			strncpy(t_len, buffer+7, 2);
			t_len[2] = '\0';

			int idx = 0;
			for(i=CSADDR+strtol(t_start, NULL, 16);i<CSADDR+strtol(t_start, NULL, 16)+strtol(t_len, NULL, 16);i++){
				strncpy(data, buffer+9+idx, 2);
				MEMORY[i] = (char)(strtol(data, NULL, 16)); //메모리에 기록
				idx+=2;
			}

		}
		else if(buffer[0]=='M'){ //object file의 line 첫 글자가 'M'인 경우 : Modification record
			
			strncpy(m_addr, buffer+1, 6);
			m_addr[6] = '\0';
			strncpy(m_len, buffer+7, 2);
			m_len[2] = '\0';
			strncpy(m_sym_num, buffer+10, 2);
			m_sym_num[2] = '\0';


			m_addr_int = CSADDR + strtol(m_addr, NULL, 16);
			m_len_int = strtol(m_len, NULL, 16);

			if(strtol(m_sym_num, NULL, 16)==1) m_sym_addr = estab[file_num].address;
			else{
				m_sym_addr = find_estab(R_record[strtol(m_sym_num, NULL, 16)], type);
				if(m_sym_addr<0) {printf("ERROR (:undefined external symbol)\n"); return -1;} 
				//estab에 존재하지 않는 symbol인 경우 에러메시지 출력하고 return
			}


			int num=0;
			int num2=0;
			int div=1;
			int odd=0;
			int neg=0;
			int num_neg=1;
			if(m_len_int%2!=0){
				//len이 홀수
				odd = MEMORY[m_addr_int]/16;
				MEMORY[m_addr_int] = MEMORY[m_addr_int]%16;
				m_len_int++;
				num=0;
				for(i=0;i<m_len_int/2;i++){
					num2 = MEMORY[m_addr_int+i];
					for(j=0;j<m_len_int-2*(i+1);j++)
						num2 *= 16;
					num += num2;
				} //memory 값 읽어와서 num에 저장
				if(buffer[9]=='+')
					num+= m_sym_addr;
				else if(buffer[9]=='-')
					num-= m_sym_addr; //+, - 기호에 맞게 연산
				if(num<0){
					for(i=0;i<m_len_int;i++) num_neg *= 16;
					neg += num_neg;
					num = num_neg+num;
				} //음수인경우
				for(i=0;i<m_len_int/2;i++){
					div=1;
					for(j=0;j<m_len_int-2*i;j++) div *= 16;
					num = num%div;
					div = div/(16*16);
					MEMORY[m_addr_int+i] = num/div;
				}
				MEMORY[m_addr_int] = MEMORY[m_addr_int]%16;
				MEMORY[m_addr_int] += 16*odd; //메모리에 다시 저장
			}
			else{
				//len이 짝수
				num=0;
				for(i=0;i<m_len_int/2;i++){
					num2 = MEMORY[m_addr_int+i];
					for(j=0;j<m_len_int-2*(i+1);j++)
						num2 *= 16;
					num += num2;
				} //memory 값 읽어와서 num에 저장
				if(buffer[9]=='+')
					num+=m_sym_addr;
				else if(buffer[9]=='-')
					num-=m_sym_addr; //연산
				if(num<0){
					for(i=0;i<m_len_int;i++) num_neg *=16;
					neg += num_neg;
					num = num_neg+num;
				}
				for(i=0;i<m_len_int/2;i++){
					div=1;
					for(j=0;j<m_len_int-2*i;j++) div *=16;
					num = num%div;
					div=div/(16*16);
					MEMORY[m_addr_int+i] = num/div; //메모리에 다시 저장
				}
			}

		}
		else if(buffer[0]=='E'){ //object file의 line 첫 글자가 'E'인 경우 (:End record)
			if(strlen(buffer)>=3){
				strncpy(specified_addr, buffer+1, 6);
				EXECADDR = CSADDR+strtol(specified_addr, NULL, 16);
			}
			break;
		}
	}
	CSADDR += CSLTH;

	return 1;
}

//symbol이 estab에 있는지 확인하는 함수이다.
//존재하면 해당 symbol의 address를 return 하고, 존재하지 않으면 -1을 return 한다.
int find_estab(char symbol[], int type){
	ESTAB *tmp;

	for(int i=1;i<=type;i++){
		for(tmp=estab[i].link;tmp;tmp=tmp->link){
			if(!strcmp(symbol, tmp->symbol_name))
					return tmp->address;
		}
	}

	return -1;
}

//입력으로 bp가 들어왔을 때, sicsim에 존재하는 breakpoint를 전부 화면에 출력하는 함수이다.
void command_bp(char command[]){
	BP *ptr;

	printf("\tbreakpoint\n\t----------\n");
	ptr = head_bp;
	while(ptr!=NULL){
		printf("\t%X\n", ptr->break_point);
		ptr = ptr->link;
	}
	insert(command); //history에 추가
}

//입력으로 bp clear가 들어왔을 때, sicsim에 존재하는 breakpoint를 전부 삭제하는 함수이다.
void command_bp_clear(char command[]){
	BP *ptr;

	while(head_bp!=NULL){
		ptr = head_bp;
		head_bp = head_bp->link;
		free(ptr);
		ptr=NULL;
	}

	printf("\t[ok] clear all breakpoints\n");

	insert(command); //history에 추가
}

//입력으로 bp [address]가 들어왔을 때, 수행하는 함수이다.
//sicsim에 breakpoint를 지정하는 함수이다.
//입력되는 bp가 프로그램 길이와 다르거나 loc 값과 다른 경우에도 bp에 추가는 되지만, run할 때 무시한다.
void command_bp_addr(char command[]){
	char cmd[MAX_LEN];
	char *addr;
	int bp;
	BP *new, *ptr;

	strcpy(cmd, command);
	addr = strtok(command, " ");
	addr = strtok(NULL, " ");
	bp = strtol(addr, NULL, 16);

	for(int i=0;i<strlen(addr);i++){
		if(!((addr[i]>=48&&addr[i]<=57)||(addr[i]>=65&&addr[i]<=70)||(addr[i]>=97&&addr[i]<=102))){
			printf("ERROR (:not hex address)\n");
			return;
		}
	} //16진수 형태가 아닌경우 에러메시지 출력 후 return

	new = (BP*)malloc(sizeof(BP));
	new->link = NULL;
	new->break_point = bp;

	if(!head_bp){
		head_bp = new;
		tail_bp = new;
	}
	else{
		for(ptr=head_bp;ptr;ptr=ptr->link){
			if(new->break_point==ptr->break_point){
				printf("ERROR (:duplicate break point)\n");
				return;
			} //이미 존재하는 bp인 경우, 에러메시지 출력하고 return
		}
		tail_bp->link = new;
		tail_bp = new;
	}

	printf("\t[ok] create breakpoint %x\n", new->break_point); 
	//정상적으로 breakpoint 추가했을 경우 메시지 출력

	insert(cmd); //history에 추가
}

//입력으로 run 명령어가 들어왔을 때 수행하는 함수이다.
//loader 명령어의 수행으로 메모리에 load된 프로그램을 실행한다.
//progaddr로 지정된 주소부터 실행된다.
void command_run(char command[]){
	int opcode, n, i, x, b, p, e, reg1, reg2, type;
	BP *tmp;
	int objectcode;
	char object_code[MAX_LEN];
	Hash_node *temp;
	int ta=0;
	int indirect_data;
	int comp=-10;
	
	//R[0]:A, R[1]:X, R[2]:L, R[3]:B, R[4]:S, R[5]:T, R[6]:F, R[8]:PC, R[9]:SW

	if(run==0){ //처음 run 실행할 때
		for(int j=0;j<10;j++) R[j]=0; //레지스터 초기화
		R[8] = EXECADDR; //PC를 EXECADDR로 초기화
		R[2] = total_len; //L 레지스터를 프로그램 길이로 초기화
		run=1;
		run_start = EXECADDR;
	}
	if(load_file==0){R[8]=PROGADDR; run_start=run_end=PROGADDR;}
	//로드된 파일이 없는 경우

	run_end=PROGADDR+total_len;

	while(R[8]<=total_len+PROGADDR){

		for(tmp=head_bp;tmp;tmp=tmp->link){
			if(R[8]==tmp->break_point && tmp->check!=1){
				tmp->check=1;
				run_end=tmp->break_point;
			}
		} //PC가 breakpoint와 같은경우 run_end에 breakpoint값 저장

		if(R[8]==run_end && R[8]<total_len){
			print_register();
			printf("\t     Stop at checkpoint[%X]\n", run_end);
			run_start = run_end;
			break;
		} 
		//레지스터 값 출력해주고 break, 다음 run 수행 시 종료한 부분부터 실행하기 위해 run_start=run_end로 설정

		if(R[8]>=total_len+PROGADDR){
			print_register();
			printf("\t     End Program\n");
			run=0;
			break;
		} //프로그램 끝까지 run 한 경우 레지스터 값 출력해주고 run=0으로 설정 후 break

		sprintf(object_code, "%02X%02X%02X", MEMORY[R[8]], MEMORY[R[8]+1], MEMORY[R[8]+2]);
		object_code[6] = '\0';
		objectcode = strtol(object_code, NULL, 16);
		opcode = objectcode/0x00FFFF - (objectcode/0x00FFFF)%4;

		for(int j=0;j<HASH_SIZE;j++){
			for(temp=table[j];temp!=NULL;temp=temp->link){
				if(temp->n==opcode) type=atoi(temp->type);
			}
		}
		//opcode 형식 찾기

		if(type==0){printf("ERROR\n"); return;} //잘못된 opcode 인 경우

		else if(type==1) R[8]+=type; //1형식
		else if(type==2){ //2형식
			R[8]+=type; //pc값 증가
			reg1 = (objectcode&0x00F000)/0x001000;
			reg2 = (objectcode&0x000F00)/0x000100;
			
			switch(opcode){
				case 0x90: //ADDR
					R[reg2] = R[reg1]+R[reg2];
					break;
				case 0xB4: //CLEAR
					R[reg1]=0;
					break;
				case 0xA0: //COMPR
					if(R[reg1]==R[reg2]) comp=0;
					else if(R[reg1]>R[reg2]) comp=-1;
					else if(R[reg1]<R[reg2]) comp=1;
					break;
				case 0x9C: //DIVR
					R[reg2] = R[reg2]/R[reg1];
					break;
				case 0x98: //MULR
					R[reg2] = R[reg2]*R[reg1];
					break;
				case 0xAC: //RMO
					R[reg2] = R[reg1];
					break;
				case 0xA4: //SHIFTL
					break;
				case 0xA8: //SHIFTR
					break;
				case 0x94: //SUBR
					R[reg2] = R[reg2]-R[reg1];
					break;
				case 0xB0: //SVC
					break;
				case 0xB8: //TIXR
					R[1]++;
					if(R[1]==R[reg1]) comp=0;
					else if(R[1]>R[reg1]) comp=1;
					else if(R[1]<R[reg1]) comp=-1;
					break;
			}
		}
		else if(type==3||type==4){ //3,4형식
			n = (objectcode&0x020000)/0x020000;
			i = (objectcode&0x010000)/0x010000;
			x = (objectcode&0x008000)/0x008000;
			b = (objectcode&0x004000)/0x004000;
			p = (objectcode&0x002000)/0x002000;
			e = (objectcode&0x001000)/0x001000;

			ta = objectcode&0x000FFF;

			if(e){ //e=1인경우 : 4형식
				type=4;
				sprintf(object_code, "%02X%02X%02X%02X", MEMORY[R[8]], MEMORY[R[8]+1], MEMORY[R[8]+2], MEMORY[R[8]+3]);
				object_code[8] = '\0';
				objectcode = strtol(object_code, NULL, 16);
				ta = objectcode&0x000FFFFF;
			}

			R[8]+=type; //pc값 증가

			if(b){ ta += R[3]; ta = ta&0xFFFFF; } //base relative
			else if(p){ //pc relative
				if(type==3){
					if((ta&0x800)==0x800) 
						ta += 0xFF000;
					ta += R[8];
				}
				else ta += R[8];
				ta = ta&0xFFFFF;
			}
			if(x){ //index addressing
				ta += R[1];
				ta = ta&0xFFFFF;
			}

			if(n==1 && i==1){ //simple addressing
				if(ta<=0xFFFFD){
					sprintf(object_code, "%02X%02X%02X", MEMORY[ta], MEMORY[ta+1], MEMORY[ta+2]);
					objectcode = strtol(object_code, NULL, 16);
				}
				else {printf("ERROR\n"); return;}
			}
			else if(n==1 && i==0){ //indirect addressing
				if(ta!=0xFFFFFF){
					if(ta>0xFFFFD){ printf("ERROR\n"); return;}
					sprintf(object_code, "%02X%02X%02X", MEMORY[ta], MEMORY[ta+1], MEMORY[ta+2]);
					indirect_data = strtol(object_code, NULL, 16);
				}
				if(indirect_data!=0xFFFFFF){
					if(indirect_data>0xFFFFD){ printf("ERROR\n"); return;}
					sprintf(object_code, "%02X%02X%02X", MEMORY[indirect_data], MEMORY[indirect_data+1], MEMORY[indirect_data+2]);
					objectcode = strtol(object_code, NULL, 16);
				}
				ta = indirect_data;
			}
			else if(n==0 && i==1){ //immediate addresssing
				objectcode = ta;
			}

			
			switch(opcode){
				case 0x18: //ADD
					R[0] = R[0]+objectcode;
					break;
				case 0x58: //ADDF
					break;
				case 0x40: //AND
					R[0] = R[0]&objectcode;
					break;
				case 0x28: //COMP
					if(R[0]==objectcode) comp=0;
					else if(R[0]>objectcode) comp=1;
					else if(R[0]<objectcode) comp=-1;
					break;
				case 0x88: //COMPF
					break;
				case 0x24: //DIV
					R[0] = R[0]/objectcode;
					break;
				case 0x64: //DIVF
					break;
				case 0x3C: //J
					R[8] = ta;
					break;
				case 0x30: //JEQ
					if(comp==0) R[8] = ta;
					break;
				case 0x34: //JGT
					if(comp==1) R[8] = ta;
					break;
				case 0x38: // JLT
					if(comp==-1) R[8] = ta;
					break;
				case 0x48: //JSUB
					R[2]=R[8]; R[8]=ta;
					break;
				case 0x00: //LDA
					R[0] = objectcode;
					break;
				case 0x68: //LDB
					R[3] = objectcode;
					break;
				case 0x50: //LDCH
					R[0] = R[0]&0xFFFF00;
					objectcode = (objectcode&0xFF0000)/0x010000;
					R[0] = R[0]|objectcode;
					break;
				case 0x70: //LDF
					break;
				case 0x08: //LDL
					R[2] = objectcode;
					break;
				case 0x6C: //LDS
					R[4] = objectcode;
					break;
				case 0x74: //LDT
					R[5] = objectcode;
					break;
				case 0x04: //LDX
					R[1] = objectcode;
					break;
				case 0xD0: //LPS
					break;
				case 0x20: //MUL
					R[0] = R[0]*objectcode;
					break;
				case 0x60: //MULF
					break;
				case 0x44: //OR
					R[0] = R[0]|objectcode;
					break;
				case 0xD8: //RD
					comp=0;
					break;
				case 0x4C: //RSUB
					R[8] = R[2];
					break;
				case 0xEC: //SSK
					break;
				case 0x0C: //STA
					MEMORY[ta] = ((R[0]&0xFF0000)/0x010000);
					MEMORY[ta+1] = ((R[0]&0x00FF00)/0x000100);
					MEMORY[ta+2] = ((R[0]&0x0000FF)/0x000001);
					break;
				case 0x78: //STB
					MEMORY[ta] = ((R[3]&0xFF0000)/0x010000);
					MEMORY[ta+1] = ((R[3]&0x00FF00)/0x000100);
					MEMORY[ta+2] = ((R[3]&0x0000FF)/0x000001);
					break;
				case 0x54: //STCH
					MEMORY[ta] = (R[0]&0x0000FF);
					break;
				case 0x80: //STF
					break;
				case 0xD4: //STI
					break;
				case 0x14: //STL
					MEMORY[ta] = ((R[2]&0xFF0000)/0x010000);
					MEMORY[ta+1] = ((R[2]&0x00FF00)/0x000100);
					MEMORY[ta+2] = ((R[2]&0x0000FF)/0x000001);
					break;
				case 0x7C: //STS
					MEMORY[ta] = ((R[4]&0xFF0000)/0x010000);
					MEMORY[ta+1] = ((R[4]&0x00FF00)/0x000100);
					MEMORY[ta+2] = ((R[4]&0x0000FF)/0x000001);
					break;
				case 0xE8: //STSW
					MEMORY[ta] = ((R[9]&0xFF0000)/0x010000);
					MEMORY[ta+1] = ((R[9]&0x00FF00)/0x000100);
					MEMORY[ta+2] = ((R[9]&0x0000FF)/0x000001);
					break;
				case 0x84: //STT
					MEMORY[ta] = ((R[5]&0xFF0000)/0x010000);
					MEMORY[ta+1] = ((R[5]&0x00FF00)/0x000100);
					MEMORY[ta+2] = ((R[5]&0x0000FF)/0x000001);
					break;
				case 0x10: //STX
					MEMORY[ta] = ((R[1]&0xFF0000)/0x010000);
					MEMORY[ta+1] = ((R[1]&0x00FF00)/0x000100);
					MEMORY[ta+2] = ((R[1]&0x0000FF)/0x000001);
					break;
				case 0x1C: //SUB
					R[0] = R[0]-objectcode;
					break;
				case 0x5C: //SUBF
					break;
				case 0xE0: //TD
					comp=-1;
					break;
				case 0x2C: //TIX
					R[1]++;
					if(R[1]==objectcode) comp=0;
					else if(R[1]>objectcode) comp=1;
					else if(R[1]<objectcode) comp=-1;
					break;
				case 0xDC: //WD
					//continue;
					break;
			}
		}
	}
	
	run_start = run_end;
	insert(command); //history에 추가

}

//run 명령어 수행할 때 PC==bp이거나 PC==program len 인 경우 레지스터 값을 출력해주기 위한 함수
void print_register(){
	printf("A : %06X   X : %06X\n", R[0], R[1]);
	printf("L : %06X  PC : %06X\n", R[2], R[8]); 
	printf("B : %06X   S : %06X\n", R[3], R[4]);
	printf("T : %06X\n", R[5]);
}

