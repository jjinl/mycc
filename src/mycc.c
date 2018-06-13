/*
 ============================================================================
 Name        : mycc.c
 Author      : jjinl
 Version     :
 Copyright   : LGPL
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "lex.h"


int main(void) {
	array *arr;
	sym_tb **pdat;
	int i;

	puts("!!!Hello World!!!"); /* prints !!!Hello World!!! */

	arr = lex("sqlite3.txt");
	if(!arr){
		printf("词法分析出错\n");
		return -1;
	}
	pdat = (sym_tb **)arr->data;


	for(i=0;i<arr->len;i++){
		printf("line :%d ",pdat[i]->line);
		switch(pdat[i]->sym_type){
		case SYM_ID:
			printf("id :%s\n",pdat[i]->sym_name);
			break;
		case SYM_NUM:
			printf("val :%d\n",pdat[i]->value);
			break;
		case SYM_OP:
			printf("op :%d\n",pdat[i]->value);
			break;
		case SYM_STR:
			printf("str :%s\n",pdat[i]->sym_name);
			break;
		default:
			printf("不支持的词法分析选项");
		}
	}
	printf("node %d\n",arr->len);

	getchar();
	free_array(arr);
	getchar();
	free(arr);

	return EXIT_SUCCESS;
}
