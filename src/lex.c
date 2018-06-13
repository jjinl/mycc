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
	pn->next = NULL;
	if(!sym_head){
		sym_head = pn;
		sym_curr = pn;
	}else{
		sym_curr->next = pn;
		sym_curr = pn;
	}

	sym_tb *tb = my_malloc(sizeof(sym_tb));
	sym_curr->data = tb;
	node_num++; //�ڵ����
	return tb;

}
//�ͷ����нڵ�
static void free_pnode()
{
	while(sym_head){
		sym_curr = sym_head->next;
		free(sym_head);
		sym_head = sym_curr;
	}
}

static array *create_array_fr_node()
{
	int i;
	sym_tb **dat = my_malloc(node_num*sizeof(char *));
	struct pnode *sym_pos = sym_head;
	array *arr = my_malloc(sizeof(array));

	for(i=0;i<node_num;i++){
		dat[i] = (sym_tb *)(sym_pos->data);
		sym_pos = sym_pos->next;
	}
	free_pnode();
	arr->len = node_num;
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
	fseek(fp, 0, SEEK_END);
	file_len = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	file_data = my_malloc(file_len+1);
	ret = fread(file_data, 1,file_len, fp);
	/*
	if(ret != file_len){ //�ı��ļ�ΪUNIX��ʽ��ΪDOS��ʽʱ�򲻵�
		printf("fread error %d %d\n",ret, file_len);
		printf("%s\n",file_data);
		free(file_data);
		fclose(fp);
		return NULL;
	}*/
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
			sym->line = line;
		}
		//���ִ���
		else if (tmp >= '0' && tmp <= '9') {
			int ival;
		    if (ival = tmp - '0'){ //10���Ƶ���
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
			 } else
				 sym->value = OP_ADD; //add +=��֧��
		 } else if (tmp == '-') {
			 sym_tb *sym = alloc_sym();
			 sym->line = line;
			 sym->sym_type = SYM_OP; //�����
			 if (*src == '-') {
				 ++src;
				 sym->value = OP_DEC; //--
			 } else
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
			 } else
				 sym->value = OP_GT; // >
		 }else if (tmp == '|') {
			sym_tb *sym = alloc_sym();
			sym->line = line;
			sym->sym_type = SYM_OP; //�����
			 if (*src == '|') {
				 ++src;
				 sym->value = OP_LOR;  // ||
			 } else
				 sym->value = OP_OR; // |
		 }else if (tmp == '&') {
			 sym_tb *sym = alloc_sym();
			 sym->line = line;
			 sym->sym_type = SYM_OP; //�����

			 if (*src == '&') {
				 ++src;
				 sym->value = OP_LAN;  // &&
			 } else
				 sym->value = OP_AND;  // &
		 }else if (tmp == '^') {
			 sym_tb *sym = alloc_sym();
			 sym->line = line;
			 sym->sym_type = SYM_OP; //�����
			 sym->value = OP_XOR;  // ^
		 } else if (tmp == '%') {
			 sym_tb *sym = alloc_sym();
			 sym->line = line;
			 sym->sym_type = SYM_OP; //�����
			 sym->value = OP_MOD;  // %
		 }else if (tmp == '*') {
			 sym_tb *sym = alloc_sym();
			 sym->line = line;
			 sym->sym_type = SYM_OP; //�����
			 sym->value = OP_MUL;  // *
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

