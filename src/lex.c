/*
 * lex.c
 *
 *  Created on: 2018��6��12��
 *      Author: Administrator
 */



/*�ʷ�����*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "lex.h"

static void *my_malloc(int len)
{
	void *p;
	p = malloc(len);

	if(!p){
		perror("malloc:");
		exit(-1);
	}
	return p;
}

//������
struct pnode{
	void *data;
	struct pnode *next;
};
static struct pnode *sym_head=NULL,*sym_curr=NULL;
static int node_num = 0;
/*����һ�����ű�ṹ�壬���ӵ�������*/
static sym_tb *alloc_sym()
{
	//��������ṹ
	struct pnode *pn = (struct pnode *)my_malloc(sizeof(struct pnode));
	pn->next = NULL;//�½ڵ���һ������ΪNULL
	if(!sym_head){ //��һ�γ�ʼ��
		sym_head = pn; //sym_headָ�������ͷ
		sym_curr = pn; //sym_currָ��ǰ���½ڵ�
	}else{
		sym_curr->next = pn;
		sym_curr = pn;
	}

	sym_tb *tb = my_malloc(sizeof(sym_tb));
	sym_curr->data = tb;//�����������ӵ�������
	node_num++; //�ڵ����
	return tb;

}
//�ͷ����нڵ�
static void free_pnode()
{
	while(sym_head){
		sym_curr = sym_head->next; //������һ�ڵ�ĵ�ַ
		free(sym_head); //�ͷ�ͷ���
		sym_head = sym_curr;
	}
}
//��������������ڲ���
static array *create_array_fr_node()
{
	int i;
	sym_tb **dat = my_malloc(node_num*sizeof(char *));//��������ָ��

	struct pnode *sym_pos = sym_head;
	array *arr = my_malloc(sizeof(array));

	for(i=0;i<node_num;i++){
		dat[i] = (sym_tb *)(sym_pos->data);
		sym_pos = sym_pos->next;
	}
	free_pnode();
	arr->len = node_num; //����Ԫ�ظ���
	arr->data = (void **)dat;

	return arr;
}

