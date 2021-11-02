/*
 * SP ASM: Simple Processor assembler
 *
 * usage: asm
 */
#include <stdio.h>
#include <stdlib.h>

#define ADD 0
#define SUB 1
#define LSF 2
#define RSF 3
#define AND 4
#define OR  5
#define XOR 6
#define LHI 7
#define LD 8
#define ST 9
#define CMB 10 // Copy Memory Background
#define POL 11 // Poll DMA transaction
#define JLT 16
#define JLE 17
#define JEQ 18
#define JNE 19
#define JIN 20
#define HLT 24

#define MEM_SIZE_BITS	(16)
#define MEM_SIZE	(1 << MEM_SIZE_BITS)
#define MEM_MASK	(MEM_SIZE - 1)
unsigned int mem[MEM_SIZE];

int pc = 0;

static void asm_cmd(int opcode, int dst, int src0, int src1, int immediate)
{
	int inst;

	inst = ((opcode & 0x1f) << 25) | ((dst & 7) << 22) | ((src0 & 7) << 19) | ((src1 & 7) << 16) | (immediate & 0xffff);
	mem[pc++] = inst;
}

static void assemble_program(char *program_name)
{
	FILE *fp;
	int addr, i, last_addr;

	for (addr = 0; addr < MEM_SIZE; addr++)
		mem[addr] = 0;

	pc = 0;

	/*
	 * DMA TEST FUNCTION
	 */

	asm_cmd(ADD, 3, 0, 1,100);// 0: R3 = 100
	asm_cmd(ADD, 2, 0, 1,0);// 1: R2 = 0
	asm_cmd(ADD, 6, 0, 1,2);// 2: R6 = 2
	asm_cmd(LD, 4, 0, 3,0);// 3: R4 = MEM[R3]
	asm_cmd(ADD, 5, 0,1,1000);// 4: R5 = 1000
	asm_cmd(CMB, 0, 5, 4,10);// 5: copy the content (size 2) from adress 1000 to adress that was stored in MEM[100] 
	asm_cmd(POL, 6, 0, 0,0);// 6: poll the DMA status into R6 
	asm_cmd(ST, 0, 6, 3,0);// 7: MEM[R3] = R6
	asm_cmd(JNE, 0, 2, 6,6);// 9: if R6 != 0 jump to pc 6
	asm_cmd(HLT, 0, 0, 0,0);// 9: HALT

	
	/* 
	 * Constants are planted into the memory somewhere after the program code:
	 */

	mem[100] = 200;
	mem[999] = 1;
	mem[1000] = 2;
	mem[1001] = 3;
	mem[1002] = 4;
	mem[1003] = 5;
	mem[1004] = 6;
	mem[1005] = 7;
	mem[1006] = 8;


	last_addr = 1010;

	fp = fopen(program_name, "w");
	if (fp == NULL) {
		printf("couldn't open file %s\n", program_name);
		exit(1);
	}
	addr = 0;
	while (addr < last_addr) {
		fprintf(fp, "%08x\n", mem[addr]);
		addr++;
	}
}


int main(int argc, char *argv[])
{
	
	if (argc != 2){
		printf("usage: asm program_name\n");
		return -1;
	}else{
		assemble_program(argv[1]);
		printf("SP assembler generated machine code and saved it as %s\n", argv[1]);
		return 0;
	}
	
}
