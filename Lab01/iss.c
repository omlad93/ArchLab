#include "iss.h"


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
void set_operation(operation* op, int d, int t, int s, int code, char* inst, int pc) {
	op->rd = d;
	op->src1 = t;
	op->src0 = s;
	op->op_num = code;
	op->op_code = set_op_by_code(code, op);
	op->inst = inst;
	op->prev_pc = pc;
	set_op_name_by_code(code,op);

}


/* Op - Codes Functions */

//0
int add(operation* op, int pc) {
	REG[op->rd] = REG[op->src0] + REG[op->src1];
	return pc + 1;

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
	REG[op->rd] = MEM[REG[op->src1]];
	return (pc + 1);
}

//9
int st(operation* op, int pc) {
		MEM[REG[op->src1]] = REG[op->src0];
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
	// printf("\n\t > reached halt function [pc=%d]\n\t", pc);
	op->prev_pc = pc;
	return -1;
}

//assign to $0
int nop(operation*op, int pc) {
	op -> prev_pc = pc;
	return pc + 1;
}


/*
write trace file without exection line
*/
int write_trace_file(operation* op, int pc, int op_count, FILE* trace) {
	int x =0;
	x = fprintf(trace, "--- instruction %i (%04x) @ PC %i (%04x) -----------------------------------------------------------\n",
					op_count, op_count, pc, pc);

	x |= fprintf(trace, "pc = %04i, inst = %s, opcode = %i (%s), dst = %i, src0 = %i, src1 = %i, immediate = %08x\n",
					pc, op->inst, op->op_num, op->op_name, op->rd, op->src0, op->src1, REG[1]);
	x |= fprintf(trace, "r[0] = 00000000 r[1] = %08x r[2] = %08x r[3] = %08x \nr[4] = %08x r[5] = %08x r[6] = %08x r[7] = %08x \n\n",
					 REG[1], REG[2], REG[3], REG[4], REG[5], REG[6], REG[7]);
	if (x==0){
		printf("(X)");
		return BAD;
	}
	return GOOD;
}

/*
write to output what opcode has been executed. 
*/
void print_exec_line(int pc, operation* op, FILE* file, int op_count) {
	switch (op->op_num) {
	case(0):
		fprintf(file, ">>>> EXEC: R[%i] = %i %s %i <<<<\n\n", op->rd, REG[op->src0], op->op_name, REG[op->src1]);
		return;
	case(1):
		fprintf(file, ">>>> EXEC: R[%i] = %i %s %i <<<<\n\n", op->rd, REG[op->src0], op->op_name, REG[op->src1]);
		return;
	case(2):
		fprintf(file, ">>>> EXEC: R[%i] = %i %s %i <<<<\n\n", op->rd, REG[op->src0], op->op_name, REG[op->src1]);
		return;
	case(3):
		fprintf(file, ">>>> EXEC: R[%i] = %i %s %i <<<<\n\n", op->rd, REG[op->src0], op->op_name, REG[op->src1]);
		return;
	case(4):
		fprintf(file, ">>>> EXEC: R[%i] = %i %s %i <<<<\n\n", op->rd, REG[op->src0], op->op_name, REG[op->src1]);
		return;
	case(5):
		fprintf(file, ">>>> EXEC: R[%i] = %i %s %i <<<<\n\n", op->rd, REG[op->src0], op->op_name, REG[op->src1]);
		return;
	case(6):
		fprintf(file, ">>>> EXEC: R[%i] = %i %s %i <<<<\n\n", op->rd, REG[op->src0], op->op_name, REG[op->src1]);
		return;
	case(7):
		fprintf(file, ">>>> EXEC: R[%i] = %i %s %i <<<<\n\n", op->rd, REG[op->src0], op->op_name, REG[op->src1]);
		return;
	case(8):
		fprintf(file, ">>>> EXEC: R[%i] = MEM[%i] = %08i <<<<\n\n", op->rd, REG[op->src1], MEM[REG[op->src1]]);
		return;
	case(9):
		fprintf(file, ">>>> EXEC: MEM[%i] = R[%i] = %08x <<<<\n\n", REG[op->src1], op->src0, REG[op->src0]);
		return;
	case(16):
		fprintf(file, ">>>> EXEC: %s %i, %i, %i <<<<\n\n", op->op_name, REG[op->src0], REG[op->src1], pc);
		return;
	case(17):
		fprintf(file, ">>>> EXEC: %s %i, %i, %i <<<<\n\n", op->op_name, REG[op->src0], REG[op->src1], pc);
		return;
	case(18):
		fprintf(file, ">>>> EXEC: %s %i, %i, %i <<<<\n\n", op->op_name, REG[op->src0], REG[op->src1], pc);
		return;
	case(19):
		fprintf(file, ">>>> EXEC: %s %i, %i, %i <<<<\n\n", op->op_name, REG[op->src0], REG[op->src1], pc);
		return;
	case(20):
		fprintf(file, ">>>> EXEC: %s %i, %i, %i <<<<\n\n", op->op_name, REG[op->src0], REG[op->src1], pc);
		return;
	case(24):
		fprintf(file, ">>>> EXEC: HALT at PC %04x<<<<\n", op->prev_pc);
		fprintf(file,"sim finished at pc %i, %i instructions", op->prev_pc, op_count);

		return;
	default:
		fprintf(file, ">>>> EXEC: NOP at PC %04x<<<<\n\n", pc);
		return;
	}
}


