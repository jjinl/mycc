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
	pn->next = NULL;//新节点下一链必须为NULL
	if(!sym_head){ //第一次初始化
		sym_head = pn; //sym_head指向链表的头
		sym_curr = pn; //sym_curr指向当前的新节点
	}else{
		sym_curr->next = pn;
		sym_curr = pn;
	}

	sym_tb *tb = my_malloc(sizeof(sym_tb));
	sym_curr->data = tb;//分配的数据添加到链表中
	node_num++; //节点个数
	return tb;

}
//释放所有节点
static void free_pnode()
{
	while(sym_head){
		sym_curr = sym_head->next; //保存下一节点的地址
		free(sym_head); //释放头结点
		sym_head = sym_curr;
	}
}
//把链表换成数组便于操作
static array *create_array_fr_node()
{
	int i;
	sym_tb **dat = my_malloc(node_num*sizeof(char *));//分配数组指针

	struct pnode *sym_pos = sym_head;
	array *arr = my_malloc(sizeof(array));

	for(i=0;i<node_num;i++){
		dat[i] = (sym_tb *)(sym_pos->data);
		sym_pos = sym_pos->next;
	}
	free_pnode();
	arr->len = node_num; //数组元素个数
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
	//求文件的长度
	fseek(fp, 0, SEEK_END);
	file_len = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	file_data = my_malloc(file_len+1);
	//把文件全部读入内存，注意 fread返回值为读出的字节数，当文件为DOS格式 file_len != ret,
	//DOS格式 \r\n 中\r被fread滤掉
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
			sym->value = s_len; //id字符串长度
			sym->line = line;
		}
		//数字处理
		else if (tmp >= '0' && tmp <= '9') {
			int ival;
		    if (ival = tmp - '0'){ //10进制的数 // @suppress("Assignment in condition")
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
		    else if(*src == '='){  // /= ;a /= b;
		    	sym_tb *sym = alloc_sym();
		    	sym->sym_type = SYM_OP; //运算符
		    	sym->value = OP_DIV_EQ; // /=
		    	sym->line = line;
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
			 }
			 else if (*src == '=')
			 {
				 ++src;
				 sym->value = OP_ADD_EQ; // +=
			 }
			 else
				 sym->value = OP_ADD; //add +=不支持
		 } else if (tmp == '-') {
			 sym_tb *sym = alloc_sym();
			 sym->line = line;
			 sym->sym_type = SYM_OP; //运算符
			 if (*src == '-') {
				 ++src;
				 sym->value = OP_DEC; //--
			 } else if (*src == '>') { // -> 指针
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
				 if(*src == '='){ // <<=
					 ++src;
					 sym->value = OP_SHL_EQ;
				 }
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
				 if(*src == '='){ // >>=
				 	++src;
				 	sym->value = OP_SHR_EQ;
				 }
			 } else
				 sym->value = OP_GT; // >
		 }else if (tmp == '|') {
			sym_tb *sym = alloc_sym();
			sym->line = line;
			sym->sym_type = SYM_OP; //运算符
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
			 sym->sym_type = SYM_OP; //运算符

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
			 sym->sym_type = SYM_OP; //运算符
			 sym->value = OP_XOR;  // ^
			 if (*src == '=') {
			 	++src;
			 	sym->value = OP_XOR_EQ;  // ^=
			 }
		 } else if (tmp == '%') {
			 sym_tb *sym = alloc_sym();
			 sym->line = line;
			 sym->sym_type = SYM_OP; //运算符
			 sym->value = OP_MOD;  // %
			 if (*src == '=') { // %=
				 ++src;
				sym->value = OP_MOD_EQ;  // %=
			}
		 }else if (tmp == '*') {  // *= ,取指针内容
			 sym_tb *sym = alloc_sym();
			 sym->line = line;
			 sym->sym_type = SYM_OP; //运算符
			 sym->value = OP_MUL;  // *
			 if (*src == '=') {
			 	++src;
			 	sym->value = OP_MUL_EQ;  // *=
			 }
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
		 }else if(tmp == '.'){
			 sym_tb *sym = alloc_sym();
			 sym->line = line;
			 sym->sym_type = SYM_PU; //点号
			 // ... 变参
			 if(*src == '.' && *(src+1) == '.'){
				 src++;
				 src++;
				 sym->sym_type = SYM_ELL; // ... 变参
			 }
		 }else if((tmp == ' ') || (tmp == '\t')){ //空格 越过
			 continue;
		 }else{
			 printf("词法分析错误line: %d  %d \n",line, tmp);
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
		if(pdat[i]->sym_type == SYM_ID){ //把标识符中关键字替换掉
			switch(pdat[i]->value){ //字符串长度
			case 2: //do,if
				if(!strncmp(pdat[i]->sym_name,"do",2)){
					pdat[i]->sym_type = SYM_KW; //关键字
					pdat[i]->value = KW_DO;
					free(pdat[i]->sym_name); //释放空间
					pdat[i]->sym_name = NULL;
				}else if(!strncmp(pdat[i]->sym_name,"if",2)){
					pdat[i]->sym_type = SYM_KW; //关键字
					pdat[i]->value = KW_IF;
					free(pdat[i]->sym_name); //释放空间
					pdat[i]->sym_name = NULL;
				}
				break;
			case 3: //for int
				if(!strncmp(pdat[i]->sym_name,"for",3)){
						pdat[i]->sym_type = SYM_KW; //关键字
						pdat[i]->value = KW_FOR;
						free(pdat[i]->sym_name); //释放空间
						pdat[i]->sym_name = NULL;
				}else if(!strncmp(pdat[i]->sym_name,"int",3)){
						pdat[i]->sym_type = SYM_KW; //关键字
						pdat[i]->value = KW_INT;
						free(pdat[i]->sym_name); //释放空间
						pdat[i]->sym_name = NULL;
				}
				break;
			case 4://auto case char else enum goto long void
				if(!strncmp(pdat[i]->sym_name,"auto",4)){
						pdat[i]->sym_type = SYM_KW; //关键字
						pdat[i]->value = KW_AUTO;
						free(pdat[i]->sym_name); //释放空间
						pdat[i]->sym_name = NULL;
				}else if(!strncmp(pdat[i]->sym_name,"case",4)){
						pdat[i]->sym_type = SYM_KW; //关键字
						pdat[i]->value = KW_CASE;
						free(pdat[i]->sym_name); //释放空间
						pdat[i]->sym_name = NULL;
				}else if(!strncmp(pdat[i]->sym_name,"char",4)){
					pdat[i]->sym_type = SYM_KW; //关键字
					pdat[i]->value = KW_CHAR;
					free(pdat[i]->sym_name); //释放空间
					pdat[i]->sym_name = NULL;
				}else if(!strncmp(pdat[i]->sym_name,"else",4)){
					pdat[i]->sym_type = SYM_KW; //关键字
					pdat[i]->value = KW_ELSE;
					free(pdat[i]->sym_name); //释放空间
					pdat[i]->sym_name = NULL;
				}else if(!strncmp(pdat[i]->sym_name,"enum",4)){
					pdat[i]->sym_type = SYM_KW; //关键字
					pdat[i]->value = KW_ENUM;
					free(pdat[i]->sym_name); //释放空间
					pdat[i]->sym_name = NULL;
				}else if(!strncmp(pdat[i]->sym_name,"goto",4)){
					pdat[i]->sym_type = SYM_KW; //关键字
					pdat[i]->value = KW_GOTO;
					free(pdat[i]->sym_name); //释放空间
					pdat[i]->sym_name = NULL;
				}else if(!strncmp(pdat[i]->sym_name,"long",4)){
					pdat[i]->sym_type = SYM_KW; //关键字
					pdat[i]->value = KW_LONG;
					free(pdat[i]->sym_name); //释放空间
					pdat[i]->sym_name = NULL;
				}else if(!strncmp(pdat[i]->sym_name,"void",4)){
					pdat[i]->sym_type = SYM_KW; //关键字
					pdat[i]->value = KW_VOID;
					free(pdat[i]->sym_name); //释放空间
					pdat[i]->sym_name = NULL;
				}
				break;
			case 5: //break const float short union while _Bool
				if(!strncmp(pdat[i]->sym_name,"break",5)){
					pdat[i]->sym_type = SYM_KW; //关键字
					pdat[i]->value = KW_BREAK;
					free(pdat[i]->sym_name); //释放空间
					pdat[i]->sym_name = NULL;
				}else if(!strncmp(pdat[i]->sym_name,"const",5)){
					pdat[i]->sym_type = SYM_KW; //关键字
					pdat[i]->value = KW_CONST;
					free(pdat[i]->sym_name); //释放空间
					pdat[i]->sym_name = NULL;
				}else if(!strncmp(pdat[i]->sym_name,"float",5)){
					pdat[i]->sym_type = SYM_KW; //关键字
					pdat[i]->value = KW_FLOAT;
					free(pdat[i]->sym_name); //释放空间
					pdat[i]->sym_name = NULL;
				}else if(!strncmp(pdat[i]->sym_name,"short",5)){
					pdat[i]->sym_type = SYM_KW; //关键字
					pdat[i]->value = KW_SHORT;
					free(pdat[i]->sym_name); //释放空间
					pdat[i]->sym_name = NULL;
				}else if(!strncmp(pdat[i]->sym_name,"union",5)){
					pdat[i]->sym_type = SYM_KW; //关键字
					pdat[i]->value = KW_UNION;
					free(pdat[i]->sym_name); //释放空间
					pdat[i]->sym_name = NULL;
				}else if(!strncmp(pdat[i]->sym_name,"while",5)){
					pdat[i]->sym_type = SYM_KW; //关键字
					pdat[i]->value = KW_WHILE;
					free(pdat[i]->sym_name); //释放空间
					pdat[i]->sym_name = NULL;
				}else if(!strncmp(pdat[i]->sym_name,"_Bool",5)){
					pdat[i]->sym_type = SYM_KW; //关键字
					pdat[i]->value = KW_BOOL;
					free(pdat[i]->sym_name); //释放空间
					pdat[i]->sym_name = NULL;
				}
				break;
			case 6://double extern inline return signed sizeof static struct switch
				if(!strncmp(pdat[i]->sym_name,"double",6)){
					pdat[i]->sym_type = SYM_KW; //关键字
					pdat[i]->value = KW_DOUBLE;
					free(pdat[i]->sym_name); //释放空间
					pdat[i]->sym_name = NULL;
				}else if(!strncmp(pdat[i]->sym_name,"extern",6)){
					pdat[i]->sym_type = SYM_KW; //关键字
					pdat[i]->value = KW_EXTERN;
					free(pdat[i]->sym_name); //释放空间
					pdat[i]->sym_name = NULL;
				}else if(!strncmp(pdat[i]->sym_name,"inline",6)){
					pdat[i]->sym_type = SYM_KW; //关键字
					pdat[i]->value = KW_INLINE;
					free(pdat[i]->sym_name); //释放空间
					pdat[i]->sym_name = NULL;
				}else if(!strncmp(pdat[i]->sym_name,"return",6)){
					pdat[i]->sym_type = SYM_KW; //关键字
					pdat[i]->value = KW_RETURN;
					free(pdat[i]->sym_name); //释放空间
					pdat[i]->sym_name = NULL;
				}else if(!strncmp(pdat[i]->sym_name,"signed",6)){
					pdat[i]->sym_type = SYM_KW; //关键字
					pdat[i]->value = KW_SIGNED;
					free(pdat[i]->sym_name); //释放空间
					pdat[i]->sym_name = NULL;
				}else if(!strncmp(pdat[i]->sym_name,"sizeof",6)){
					pdat[i]->sym_type = SYM_KW; //关键字
					pdat[i]->value = KW_SIZEOF;
					free(pdat[i]->sym_name); //释放空间
					pdat[i]->sym_name = NULL;
				}else if(!strncmp(pdat[i]->sym_name,"static",6)){
					pdat[i]->sym_type = SYM_KW; //关键字
					pdat[i]->value = KW_STATIC;
					free(pdat[i]->sym_name); //释放空间
					pdat[i]->sym_name = NULL;
				}else if(!strncmp(pdat[i]->sym_name,"struct",6)){
					pdat[i]->sym_type = SYM_KW; //关键字
					pdat[i]->value = KW_STRUCT;
					free(pdat[i]->sym_name); //释放空间
					pdat[i]->sym_name = NULL;
				}else if(!strncmp(pdat[i]->sym_name,"switch",6)){
					pdat[i]->sym_type = SYM_KW; //关键字
					pdat[i]->value = KW_SWITCH;
					free(pdat[i]->sym_name); //释放空间
					pdat[i]->sym_name = NULL;
				}
				break;
			case 7: //default typedef
				if(!strncmp(pdat[i]->sym_name,"default",7)){
					pdat[i]->sym_type = SYM_KW; //关键字
					pdat[i]->value = KW_DEFAULT;
					free(pdat[i]->sym_name); //释放空间
					pdat[i]->sym_name = NULL;
				}else if(!strncmp(pdat[i]->sym_name,"typedef",7)){
					pdat[i]->sym_type = SYM_KW; //关键字
					pdat[i]->value = KW_TYPEDEF;
					free(pdat[i]->sym_name); //释放空间
					pdat[i]->sym_name = NULL;
				}
				break;
			case 8://continue register restrict unsigned volatile _Complex
				if(!strncmp(pdat[i]->sym_name,"continue",8)){
					pdat[i]->sym_type = SYM_KW; //关键字
					pdat[i]->value = KW_CONTINUE;
					free(pdat[i]->sym_name); //释放空间
					pdat[i]->sym_name = NULL;
				}else if(!strncmp(pdat[i]->sym_name,"register",8)){
					pdat[i]->sym_type = SYM_KW; //关键字
					pdat[i]->value = KW_REGISTER;
					free(pdat[i]->sym_name); //释放空间
					pdat[i]->sym_name = NULL;
				}else if(!strncmp(pdat[i]->sym_name,"restrict",8)){
					pdat[i]->sym_type = SYM_KW; //关键字
					pdat[i]->value = KW_RESTRICT;
					free(pdat[i]->sym_name); //释放空间
					pdat[i]->sym_name = NULL;
				}else if(!strncmp(pdat[i]->sym_name,"unsigned",8)){
					pdat[i]->sym_type = SYM_KW; //关键字
					pdat[i]->value = KW_UNSIGNED;
					free(pdat[i]->sym_name); //释放空间
					pdat[i]->sym_name = NULL;
				}else if(!strncmp(pdat[i]->sym_name,"volatile",8)){
					pdat[i]->sym_type = SYM_KW; //关键字
					pdat[i]->value = KW_VOLATILE;
					free(pdat[i]->sym_name); //释放空间
					pdat[i]->sym_name = NULL;
				}else if(!strncmp(pdat[i]->sym_name,"_Complex",8)){
					pdat[i]->sym_type = SYM_KW; //关键字
					pdat[i]->value = KW_COMPLEX;
					free(pdat[i]->sym_name); //释放空间
					pdat[i]->sym_name = NULL;
				}
				//内置宏
				else if(!strncmp(pdat[i]->sym_name,"__LINE__",8)){
					pdat[i]->sym_type = SYM_NUM; //数字
					pdat[i]->value = pdat[i]->line;
					free(pdat[i]->sym_name); //释放空间
					pdat[i]->sym_name = NULL;
				}
				break;
			case 10: //_Imaginary
				if(!strncmp(pdat[i]->sym_name,"_Imaginary",10)){
					pdat[i]->sym_type = SYM_KW; //关键字
					pdat[i]->value = KW_IMAG;
					free(pdat[i]->sym_name); //释放空间
					pdat[i]->sym_name = NULL;
				}
				break;
		//	default:
		//		printf();
			}

		}

	}
}

//语法分析
int yacc(array *arr)
{

	int *sym_stack; //语法分析栈，存储数组的序号
	int i,n;
	sym_tb **pdat;

	if(!arr){
		printf("arr is NULL\n");
		return -1;
	}

	n = arr->len;
	pdat = (sym_tb **)arr->data;

	sym_stack = my_mallo(1024);//栈的深度

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
