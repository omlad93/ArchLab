#define _CRT_SECURE_NO_DEPRECATE
#include <stdio.h>

#define LOW_MASK = 0x0000FFFF
#define HIG_SHFT = 16

extern int   REG[8];
extern char* MEM[65536][5];

/* **********************************************************/
/*  ~~~~~~~~~~~~~~~    OPERATION  STRUCT    ~~~~~~~~~~~~~~  */
/* **********************************************************/

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


/* **********************************************************/
/*  ~~~~~~~~~~~~~    FUNCTION DECLARATIONS    ~~~~~~~~~~~~  */
/* **********************************************************/

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


/* **********************************************************/
/*  ~~~~~~~~~~~~~    SIMP OP CODES OPERATIONS ~~~~~~~~~~~~  */
/* **********************************************************/
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