array *lex(char *filename)
{
	//�򿪵ĵ�ǰ�ļ�
	FILE *fp;
	int file_len;
	int ret;
	char *file_data;
	int line; //�к�
	fp = fopen(filename, "r");
	if(!fp){
		perror("fopen:");
		return NULL;
	}
	//���ļ��ĳ���
	fseek(fp, 0, SEEK_END);
	file_len = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	file_data = my_malloc(file_len+1);
	//���ļ�ȫ�������ڴ棬ע�� fread����ֵΪ�������ֽ��������ļ�ΪDOS��ʽ file_len != ret,
	//DOS��ʽ \r\n ��\r��fread�˵�
	ret = fread(file_data, 1,file_len, fp);

	file_data[file_len] = '\0';
	fclose(fp);

	line = 1;
	char tmp;
	char *src = file_data;

	while(tmp = *src){
		src++;

		if(tmp == '\n'){
			line++;
		}
		else if(tmp == '#'){
			while((*src != 0) && (*src != '\n')) ++src;
		}
		//��ʶ��
		else if((tmp >= 'a' && tmp <= 'z') || (tmp >= 'A' && tmp <= 'Z') || tmp == '_'){
			char *s_head = src-1; //��ʶ����ͷ
			while ((*src >= 'a' && *src <= 'z') || (*src >= 'A' && *src <= 'Z') || (*src >= '0' && *src <= '9') || *src == '_')
				++src;
			//�ҵ�һ����ʶ��
			int s_len = src - s_head; //��ʶ������
			sym_tb *sym = alloc_sym();
			sym->sym_type = SYM_ID; //��ʶ��
			sym->sym_name = my_malloc(s_len+1);
			strncpy(sym->sym_name,s_head,s_len );
			sym->sym_name[s_len] = '\0';
			sym->value = s_len; //id�ַ�������
			sym->line = line;
		}
		//���ִ���
		else if (tmp >= '0' && tmp <= '9') {
			int ival;
		    if (ival = tmp - '0'){ //10���Ƶ��� // @suppress("Assignment in condition")
		    	while (*src >= '0' && *src <= '9')
		    		ival = ival * 10 + *src++ - '0';
		    }
		    else if (*src == 'x' || *src == 'X') { //16���Ƶ���
		        while ((tmp = *++src) && ((tmp >= '0' && tmp <= '9') || (tmp >= 'a' && tmp <= 'f') || (tmp >= 'A' && tmp <= 'F')))
		          ival = ival * 16 + (tmp & 15) + (tmp >= 'A' ? 9 : 0);
		    }
		    else {
		    	while (*src >= '0' && *src <= '7')
		    		ival = ival * 8 + *src++ - '0';
		    }

		    sym_tb *sym = alloc_sym();
		    sym->sym_type = SYM_NUM; //����
		    sym->value = ival;
		    sym->line = line;
		 }
		 else if (tmp == '/') {
		    if (*src == '/') { //����ע��
		        ++src;
		        while (*src != 0 && *src != '\n')
		        	++src;
		      }
		    else if(*src == '*'){ //����ע��
		    	++src;
		    	while (*src != 0 && *(src+1) != 0 ){
		    		if(*src == '*' && *(src+1) == '/' ){
		    			src++;
		    			src++;
		    			break;
		    		}
		    		if(*src == '\n')
		    			line++;
		    		++src;
		    	}
		    }
		    else if(*src == '='){  // /= ;a /= b;
		    	sym_tb *sym = alloc_sym();
		    	sym->sym_type = SYM_OP; //�����
		    	sym->value = OP_DIV_EQ; // /=
		    	sym->line = line;
		    }
		    else { //����
		    	 sym_tb *sym = alloc_sym();
		    	 sym->sym_type = SYM_OP; //�����
		    	 sym->value = OP_DIV; //
		    	 sym->line = line;
		      }
		    }
		 else if (tmp == '\'' || tmp == '"') { //�����ţ�˫����
		      //pp = data;
			 char *s_head = src;
		      while (*src != 0 && *src != tmp) {
		    	  /*
		        if ((ival = *src++) == '\\') { //ת���ַ�����
		          if ((ival = *src++) == 'n') ival = '\n';
		        }*/
		    	  src++;
		      }

		      sym_tb *sym = alloc_sym();
		      sym->line = line;
		      if (tmp == '"'){ //�ַ���
		    	  int src_len = src - s_head;
		    	  sym->sym_type = SYM_STR; //�ַ���
		    	  sym->sym_name = my_malloc(src_len+1);
		    	  sym->sym_name[src_len] = '\0';
		    	  strncpy(sym->sym_name, s_head, src_len);
		      }else{ // 'A'
		    	  sym->sym_type = 5; //�����ַ�
		    	  sym->value = *s_head;
		      }
		      src++;
		    }
		 else if (tmp == '=') {
			 sym_tb *sym = alloc_sym();
			 sym->line = line;
			 sym->sym_type = SYM_OP; //�����

			 if (*src == '=') { //����� ==
				 ++src;
				 sym->value = OP_EQ; //==
			 } else
				 sym->value = OP_ASSIGN; //= ��ֵ
		 }else if (tmp == '+') {
			 sym_tb *sym = alloc_sym();
			 sym->line = line;
			 sym->sym_type = SYM_OP; //�����
			 if (*src == '+')
			 {
				 ++src;
				 sym->value = OP_INC; //++ ,��һ
			 }
			 else if (*src == '=')
			 {
				 ++src;
				 sym->value = OP_ADD_EQ; // +=
			 }
			 else
				 sym->value = OP_ADD; //add +=��֧��
		 } else if (tmp == '-') {
			 sym_tb *sym = alloc_sym();
			 sym->line = line;
			 sym->sym_type = SYM_OP; //�����
			 if (*src == '-') {
				 ++src;
				 sym->value = OP_DEC; //--
			 } else if (*src == '>') { // -> ָ��
				 ++src;
				 sym->value = OP_DEREF; //->
			 }
			 else if (*src == '=') { // -=
			 	++src;
			 	sym->value = OP_SUB_EQ; //-=
			 }
			 else
				 sym->value = OP_SUB; //-,
		 } else if (tmp == '!') {
			 sym_tb *sym = alloc_sym();
			 sym->line = line;
			 sym->sym_type = SYM_OP; //�����
			 sym->value = OP_NE; // !
			 if (*src == '=') {
				 ++src;
				 sym->value = OP_NOT; // !=
			 }
		 }else if (tmp == '<') {
			 sym_tb *sym = alloc_sym();
			 sym->line = line;
			sym->sym_type = SYM_OP; //�����
			 if (*src == '=') { // <=
				 ++src;
				 sym->value = OP_LE; // <=
			 } else if (*src == '<') {
				 ++src;
				 sym->value = OP_SHL; // <<
				 if(*src == '='){ // <<=
					 ++src;
					 sym->value = OP_SHL_EQ;
				 }
			 } else
				 sym->value = OP_LT; // <
		 }else if (tmp == '>') {
			 sym_tb *sym = alloc_sym();
			 sym->line = line;
			 sym->sym_type = SYM_OP; //�����
			 if (*src == '=') { //>=
				 ++src;
				 sym->value = OP_GE;
			 } else if (*src == '>') {
				 ++src;
				 sym->value = OP_SHR; //>>
				 if(*src == '='){ // >>=
				 	++src;
				 	sym->value = OP_SHR_EQ;
				 }
			 } else
				 sym->value = OP_GT; // >
		 }else if (tmp == '|') {
			sym_tb *sym = alloc_sym();
			sym->line = line;
			sym->sym_type = SYM_OP; //�����
			 if (*src == '|') {
				 ++src;
				 sym->value = OP_LOR;  // ||
			 }else if (*src == '=') {
				 ++src;
				 sym->value = OP_LOR_EQ;  // |=
			 }
			 else
				 sym->value = OP_OR; // |
		 }else if (tmp == '&') {
			 sym_tb *sym = alloc_sym();
			 sym->line = line;
			 sym->sym_type = SYM_OP; //�����

			 if (*src == '&') {
				 ++src;
				 sym->value = OP_LAN;  // &&
			 }
			 else if (*src == '=') {
				 ++src;
				 sym->value = OP_LAN_EQ;  // &=
			 }
			 else
				 sym->value = OP_AND;  // &
		 }else if (tmp == '^') {
			 sym_tb *sym = alloc_sym();
			 sym->line = line;
			 sym->sym_type = SYM_OP; //�����
			 sym->value = OP_XOR;  // ^
			 if (*src == '=') {
			 	++src;
			 	sym->value = OP_XOR_EQ;  // ^=
			 }
		 } else if (tmp == '%') {
			 sym_tb *sym = alloc_sym();
			 sym->line = line;
			 sym->sym_type = SYM_OP; //�����
			 sym->value = OP_MOD;  // %
			 if (*src == '=') { // %=
				 ++src;
				sym->value = OP_MOD_EQ;  // %=
			}
		 }else if (tmp == '*') {  // *= ,ȡָ������
			 sym_tb *sym = alloc_sym();
			 sym->line = line;
			 sym->sym_type = SYM_OP; //�����
			 sym->value = OP_MUL;  // *
			 if (*src == '=') {
			 	++src;
			 	sym->value = OP_MUL_EQ;  // *=
			 }
		 } else if (tmp == '[') {
			 sym_tb *sym = alloc_sym();
			 sym->line = line;
			 sym->sym_type = SYM_LBR; //��������
		 }else if (tmp == '?') {
			 sym_tb *sym = alloc_sym();
			 sym->line = line;
			 sym->sym_type = SYM_OP; //�����
			 sym->value = OP_COND;  // ?
		 }else if (tmp == '~'){
			 sym_tb *sym = alloc_sym();
			 sym->line = line;
			 sym->sym_type = SYM_OP; //�����
			 sym->value = OP_BNO;  // ~
		 }else if(tmp == ';'){
			 sym_tb *sym = alloc_sym();
			 sym->line = line;
			 sym->sym_type = SYM_SEM; //�ֺ� ;
		 }else if(tmp == '{'){
			 sym_tb *sym = alloc_sym();
			 sym->line = line;
			 sym->sym_type = SYM_LB; //
			 sym->value = 29;  // ?
		 }else if(tmp == '}'){
			 sym_tb *sym = alloc_sym();
			 sym->line = line;
			 sym->sym_type = SYM_RB; //
			 sym->value = 30;  // ?
		 }else if(tmp == '('){
			 sym_tb *sym = alloc_sym();
			 sym->line = line;
			 sym->sym_type = SYM_LPA; //
		 }else if(tmp == ')'){
			 sym_tb *sym = alloc_sym();
			 sym->line = line;
			 sym->sym_type = SYM_RPA; //
		 }else if(tmp == ']'){
			 sym_tb *sym = alloc_sym();
			 sym->line = line;
			 sym->sym_type = SYM_RBR; //��������
		 }else if(tmp == ','){
			 sym_tb *sym = alloc_sym();
			 sym->line = line;
			 sym->sym_type = SYM_COM; //���� ��
		 }else if(tmp == ':'){
			 sym_tb *sym = alloc_sym();
			 sym->line = line;
			 sym->sym_type = SYM_COL; //ð��
		 }else if(tmp == '.'){
			 sym_tb *sym = alloc_sym();
			 sym->line = line;
			 sym->sym_type = SYM_PU; //���
			 // ... ���
			 if(*src == '.' && *(src+1) == '.'){
				 src++;
				 src++;
				 sym->sym_type = SYM_ELL; // ... ���
			 }
		 }else if((tmp == ' ') || (tmp == '\t')){ //�ո� Խ��
			 continue;
		 }else{
			 printf("�ʷ���������line: %d  %d \n",line, tmp);
		 }

	}
	//�������飬�ͷŽڵ�
	return create_array_fr_node();
}