void print_mem_file(FILE* fd) {
	int j;
	for (j = 0; j < MEM_SIZE; j++) {
		fprintf(fd, "%08x\n", MEM[j]);
	}
}

/*
A function to parse operation from instruction memory aka IMEM
*/
void parse_opcode(char* line, operation* operation, int pc) {
	int rd, src0, src1, op, im;
	int parsed_line = (int)strtol(&line[0], NULL, 16);
	op = (parsed_line & OPP_MASK) >> OPP_SHFT;
	rd = (parsed_line & DST_MASK) >> DST_SHFT;
	src0 = (parsed_line & SR0_MASK) >> SR0_SHFT;
	src1 = (parsed_line & SR1_MASK) >> SR1_SHFT;
	im =  (parsed_line & IMM_MASK);
	REG[1] = im;
	set_operation(operation, rd, src1, src0, op, line, pc);
}


/* *********************************************************/
/*  ~~~~~~~~~~~~~~~          MAIN          ~~~~~~~~~~~~~~  */
/* *********************************************************/


int main(int argc, char* argv[]) {
	int j = 0, op_count=0;
	FILE *input, *trace, *sram_out;
	char line[MAX_LINE];
	// char read_line[MAX_LINE];

	printf("\tRunning Simulator(){");

	input = fopen(argv[1],"r");
	sram_out = fopen("sram_out.txt","w");
	trace = fopen("trace.txt","w");


	if ((argc != 2) || (input == NULL) || (trace == NULL) || (sram_out) == NULL){
		printf( "Error opening files");
		exit(3);
	}

	operation* op = (operation*)calloc(1, sizeof(op));
	if (!op) {
		printf("error allocating operation struct");
		exit(1);
	}
	printf("\n\t 1) Reading SRAM.");

	while (!feof(input)) {
		fscanf(input, "%08x",&MEM[j]);
		// fgets(read_line, MAX_LINE, input);
		// read_line[8] = '\0';
		// strcpy(MEM[j], read_line);
		j++;
	}
	while (j < MEM_SIZE) {
		MEM[j] = 0;
		j++;
	}



	printf("\n\t 2) Starting Operation sequance.");
	while ((-1 < pc) && (pc <= MEM_SIZE)) {
		sprintf(line, "%08x", MEM[pc]); //  get line to parse & operate
		parse_opcode(line, op, pc);
		write_trace_file(op, pc, op_count, trace);
		pc = (op->op_code)(op, pc);
		print_exec_line(pc,op, trace, op_count);
		op_count++;
	}
	printf("\n\t 3) Operation sequance Finished: with %s @ %i after %i operations.",op->op_name, pc, op_count-1);
	print_mem_file(sram_out);
	printf("\n\t 4) File are Written.");

	printf("\n\n\t >>> %i x %i = %i", MEM[1000], MEM[1001], MEM[1002]);

	return GOOD;


}

