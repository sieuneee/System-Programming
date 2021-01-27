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
	printf("h[elp]\nd[ir]\nq[uit]\nhi[story]\ndu[mp] [start, end]\ne[dit] address, value\nf[ill] start, end, value\nreset\nopcode mnemonic\nopcodelist\nasssemble filename\ntype filename\nsymbol\n");
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
	if(start_int>MEMORY_SIZE || end_int<0 || start_int>end_int){
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
	
	LOCCTR = atoi(operand); //주소 배정을 위한 포인터에 주소 저장
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
		printf("Not exits \'%s\'\n", filename);
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
				//변수 만나면 한번 끊바꿈
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


