#include "opp.h"

/*
this module implemnts the header opp.c

you can find in this nodule 22 functions which can be pointed by the operatin DS:
1 for each op from the ISA and additional no-op for unreconized op_codes or assignment to $0
the opcode number is commented above the function

OUT & IN operation also simulate IO behivour
*/

/*
set the functions to be performed in the *op_code field in operation struct
*/
void* set_op_by_code(int code, operation* op) {

	if (code <=8 && op->rd == 0) {
		return &nop;
	}
	//else go by opcode
	switch (code) {
	case(0):
		return &add;
	case(1):
		return &sub;
	case(2):
		return &lsf;
	case(3):
		return &rsf;
	case(4):
		return &and;
	case(5):
		return &or;
	case(6):
		return &xor;
	case(7):
		return &lhi;
	case(8):
		return &ld;
	case(9):
		return &st;
	case(16):
		return &jlt;
	case(17):
		return &jle;
	case(18):
		return &jeq;
	case(19):
		return &jne;
	case(20):
		return &jin;
	case(24):
		return &halt;
	default:
		return &nop;
	}
}

/*
set the function name in operation struct
*/
void set_op_name_by_code(int code, operation* op) {
	//else go by opcode
	switch (code) {
	case(0):
		op->op_name = "ADD";
		return;
	case(1):
		op->op_name = "SUB";
		return;
	case(2):
		op->op_name = "LSF";
		return;
	case(3):
		op->op_name = "RSF";
		return;
	case(4):
		op->op_name = "AND";
		return;
	case(5):
		op->op_name = "OR";
		return;
	case(6):
		op->op_name = "XOR";
		return;
	case(7):
		op->op_name = "LHI";
		return;
	case(8):
		op->op_name = "LD";
		return;
	case(9):
		op->op_name = "ST";
		return;
	case(16):
		op->op_name = "JLT";
		return;
	case(17):
		op->op_name = "JLE";
		return;
	case(18):
		op->op_name = "JEQ";
		return;
	case(19):
		op->op_name = "JNE";
		return;
	case(20):
		op->op_name = "JIN";
		return;
	case(24):
		op->op_name = "HLT";
		return;
	default:
		op->op_name = "NOP";
	}
}


/*
load input values to fields in the operation struct
*/
void set_operation(operation* op, int d, int t, int s, int code, char* inst) {
	op->rd = d;
	op->src1 = t;
	op->src0 = s;
	op->op_num = code;
	op->op_code = set_op_by_code(code, op);
	op->inst = inst;
	op->imm_used = imm_usage(op);
	set_op_name_by_code(code,op);

}


/* **********************************************************/
/*  ~~~~~~~~~~~~~    SIMP OP CODES OPERATIONS ~~~~~~~~~~~~  */
/* **********************************************************/
//0
int add(operation* op, int pc) {
	REG[op->rd] = REG[op->src0] + REG[op->src1];
	return (pc + 1);

}

//1
int sub(operation* op, int pc) {
	REG[op->rd] = REG[op->src0] - REG[op->src1];
	return (pc + 1);

}

//2
int lsf(operation* op, int pc) {
	REG[op->rd] = REG[op->src0] << REG[op->src1];
	return (pc + 1);
}

//3
int rsf (operation* op, int pc) {
	REG[op->rd] = REG[op->src0] >> REG[op->src1];
	return (pc + 1);
}

//4
int and (operation* op, int pc) {
	REG[op->rd] = REG[op->src0] & REG[op->src1];
	return (pc + 1);

}

//5
int or(operation* op, int pc) {
	REG[op->rd] = REG[op->src0] | REG[op->src1];
	return (pc + 1);
}

//6
int xor(operation* op, int pc) {
	REG[op->rd] = REG[op->src0] ^ REG[op->src1];
	return (pc + 1);
}

//7
int lhi(operation* op, int pc) {
	int low_bits = op->rd && LOW_MASK;
	int shifted_imm = REG[1] << HIG_SHFT;
	REG[op->rd] = low_bits | shifted_imm;
	return (pc + 1);
}

//8
int ld(operation* op, int pc) {
	REG[op->rd] = MEM[REG[op->src1]]];
	return (pc + 1);
}

//9
int st(operation* op, int pc) {
		MEM[REG[op->src1]] = REG[op->src0]
		return (pc + 1);
}


//16
int jlt(operation* op, int pc) {
	if (REG[op->src0] < REG[op->src1]) {
		REG[7] = pc;
		return REG[1];
	}
		return (pc + 1);
}

//17
int jle(operation* op, int pc) {
	if (REG[op->src0] <= REG[op->src1]) {
		REG[7] = pc;
		return REG[1];
	}
		return (pc + 1);
}

//18
int jeq(operation* op, int pc) {
	if (REG[op->src0] == REG[op->src1]) {
		REG[7] = pc;
		return REG[1];
	}
		return (pc + 1);
}

//19
int jne(operation* op, int pc) {
	if (REG[op->src0] != REG[op->src1]) {
		REG[7] = pc;
		return REG[1];
	}
		return (pc + 1);
}

//20
int jin(operation* op, int pc) {
	REG[7] = pc;
	return REG[op->src0];
}


//24
int halt(operation* op, int pc) {
	printf("  > reached halt function [pc=%d]\n\t", pc);
	return -1;
}

//assign to $0
int nop(operation*op, int pc) {
	return pc + 1;
}

