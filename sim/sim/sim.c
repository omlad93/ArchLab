#include "sim.h"

/* *********************************************************/
/*  ~~~~~~~~~~~~~~~     FILE   PARSING     ~~~~~~~~~~~~~~  */
/* *********************************************************/

/*
write trace file without exection line
*/
int write_trace_file(operation* op, int pc, int op_count, FILE* trace) {

	fprintf(trace, "--- instruction %i (%04x) @ PC %i (%04x) -----------------------------------------------------------\n", op_count, op_count, pc, pc);
	fprintf(trace, "pc = %04i, inst = %s, opcode = %i (%s), dst = %i, src0 = %i, src1 = %i, immediate = %08x\n",
					pc, op->inst, op->op_num, op->op_name, op->rd, op->src0, op->src1, REG[1]);
	fprintf(trace, "r[0] = 00000000 r[1] = %08x r[2] = %08x r[3] = %08x \nr[4] = %08x r[5] = %08x r[6] = %08x r[7] = %08x \n\n",
					 REG[1], REG[2], REG[3], REG[4], REG[5], REG[6], REG[7]);


	return GOOD;
}

/*
write to output what opcode has been executed. 
*/
void print_exec_line(int pc, operation* op, FILE* file) {
	switch (op->op_num) {
	case(0):
		fprintf(file, ">>>> EXEC: R[%i] = %i %s %i <<<<\n\n", op->rd REG[op->src0], op->op_name, REG[op->src1]);
		return;
	case(1):
		fprintf(file, ">>>> EXEC: R[%i] = %i %s %i <<<<\n\n", op->rd REG[op->src0], op->op_name, REG[op->src1]);
		return;
	case(2):
		fprintf(file, ">>>> EXEC: R[%i] = %i %s %i <<<<\n\n", op->rd REG[op->src0], op->op_name, REG[op->src1]);
		return;
	case(3):
		fprintf(file, ">>>> EXEC: R[%i] = %i %s %i <<<<\n\n", op->rd REG[op->src0], op->op_name, REG[op->src1]);
		return;
	case(4):
		fprintf(file, ">>>> EXEC: R[%i] = %i %s %i <<<<\n\n", op->rd REG[op->src0], op->op_name, REG[op->src1]);
		return;
	case(5):
		fprintf(file, ">>>> EXEC: R[%i] = %i %s %i <<<<\n\n", op->rd REG[op->src0], op->op_name, REG[op->src1]);
		return;
	case(6):
		fprintf(file, ">>>> EXEC: R[%i] = %i %s %i <<<<\n\n", op->rd REG[op->src0], op->op_name, REG[op->src1]);
		return;
	case(7):
		fprintf(file, ">>>> EXEC: R[%i] = %i %s %i <<<<\n\n", op->rd REG[op->src0], op->op_name, REG[op->src1]);
		return;
	case(8):
		fprintf(file, ">>>> EXEC: R[%i] = MEM[%i] = %08i <<<<\n\n", op->rd, REG[src1], REG[1]);
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
		fprintf(file, ">>>> EXEC: HALT at PC %04x<<<<\n", pc);
		return;
	default:
		fprintf(file, "");
		return;
}


void print_mem_file(FILE* fd) {
	int j;
	for (j = 0; j < MEM_SIZE; j++) {
		fprintf(fd, "%08X\n", MEM[j]);
	}
}

/*
A function to parse operation from instruction memory aka IMEM
*/
void parse_opcode(char* line, operation* operation, int pc) {
	int rd, rs, src1, op, im;
	int parsed_line = strtol(&line[0], NULL, 16);

	op = parsed_line & OPP_MASK >> OPP_SHFT;
	rd = parsed_line & DST_MASK >> DST_SHFT;
	src0 = parsed_line & SR0_MASK >> SR0_SHFT;
	src1 = parsed_line & SR1_MASK >> SR1_SHFT;
	im =  parsed_line & IMM_MASK >> IMM_MASK
	REG[1] = imm
	set_operation(operation, rd, src1, src0, op, line);
}


/* *********************************************************/
/*  ~~~~~~~~~~~~~~~          MAIN          ~~~~~~~~~~~~~~  */
/* *********************************************************/


int main(int argc, char* argv[]) {
	int j = 0;
	FILE *input, *trace, *sram_out;
	char* line = NULL;
	char read_line[MAX_LINE];

	input = fopen(argv[1],'r');
	trace = fopen('trace.txt','w');
	sram_out = fopen('sram_out.txt','w')

	if (input == NULL || trace == NULL || sram_out == NULL ){
		printf( 'Error opening files');
		exit(3);
	}

	operation* op = (operation*)calloc(1, sizeof(op));
	if (!op) {
		printf("error allocating operation struct");
		exit(1);
	}

	while (!feof(input)) {
		fgets(read_line, MAX_LINE, input);
		read_line[8] = '\0';
		MEM[j] = read_line;
		j++;
	}
	while (j < MEM_SIZE) {
		MEM[j] = '00000000';
		j++;
	}

	printf("\n\tStarting Operation sequance.\n\t");

	while ((-1 < pc) && (pc <= MEM_SIZE)) {

		line = MEM[pc];					 //  get line to parse & operate
		parse_opcode(line, op, pc);
		write_trace_file(op, pc, op_count, trace);
		pc = (op->op_code)(op, pc);
		print_exec_line(pc,op, trace);
		op_count++;
		
	}
	print_mem_file(sram_out);
	fclose(sram_out);
	fclose(input);
	fclose(trace);
	printf("\n\n\tSimulator finished running. [v] \n ");

	return GOOD;


}

