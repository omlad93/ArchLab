#define _CRT_SECURE_NO_DEPRECATE
#include "opp.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#define GOOD 0
#define MAX_LINE 12
#define MEM_SIZE 65536
#define HIG_SHFT = 16
#define OPP_SHFT 25
#define DST_SHFT 22
#define SR0_SHFT 19
#define SR1_SHFT 16
#define OPP_MASK 0x3E000000 //bits 29-25
#define DST_MASK 0x01c00000 //bits 24-22
#define SR0_MASK 0x00380000 //bits 21-19
#define SR1_MASK 0x00070000 //bits 18-16
#define IMM_MASK 0x0000FFFF //bits 15-0
#define LOW_MASK = 0x0000FFFF

int pc = 0; //trivial
int REG[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
char MEM[MEM_SIZE][9];


typedef struct operation {
	int rd; //rd register
	int src0; //src0 register
	int src1; //src1 register
	int op_num; // number of op_code
	char* op_name; //name of op_code
	int imm_used; // and indicator of using immediate
	char* inst; // the opcode line from Imem
	int(*op_code)(struct operation* op, int pc); // a function pointer according to opcode
} operation;



/*
set the functions to be performed in the *op_code field in operation struct
*/
void* set_op_by_code(int code, operation* op);

/*
set the function name in operation struct
*/
void set_op_name_by_code(int code, operation* op);

/*
load input values to fields in the operation struct
*/
void set_operation(operation* op, int d, int t, int s, int code, char* inst);

/*
this function has a debug feature comapring trace file written to a given referance trace
if not debug: original_trace=NULL & debug=0
*/
int write_trace_file(operation* op, int pc, int op_count, FILE* trace);
/*
write to output what opcode has been executed. 
*/
void print_exec_line(int pc, operation* op, FILE* file);

/*
A function to pasrc0e operation from instruction memory aka IMEM
*/
void parse_opcode(char* line, operation* op, int pc);


/* OPERATION By OPCODE */

//0
int add(operation* op, int pc);

//1
int sub(operation* op, int pc);

//2
int lsf(operation* op, int pc);

//3
int rsf(operation* op, int pc);

//4
int and(operation* op, int pc);

//5
int or(operation* op, int pc);

//6
int xor (operation* op, int pc);

//7
int lhi(operation* op, int pc);

//8
int ld(operation* op, int pc);

//9
int st(operation* op, int pc);


//16
int jlt(operation* op, int pc);

//17
int jle(operation* op, int pc);

//18
int jeq(operation* op, int pc);

//19
int jne(operation* op, int pc);

//20
int jin(operation* op, int pc);

//24
int halt(operation* op, int pc);

//assign to $0 or unreconized behivour 
int nop(operation*op, int pc);

