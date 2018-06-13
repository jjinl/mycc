/*
 * lex.c
 *
 *  Created on: 2018年6月12日
 *      Author: Administrator
 */



/*词法分析*/

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

//单链表
struct pnode{
	void *data;
	struct pnode *next;
};
static struct pnode *sym_head=NULL,*sym_curr=NULL;
static int node_num = 0;
/*分配一个符号表结构体，并加到链表中*/
static sym_tb *alloc_sym()
{
	//分配链表结构
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
	node_num++; //节点个数
	return tb;

}
//释放所有节点
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
	//打开的当前文件
	FILE *fp;
	int file_len;
	int ret;
	char *file_data;
	int line; //行号
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
	if(ret != file_len){ //文本文件为UNIX格式，为DOS格式时候不等
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
		//标识符
		else if((tmp >= 'a' && tmp <= 'z') || (tmp >= 'A' && tmp <= 'Z') || tmp == '_'){
			char *s_head = src-1; //标识符的头
			while ((*src >= 'a' && *src <= 'z') || (*src >= 'A' && *src <= 'Z') || (*src >= '0' && *src <= '9') || *src == '_')
				++src;
			//找到一个标识符
			int s_len = src - s_head; //标识符长度
			sym_tb *sym = alloc_sym();
			sym->sym_type = SYM_ID; //标识符
			sym->sym_name = my_malloc(s_len+1);
			strncpy(sym->sym_name,s_head,s_len );
			sym->sym_name[s_len] = '\0';
			sym->line = line;
		}
		//数字处理
		else if (tmp >= '0' && tmp <= '9') {
			int ival;
		    if (ival = tmp - '0'){ //10进制的数
		    	while (*src >= '0' && *src <= '9')
		    		ival = ival * 10 + *src++ - '0';
		    }
		    else if (*src == 'x' || *src == 'X') { //16进制的数
		        while ((tmp = *++src) && ((tmp >= '0' && tmp <= '9') || (tmp >= 'a' && tmp <= 'f') || (tmp >= 'A' && tmp <= 'F')))
		          ival = ival * 16 + (tmp & 15) + (tmp >= 'A' ? 9 : 0);
		    }
		    else {
		    	while (*src >= '0' && *src <= '7')
		    		ival = ival * 8 + *src++ - '0';
		    }

		    sym_tb *sym = alloc_sym();
		    sym->sym_type = SYM_NUM; //数字
		    sym->value = ival;
		    sym->line = line;
		 }
		 else if (tmp == '/') {
		    if (*src == '/') { //单行注释
		        ++src;
		        while (*src != 0 && *src != '\n')
		        	++src;
		      }
		    else if(*src == '*'){ //多行注释
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
		     else { //除法
		    	 sym_tb *sym = alloc_sym();
		    	 sym->sym_type = SYM_OP; //运算符
		    	 sym->value = OP_DIV; //
		    	 sym->line = line;
		      }
		    }
		 else if (tmp == '\'' || tmp == '"') { //单引号，双引号
		      //pp = data;
			 char *s_head = src;
		      while (*src != 0 && *src != tmp) {
		    	  /*
		        if ((ival = *src++) == '\\') { //转义字符处理
		          if ((ival = *src++) == 'n') ival = '\n';
		        }*/
		    	  src++;
		      }

		      sym_tb *sym = alloc_sym();
		      sym->line = line;
		      if (tmp == '"'){ //字符串
		    	  int src_len = src - s_head;
		    	  sym->sym_type = SYM_STR; //字符串
		    	  sym->sym_name = my_malloc(src_len+1);
		    	  sym->sym_name[src_len] = '\0';
		    	  strncpy(sym->sym_name, s_head, src_len);
		      }else{ // 'A'
		    	  sym->sym_type = 5; //单个字符
		    	  sym->value = *s_head;
		      }
		      src++;
		    }
		 else if (tmp == '=') {
			 sym_tb *sym = alloc_sym();
			 sym->line = line;
			 sym->sym_type = SYM_OP; //运算符

			 if (*src == '=') { //如果是 ==
				 ++src;
				 sym->value = OP_EQ; //==
			 } else
				 sym->value = OP_ASSIGN; //= 赋值
		 }else if (tmp == '+') {
			 sym_tb *sym = alloc_sym();
			 sym->line = line;
			 sym->sym_type = SYM_OP; //运算符
			 if (*src == '+')
			 {
				 ++src;
				 sym->value = OP_INC; //++ ,加一
			 } else
				 sym->value = OP_ADD; //add +=不支持
		 } else if (tmp == '-') {
			 sym_tb *sym = alloc_sym();
			 sym->line = line;
			 sym->sym_type = SYM_OP; //运算符
			 if (*src == '-') {
				 ++src;
				 sym->value = OP_DEC; //--
			 } else
				 sym->value = OP_SUB; //-,
		 } else if (tmp == '!') {
			 sym_tb *sym = alloc_sym();
			 sym->line = line;
			 sym->sym_type = SYM_OP; //运算符
			 sym->value = OP_NE; // !
			 if (*src == '=') {
				 ++src;
				 sym->value = OP_NOT; // !=
			 }
		 }else if (tmp == '<') {
			 sym_tb *sym = alloc_sym();
			 sym->line = line;
			sym->sym_type = SYM_OP; //运算符
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
			 sym->sym_type = SYM_OP; //运算符
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
			sym->sym_type = SYM_OP; //运算符
			 if (*src == '|') {
				 ++src;
				 sym->value = OP_LOR;  // ||
			 } else
				 sym->value = OP_OR; // |
		 }else if (tmp == '&') {
			 sym_tb *sym = alloc_sym();
			 sym->line = line;
			 sym->sym_type = SYM_OP; //运算符

			 if (*src == '&') {
				 ++src;
				 sym->value = OP_LAN;  // &&
			 } else
				 sym->value = OP_AND;  // &
		 }else if (tmp == '^') {
			 sym_tb *sym = alloc_sym();
			 sym->line = line;
			 sym->sym_type = SYM_OP; //运算符
			 sym->value = OP_XOR;  // ^
		 } else if (tmp == '%') {
			 sym_tb *sym = alloc_sym();
			 sym->line = line;
			 sym->sym_type = SYM_OP; //运算符
			 sym->value = OP_MOD;  // %
		 }else if (tmp == '*') {
			 sym_tb *sym = alloc_sym();
			 sym->line = line;
			 sym->sym_type = SYM_OP; //运算符
			 sym->value = OP_MUL;  // *
		 } else if (tmp == '[') {
			 sym_tb *sym = alloc_sym();
			 sym->line = line;
			 sym->sym_type = SYM_LBR; //左中括号
		 }else if (tmp == '?') {
			 sym_tb *sym = alloc_sym();
			 sym->line = line;
			 sym->sym_type = SYM_OP; //运算符
			 sym->value = OP_COND;  // ?
		 }else if (tmp == '~'){
			 sym_tb *sym = alloc_sym();
			 sym->line = line;
			 sym->sym_type = SYM_OP; //运算符
			 sym->value = OP_BNO;  // ~
		 }else if(tmp == ';'){
			 sym_tb *sym = alloc_sym();
			 sym->line = line;
			 sym->sym_type = SYM_SEM; //分号 ;
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
			 sym->sym_type = SYM_RBR; //右中括号
		 }else if(tmp == ','){
			 sym_tb *sym = alloc_sym();
			 sym->line = line;
			 sym->sym_type = SYM_COM; //逗号 ，
		 }else if(tmp == ':'){
			 sym_tb *sym = alloc_sym();
			 sym->line = line;
			 sym->sym_type = SYM_COL; //冒号
		 }

	}
	//创建数组，释放节点
	return create_array_fr_node();
}

//释放节点
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

