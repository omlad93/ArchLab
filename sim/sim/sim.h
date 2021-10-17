#pragma once

#include "opp.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>


#define GOOD 0
#define MAX_LINE 12
#define MEM_SIZE 65536

#define OPP_MASK 0x3E000000 //bits 29-25
#define DST_MASK 0x01c00000 //bits 24-22
#define SR0_MASK 0x00380000 //bits 21-19
#define SR1_MASK 0x00070000 //bits 18-16
#define IMM_MASK 0x0000FFFF //bits 15-0

#define OPP_SHFT 25
#define DST_SHFT 22
#define SR0_SHFT 19
#define SR1_SHFT 16


FILE *Sram_in;	//input
FILE *Sram_out; //output

int pc = 0; //trivial
int REG[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
char MEM[MEM_SIZE][9];


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

