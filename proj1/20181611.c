#include "20181611.h"

Hash_node * table[HASH_SIZE]; //hash table

NODE *head=NULL, *temp=NULL; //history 저장할 떄 사용할 linked list 포인터

unsigned char MEMORY[MEMORY_SIZE]; //1MB 메모리
int last_address=-1; //메모리 출력 시 이전 주소가 저장된 변수
int quit=0; //q[uit] command가 입력되었을 때 값이 1로 바뀐다.

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
	printf("h[elp]\nd[ir]\nq[uit]\nhi[story]\ndu[mp] [start, end]\ne[dit] address, value\nf[ill] start, end, value\nreset\nopcode mnemonic\nopcodelist\n");
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
	else printf("ERROR\n");

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
