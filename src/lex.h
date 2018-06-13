/*
 * lex.h
 *
 *  Created on: 2018��6��12��
 *      Author: Administrator
 */

#ifndef LEX_H_
#define LEX_H_

//���ű����� ��Ӧsym_type
#define SYM_OP   1   //�����
#define SYM_ID   2   //��ʶ��
#define SYM_NUM  3   //����
#define SYM_STR  4   //�ַ���
#define SYM_SEM  5   //�ֺ� ;
#define SYM_COM  6   //���� ,
#define SYM_COL  7   //ð�� :
#define SYM_LBR  8   //��������
#define SYM_RBR  9   //��������
#define SYM_RPA  10  //��Բ����)
#define SYM_LPA  11  //��Բ����(
#define SYM_LB   12  //������� {
#define SYM_RB   13  //�Ҵ����� }



//���������
#define OP_ASSIGN 		1   // =
#define OP_COND 		2	// ?
#define OP_LOR 			3	// ||
#define OP_LAN 			4	// &&
#define OP_OR 			5	// |
#define OP_XOR 			6	// ^
#define OP_AND 			7	// &
#define OP_EQ 			8	// ==
#define OP_NE 			9	// !=
#define OP_LT 			10	// <
#define OP_GT 			11	// >
#define OP_LE 			12	// <=
#define OP_GE 			13	// >=
#define OP_SHL 			14	// <<
#define OP_SHR 			15	// >>
#define OP_ADD 			16	// +
#define OP_SUB 			17	// -
#define OP_MUL 			18	// *
#define OP_DIV 			19	// /
#define OP_MOD 			20	// %
#define OP_INC 			21	// ++
#define OP_DEC 			22	// --
#define OP_NOT			23	// !
#define OP_BNO			24	// ~
// +=, -=,���﷨�����д���



/*���ű�*/
typedef struct{
	int sym_type; //�������ͣ���ʶ�����ؼ���...
	int line; //�˱�ʶ�������к�

	char *sym_name;

	int value; //��������֣���Ϊ���ֵ�ֵ




}sym_tb;

//����ṹ
typedef struct {
	int len; //����Ԫ�ظ���
	void **data;
}array;

/*���ɷ��ű�����ֵΪһ������ָ�룬���飬
*����
*  array *p = lex(file_name);
*  sym_tb **sym = p->data;
*  for(i=0;i<p->len;i++)
*  		p[i]->sym_type;
*
*/
array* lex(char *filename);

//�ͷŽڵ�
void free_array(array *arr);

#endif /* LEX_H_ */