//�ͷŽڵ�
void free_array(array *arr)
{
	int i;

	if(!arr)
		return;

	for(i = 0; i< arr->len; i++ ){
		free(arr->data[i]);
		arr->data[i] = NULL;
	}
	free(arr->data);
	arr->data = NULL;
	arr->len = 0;
}


void lex2(array *arr)
{
	int i,n;
	sym_tb **pdat;
//	char *p_id;

	if(!arr){
		printf("arr is NULL\n");
		return;
	}

	n = arr->len;
	pdat = (sym_tb **)arr->data;

	for(i=0;i<n;i++){
		if(pdat[i]->sym_type == SYM_ID){ //�ѱ�ʶ���йؼ����滻��
			switch(pdat[i]->value){ //�ַ�������
			case 2: //do,if
				if(!strncmp(pdat[i]->sym_name,"do",2)){
					pdat[i]->sym_type = SYM_KW; //�ؼ���
					pdat[i]->value = KW_DO;
					free(pdat[i]->sym_name); //�ͷſռ�
					pdat[i]->sym_name = NULL;
				}else if(!strncmp(pdat[i]->sym_name,"if",2)){
					pdat[i]->sym_type = SYM_KW; //�ؼ���
					pdat[i]->value = KW_IF;
					free(pdat[i]->sym_name); //�ͷſռ�
					pdat[i]->sym_name = NULL;
				}
				break;
			case 3: //for int
				if(!strncmp(pdat[i]->sym_name,"for",3)){
						pdat[i]->sym_type = SYM_KW; //�ؼ���
						pdat[i]->value = KW_FOR;
						free(pdat[i]->sym_name); //�ͷſռ�
						pdat[i]->sym_name = NULL;
				}else if(!strncmp(pdat[i]->sym_name,"int",3)){
						pdat[i]->sym_type = SYM_KW; //�ؼ���
						pdat[i]->value = KW_INT;
						free(pdat[i]->sym_name); //�ͷſռ�
						pdat[i]->sym_name = NULL;
				}
				break;
			case 4://auto case char else enum goto long void
				if(!strncmp(pdat[i]->sym_name,"auto",4)){
						pdat[i]->sym_type = SYM_KW; //�ؼ���
						pdat[i]->value = KW_AUTO;
						free(pdat[i]->sym_name); //�ͷſռ�
						pdat[i]->sym_name = NULL;
				}else if(!strncmp(pdat[i]->sym_name,"case",4)){
						pdat[i]->sym_type = SYM_KW; //�ؼ���
						pdat[i]->value = KW_CASE;
						free(pdat[i]->sym_name); //�ͷſռ�
						pdat[i]->sym_name = NULL;
				}else if(!strncmp(pdat[i]->sym_name,"char",4)){
					pdat[i]->sym_type = SYM_KW; //�ؼ���
					pdat[i]->value = KW_CHAR;
					free(pdat[i]->sym_name); //�ͷſռ�
					pdat[i]->sym_name = NULL;
				}else if(!strncmp(pdat[i]->sym_name,"else",4)){
					pdat[i]->sym_type = SYM_KW; //�ؼ���
					pdat[i]->value = KW_ELSE;
					free(pdat[i]->sym_name); //�ͷſռ�
					pdat[i]->sym_name = NULL;
				}else if(!strncmp(pdat[i]->sym_name,"enum",4)){
					pdat[i]->sym_type = SYM_KW; //�ؼ���
					pdat[i]->value = KW_ENUM;
					free(pdat[i]->sym_name); //�ͷſռ�
					pdat[i]->sym_name = NULL;
				}else if(!strncmp(pdat[i]->sym_name,"goto",4)){
					pdat[i]->sym_type = SYM_KW; //�ؼ���
					pdat[i]->value = KW_GOTO;
					free(pdat[i]->sym_name); //�ͷſռ�
					pdat[i]->sym_name = NULL;
				}else if(!strncmp(pdat[i]->sym_name,"long",4)){
					pdat[i]->sym_type = SYM_KW; //�ؼ���
					pdat[i]->value = KW_LONG;
					free(pdat[i]->sym_name); //�ͷſռ�
					pdat[i]->sym_name = NULL;
				}else if(!strncmp(pdat[i]->sym_name,"void",4)){
					pdat[i]->sym_type = SYM_KW; //�ؼ���
					pdat[i]->value = KW_VOID;
					free(pdat[i]->sym_name); //�ͷſռ�
					pdat[i]->sym_name = NULL;
				}
				break;
			case 5: //break const float short union while _Bool
				if(!strncmp(pdat[i]->sym_name,"break",5)){
					pdat[i]->sym_type = SYM_KW; //�ؼ���
					pdat[i]->value = KW_BREAK;
					free(pdat[i]->sym_name); //�ͷſռ�
					pdat[i]->sym_name = NULL;
				}else if(!strncmp(pdat[i]->sym_name,"const",5)){
					pdat[i]->sym_type = SYM_KW; //�ؼ���
					pdat[i]->value = KW_CONST;
					free(pdat[i]->sym_name); //�ͷſռ�
					pdat[i]->sym_name = NULL;
				}else if(!strncmp(pdat[i]->sym_name,"float",5)){
					pdat[i]->sym_type = SYM_KW; //�ؼ���
					pdat[i]->value = KW_FLOAT;
					free(pdat[i]->sym_name); //�ͷſռ�
					pdat[i]->sym_name = NULL;
				}else if(!strncmp(pdat[i]->sym_name,"short",5)){
					pdat[i]->sym_type = SYM_KW; //�ؼ���
					pdat[i]->value = KW_SHORT;
					free(pdat[i]->sym_name); //�ͷſռ�
					pdat[i]->sym_name = NULL;
				}else if(!strncmp(pdat[i]->sym_name,"union",5)){
					pdat[i]->sym_type = SYM_KW; //�ؼ���
					pdat[i]->value = KW_UNION;
					free(pdat[i]->sym_name); //�ͷſռ�
					pdat[i]->sym_name = NULL;
				}else if(!strncmp(pdat[i]->sym_name,"while",5)){
					pdat[i]->sym_type = SYM_KW; //�ؼ���
					pdat[i]->value = KW_WHILE;
					free(pdat[i]->sym_name); //�ͷſռ�
					pdat[i]->sym_name = NULL;
				}else if(!strncmp(pdat[i]->sym_name,"_Bool",5)){
					pdat[i]->sym_type = SYM_KW; //�ؼ���
					pdat[i]->value = KW_BOOL;
					free(pdat[i]->sym_name); //�ͷſռ�
					pdat[i]->sym_name = NULL;
				}
				break;
			case 6://double extern inline return signed sizeof static struct switch
				if(!strncmp(pdat[i]->sym_name,"double",6)){
					pdat[i]->sym_type = SYM_KW; //�ؼ���
					pdat[i]->value = KW_DOUBLE;
					free(pdat[i]->sym_name); //�ͷſռ�
					pdat[i]->sym_name = NULL;
				}else if(!strncmp(pdat[i]->sym_name,"extern",6)){
					pdat[i]->sym_type = SYM_KW; //�ؼ���
					pdat[i]->value = KW_EXTERN;
					free(pdat[i]->sym_name); //�ͷſռ�
					pdat[i]->sym_name = NULL;
				}else if(!strncmp(pdat[i]->sym_name,"inline",6)){
					pdat[i]->sym_type = SYM_KW; //�ؼ���
					pdat[i]->value = KW_INLINE;
					free(pdat[i]->sym_name); //�ͷſռ�
					pdat[i]->sym_name = NULL;
				}else if(!strncmp(pdat[i]->sym_name,"return",6)){
					pdat[i]->sym_type = SYM_KW; //�ؼ���
					pdat[i]->value = KW_RETURN;
					free(pdat[i]->sym_name); //�ͷſռ�
					pdat[i]->sym_name = NULL;
				}else if(!strncmp(pdat[i]->sym_name,"signed",6)){
					pdat[i]->sym_type = SYM_KW; //�ؼ���
					pdat[i]->value = KW_SIGNED;
					free(pdat[i]->sym_name); //�ͷſռ�
					pdat[i]->sym_name = NULL;
				}else if(!strncmp(pdat[i]->sym_name,"sizeof",6)){
					pdat[i]->sym_type = SYM_KW; //�ؼ���
					pdat[i]->value = KW_SIZEOF;
					free(pdat[i]->sym_name); //�ͷſռ�
					pdat[i]->sym_name = NULL;
				}else if(!strncmp(pdat[i]->sym_name,"static",6)){
					pdat[i]->sym_type = SYM_KW; //�ؼ���
					pdat[i]->value = KW_STATIC;
					free(pdat[i]->sym_name); //�ͷſռ�
					pdat[i]->sym_name = NULL;
				}else if(!strncmp(pdat[i]->sym_name,"struct",6)){
					pdat[i]->sym_type = SYM_KW; //�ؼ���
					pdat[i]->value = KW_STRUCT;
					free(pdat[i]->sym_name); //�ͷſռ�
					pdat[i]->sym_name = NULL;
				}else if(!strncmp(pdat[i]->sym_name,"switch",6)){
					pdat[i]->sym_type = SYM_KW; //�ؼ���
					pdat[i]->value = KW_SWITCH;
					free(pdat[i]->sym_name); //�ͷſռ�
					pdat[i]->sym_name = NULL;
				}
				break;
			case 7: //default typedef
				if(!strncmp(pdat[i]->sym_name,"default",7)){
					pdat[i]->sym_type = SYM_KW; //�ؼ���
					pdat[i]->value = KW_DEFAULT;
					free(pdat[i]->sym_name); //�ͷſռ�
					pdat[i]->sym_name = NULL;
				}else if(!strncmp(pdat[i]->sym_name,"typedef",7)){
					pdat[i]->sym_type = SYM_KW; //�ؼ���
					pdat[i]->value = KW_TYPEDEF;
					free(pdat[i]->sym_name); //�ͷſռ�
					pdat[i]->sym_name = NULL;
				}
				break;
			case 8://continue register restrict unsigned volatile _Complex
				if(!strncmp(pdat[i]->sym_name,"continue",8)){
					pdat[i]->sym_type = SYM_KW; //�ؼ���
					pdat[i]->value = KW_CONTINUE;
					free(pdat[i]->sym_name); //�ͷſռ�
					pdat[i]->sym_name = NULL;
				}else if(!strncmp(pdat[i]->sym_name,"register",8)){
					pdat[i]->sym_type = SYM_KW; //�ؼ���
					pdat[i]->value = KW_REGISTER;
					free(pdat[i]->sym_name); //�ͷſռ�
					pdat[i]->sym_name = NULL;
				}else if(!strncmp(pdat[i]->sym_name,"restrict",8)){
					pdat[i]->sym_type = SYM_KW; //�ؼ���
					pdat[i]->value = KW_RESTRICT;
					free(pdat[i]->sym_name); //�ͷſռ�
					pdat[i]->sym_name = NULL;
				}else if(!strncmp(pdat[i]->sym_name,"unsigned",8)){
					pdat[i]->sym_type = SYM_KW; //�ؼ���
					pdat[i]->value = KW_UNSIGNED;
					free(pdat[i]->sym_name); //�ͷſռ�
					pdat[i]->sym_name = NULL;
				}else if(!strncmp(pdat[i]->sym_name,"volatile",8)){
					pdat[i]->sym_type = SYM_KW; //�ؼ���
					pdat[i]->value = KW_VOLATILE;
					free(pdat[i]->sym_name); //�ͷſռ�
					pdat[i]->sym_name = NULL;
				}else if(!strncmp(pdat[i]->sym_name,"_Complex",8)){
					pdat[i]->sym_type = SYM_KW; //�ؼ���
					pdat[i]->value = KW_COMPLEX;
					free(pdat[i]->sym_name); //�ͷſռ�
					pdat[i]->sym_name = NULL;
				}
				//���ú�
				else if(!strncmp(pdat[i]->sym_name,"__LINE__",8)){
					pdat[i]->sym_type = SYM_NUM; //����
					pdat[i]->value = pdat[i]->line;
					free(pdat[i]->sym_name); //�ͷſռ�
					pdat[i]->sym_name = NULL;
				}
				break;
			case 10: //_Imaginary
				if(!strncmp(pdat[i]->sym_name,"_Imaginary",10)){
					pdat[i]->sym_type = SYM_KW; //�ؼ���
					pdat[i]->value = KW_IMAG;
					free(pdat[i]->sym_name); //�ͷſռ�
					pdat[i]->sym_name = NULL;
				}
				break;
		//	default:
		//		printf();
			}

		}

	}
}

//�﷨����
int yacc(array *arr)
{

	int *sym_stack; //�﷨����ջ���洢��������
	int i,n;
	sym_tb **pdat;

	if(!arr){
		printf("arr is NULL\n");
		return -1;
	}

	n = arr->len;
	pdat = (sym_tb **)arr->data;

	sym_stack = my_mallo(1024);//ջ�����

#define ST_IDLE 0
	int status = ST_IDLE;

	for(i=0; i< n;i++){

		switch(status){
		case ST_IDLE:

			break;
		}

	}



	return 0;
}
