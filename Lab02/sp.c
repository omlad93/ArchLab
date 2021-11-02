#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 


#include "llsim.h"

#define sp_printf(a...)						\
	do {							\
		llsim_printf("sp: clock %d: ", llsim->clock);	\
		llsim_printf(a);				\
	} while (0)

#define IMM_MSK 0xFFFF
#define MEM_MSK 65535
#define IMM_SHF 16
#define OPC_MSK 31
#define REG_MSK 7
#define OPC_SHF 25
#define DST_SHF 22
#define SR0_SHF 19
#define SR1_SHF 16



int nr_simulated_instructions = 0;
FILE *inst_trace_fp = NULL, *cycle_trace_fp = NULL;

typedef struct sp_registers_s {
	// 6 32 bit registers (r[0], r[1] don't exist)
	int r[8];

	// 16 bit program counter
	int pc;

	// 32 bit instruction
	int inst;

	// 5 bit opcode
	int opcode;

	// 3 bit destination register index
	int dst;

	// 3 bit source #0 register index
	int src0;

	// 3 bit source #1 register index
	int src1;

	// 32 bit alu #0 operand
	int alu0;

	// 32 bit alu #1 operand
	int alu1;

	// 32 bit alu output
	int aluout;

	// 32 bit immediate field (original 16 bit sign extended)
	int immediate;

	// 32 bit cycle counter
	int cycle_counter;

	// 3 bit control state machine state register
	int ctl_state;

	// control states
	#define CTL_STATE_IDLE		0
	#define CTL_STATE_FETCH0	1
	#define CTL_STATE_FETCH1	2
	#define CTL_STATE_DEC0		3
	#define CTL_STATE_DEC1		4
	#define CTL_STATE_EXEC0		5
	#define CTL_STATE_EXEC1		6
} sp_registers_t;

/*
 * Master structure
 */
typedef struct sp_s {
	// local sram
#define SP_SRAM_HEIGHT	64 * 1024
	llsim_memory_t *sram;

	unsigned int memory_image[SP_SRAM_HEIGHT];
	int memory_image_size;

	sp_registers_t *spro, *sprn;
	
	int start;
} sp_t;

static void sp_reset(sp_t *sp)
{
	sp_registers_t *sprn = sp->sprn;

	memset(sprn, 0, sizeof(*sprn));
}

/*
 * opcodes
 */
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
#define JLT 16
#define JLE 17
#define JEQ 18
#define JNE 19
#define JIN 20
#define HLT 24

static char opcode_name[32][4] = {"ADD", "SUB", "LSF", "RSF", "AND", "OR", "XOR", "LHI",
				 "LD", "ST", "U", "U", "U", "U", "U", "U",
				 "JLT", "JLE", "JEQ", "JNE", "JIN", "U", "U", "U",
				 "HLT", "U", "U", "U", "U", "U", "U", "U"};



static void dump_sram(sp_t *sp){
	FILE *fp;
	int i;

	fp = fopen("sram_out.txt", "w");
	if (fp == NULL) {
                printf("couldn't open file sram_out.txt\n");
                exit(1);
	}
	for (i = 0; i < SP_SRAM_HEIGHT; i++)
		fprintf(fp, "%08x\n", llsim_mem_extract(sp->sram, i, 31, 0));
	fclose(fp);
}

/* Op - Codes Functions */

void add_ex0(sp_registers_t *spro, sp_registers_t *sprn) {
	sprn->aluout = spro->alu0 + spro->alu1;
}

void sub_ex0(sp_registers_t *spro, sp_registers_t *sprn) {
	sprn->aluout = spro->alu0 - spro->alu1;
}

void lsf_ex0(sp_registers_t *spro, sp_registers_t *sprn) {
	sprn->aluout = spro->alu0 << spro->alu1;
}

void rsf_ex0(sp_registers_t *spro, sp_registers_t *sprn) {
	sprn->aluout = spro->alu0 >> spro->alu1;
}

void and_ex0(sp_registers_t *spro, sp_registers_t *sprn) {
	sprn->aluout = spro->alu1 & spro->alu0;
}

void or_ex0(sp_registers_t *spro, sp_registers_t *sprn) {
	sprn->aluout = spro->alu1 | spro->alu0;
}

void xor_ex0(sp_registers_t *spro, sp_registers_t *sprn) {
	sprn->aluout = spro->alu1 ^ spro->alu0;
}

void lhi_ex0(sp_registers_t *spro, sp_registers_t *sprn) {
	sprn->aluout = (spro->alu0 & IMM_MSK) | (spro->alu1 << IMM_SHF);
}

void ld_ex0(sp_t* sp, sp_registers_t* spro) {
	llsim_mem_read(sp->sram, spro->alu1);
}

void st_ex0(sp_registers_t* sprn) {
	return;
}

void jlt_ex0(sp_registers_t *spro, sp_registers_t *sprn) {
	sprn -> aluout = (spro->alu0 < spro->alu1) ? 1 : 0;
}

void jle_ex0(sp_registers_t *spro, sp_registers_t *sprn) {
	sprn -> aluout = (spro->alu0 <= spro->alu1) ? 1 : 0;
}

void jne_ex0(sp_registers_t *spro, sp_registers_t *sprn) {
	sprn -> aluout = (spro->alu0 != spro->alu1) ? 1 : 0;
}

void jeq_ex0(sp_registers_t *spro, sp_registers_t *sprn) {
	sprn -> aluout = (spro->alu0 == spro->alu1) ? 1 : 0;
}

void jin_ex0(sp_registers_t *spro, sp_registers_t *sprn) {
	sprn -> aluout = 1;
}

void halt_ex0() {
	return;
	}

int jumps_ex1(sp_registers_t *spro, sp_registers_t *sprn){
	if (spro->aluout == 1){
		sprn->pc = spro->immediate;
		sprn->r[7] = spro->pc; //Saving the pc from the jump to r[7]
	}
	sprn->ctl_state = CTL_STATE_FETCH0; // update machine state
	return (spro->aluout == 1);

}

int mem_ex1(sp_registers_t *spro, sp_registers_t *sprn, sp_t* sp ){
	int dataout = 0;
			if (spro->opcode == LD){
				dataout = llsim_mem_extract_dataout(sp->sram, 31, 0);
				sprn->r[spro->dst] = dataout;
			}
			else if (spro->opcode == ST){
                llsim_mem_set_datain(sp->sram, spro->alu0, 31, 0);
                llsim_mem_write(sp->sram, spro->alu1);
            } 
	sprn->ctl_state = CTL_STATE_FETCH0; // update machine state
	return dataout;
}

void arithmetics_ex1(sp_registers_t *spro, sp_registers_t *sprn){
	sprn->r[spro->dst] = spro->aluout;
	sprn->ctl_state = CTL_STATE_FETCH0; // update machine state
}

void halt_ex1(sp_t *sp){
	sp_registers_t *sprn = sp->sprn;
	sp->start = 0;
    sprn->ctl_state = CTL_STATE_IDLE;
    dump_sram(sp);
    llsim_stop();
}

void print_trace_file(FILE* trace, sp_registers_t *spro, sp_registers_t *sprn, int data_out){
		int is_jump = (spro->opcode == JLT || spro->opcode == JLE || spro->opcode == JEQ || spro->opcode == JNE || spro->opcode == JIN);
		int is_arithmetic = (spro->opcode == ADD || spro->opcode == SUB || spro->opcode == LSF || spro->opcode == RSF || spro->opcode == AND || spro->opcode == OR || spro->opcode == XOR || spro->opcode == LHI);
        fprintf(trace,"--- instruction %i (%04x) @ PC %i (%04x) -----------------------------------------------------------\n",
				(spro->cycle_counter)/6 -1, (spro->cycle_counter)/6 -1, spro->pc , spro->pc );
        fprintf(trace,"pc = %04d, inst = %08x, opcode = %i (%s), dst = %i, src0 = %i, src1 = %i, immediate = %08x\n",
				spro->pc , spro->inst,  spro->opcode, opcode_name[spro->opcode], spro->dst, spro->src0, spro->src1, spro->immediate);
        fprintf(trace,"r[0] = 00000000 r[1] = %08x r[2] = %08x r[3] = %08x \nr[4] = %08x r[5] = %08x r[6] = %08x r[7] = %08x \n\n", 
				(spro->immediate != 0)?spro->immediate:0 , spro->r[2],spro->r[3], spro->r[4], spro->r[5],spro->r[6], spro->r[7]);
        if (is_arithmetic) {
            fprintf(trace,">>>> EXEC: R[%i] = %i %s %i <<<<\n\n", spro->dst, spro->alu0, opcode_name[spro->opcode], spro->alu1);
        } 
		else if (spro->opcode == LD) 
		{
            fprintf(trace,">>>> EXEC: R[%i] = MEM[%i] = %08x <<<<\n\n", spro->dst, (spro->src1 == 1)?spro->immediate:spro->r[spro->src1], data_out);
        } 
		else if (spro->opcode == ST) 
		{
            fprintf(trace,">>>> EXEC: MEM[%i] = R[%i] = %08x <<<<\n\n", (spro->src1 == 1)?spro->immediate:spro->r[spro->src1], spro->src0, spro->r[spro->src0]);
        } 
		else if (is_jump) 
		{
            fprintf(trace,">>>> EXEC: %s %i, %i, %i <<<<\n\n", opcode_name[spro->opcode], spro->r[spro->src0], spro->r[spro->src1], (spro->aluout == 1)?spro->immediate: spro->pc+1);
        } 
		else if (spro->opcode == HLT) 
		{
            fprintf(trace, ">>>> EXEC: HALT at PC %04x<<<<\n", spro->pc);
            fprintf(trace, "sim finished at pc %i, %i instructions", spro->pc, (spro->cycle_counter)/6);
        }
    }
/*
	Our Addition
*/
static void sp_ctl(sp_t *sp)
{
	sp_registers_t *spro = sp->spro;
	sp_registers_t *sprn = sp->sprn;
	int i;
	int data_out, took_jump;
	int is_jump, is_arithmetic, is_mem;
	is_jump = (spro->opcode == JLT || spro->opcode == JLE || spro->opcode == JEQ || spro->opcode == JNE || spro->opcode == JIN);
	is_arithmetic = (spro->opcode == ADD || spro->opcode == SUB || spro->opcode == LSF || spro->opcode == RSF || spro->opcode == AND || spro->opcode == OR || spro->opcode == XOR || spro->opcode == LHI);
	is_mem =(spro->opcode == LD || spro->opcode == ST);


	// sp_ctl

	fprintf(cycle_trace_fp, "cycle %d\n", spro->cycle_counter);
	for (i = 2; i <= 7; i++){
		fprintf(cycle_trace_fp, "r%d %08x\n", i, spro->r[i]);
		}
	fprintf(cycle_trace_fp, "pc %08x\n", spro->pc);
	fprintf(cycle_trace_fp, "inst %08x\n", spro->inst);
	fprintf(cycle_trace_fp, "opcode %08x\n", spro->opcode);
	fprintf(cycle_trace_fp, "dst %08x\n", spro->dst);
	fprintf(cycle_trace_fp, "src0 %08x\n", spro->src0);
	fprintf(cycle_trace_fp, "src1 %08x\n", spro->src1);
	fprintf(cycle_trace_fp, "immediate %08x\n", spro->immediate);
	fprintf(cycle_trace_fp, "alu0 %08x\n", spro->alu0);
	fprintf(cycle_trace_fp, "alu1 %08x\n", spro->alu1);
	fprintf(cycle_trace_fp, "aluout %08x\n", spro->aluout);
	fprintf(cycle_trace_fp, "cycle_counter %08x\n", spro->cycle_counter);
	fprintf(cycle_trace_fp, "ctl_state %08x\n\n", spro->ctl_state);

	sprn->cycle_counter = spro->cycle_counter + 1;

    switch (spro->ctl_state) {
        case CTL_STATE_IDLE:
            sprn->pc = 0;
            if (sp->start)
                sprn->ctl_state = CTL_STATE_FETCH0;
            break;

        case CTL_STATE_FETCH0:
            llsim_mem_read(sp->sram, spro->pc);
            sprn->ctl_state = CTL_STATE_FETCH1; // update machine state
            break;

        case CTL_STATE_FETCH1:
            sprn->inst = llsim_mem_extract_dataout(sp->sram, 31, 0);
            sprn->ctl_state = CTL_STATE_DEC0; // update machine state
            break;

        case CTL_STATE_DEC0:
            sprn->opcode = (spro->inst >> OPC_SHF) & OPC_MSK;
            sprn->dst = (spro->inst >> DST_SHF) & REG_MSK;
            sprn->src0 = (spro->inst >> SR0_SHF) & REG_MSK;
            sprn->src1 = (spro->inst >> SR1_SHF) & REG_MSK;
            sprn->immediate = spro->inst & IMM_MSK; //check it up
            sprn->ctl_state = CTL_STATE_DEC1; // update machine state
            break;

        case CTL_STATE_DEC1:
            if (spro->opcode == LHI){
                sprn->alu0 = spro->r[spro->dst]; // leave 16 LSBs
                sprn->alu1 = spro->immediate;    // load 16 MSBs
            }
			else {
				sprn->alu0 = (spro->src0 == 0) ? 0 : ((spro->src0 == 1) ? spro->immediate : spro->r[spro->src0]);
				sprn->alu1 = (spro->src1 == 0) ? 0 : ((spro->src1 == 1) ? spro->immediate : spro->r[spro->src1]);
            }
            sprn->ctl_state = CTL_STATE_EXEC0; // update machine state
            break;

        case CTL_STATE_EXEC0: // perform Operation (without updates)
            switch (spro->opcode){ 
                case ADD:
                    add_ex0(spro, sprn);
                    break;
                case SUB:
                    sub_ex0(spro, sprn);
                    break;
				case LSF:
                    lsf_ex0(spro, sprn);
                    break;
				case RSF:
                    rsf_ex0(spro, sprn);
					break;
				case AND:
					and_ex0(spro, sprn);
					break;
				case OR:
					or_ex0(spro, sprn);
					break;
				case XOR:
					xor_ex0(spro, sprn);
					break;
                case LHI:
                    lhi_ex0(spro, sprn);
                    break;
                case LD:
					ld_ex0(sp ,spro);
                    break;
				case ST:
					st_ex0(sprn);
					break;
                case JLT:
                    jlt_ex0(spro, sprn);
                    break;
                case JLE:
					jle_ex0(spro, sprn);
                    break;
				case JNE:
					jne_ex0(spro, sprn);
					break;
                case JEQ:
					jeq_ex0(spro, sprn);
                    break;
                case JIN:
                    jin_ex0(spro, sprn);
                    break;
                case HLT:
               	  	halt_ex0();
                    break;
                default:
                    break;
            }
            sprn->ctl_state = CTL_STATE_EXEC1; // update machine state
            break;

        case CTL_STATE_EXEC1:
            if (spro->opcode == HLT){
				halt_ex1(sp);
				print_trace_file(inst_trace_fp, spro, sprn, data_out);
                break;
            } 
			else if (is_mem){
				data_out = mem_ex1(spro, sprn, sp);
            } 
			else if (is_jump){
				took_jump = jumps_ex1(spro, sprn);
				if (took_jump){
					print_trace_file(inst_trace_fp, spro, sprn, data_out);
					break;
				}
				
			}
			else if (is_arithmetic){
                arithmetics_ex1(spro, sprn);
            } 
            sprn->pc += 1;
			print_trace_file(inst_trace_fp, spro, sprn, data_out);
            break;
    }


}

static void sp_run(llsim_unit_t *unit)
{
	sp_t *sp = (sp_t *) unit->private;

	if (llsim->reset) {
		sp_reset(sp);
		return;
	}

	sp->sram->read = 0;
	sp->sram->write = 0;

	sp_ctl(sp);
}

static void sp_generate_sram_memory_image(sp_t *sp, char *program_name)
{
        FILE *fp;
        int addr, i;

        fp = fopen(program_name, "r");
        if (fp == NULL) {
                printf("couldn't open file %s\n", program_name);
                exit(1);
        }
        addr = 0;
        while (addr < SP_SRAM_HEIGHT) {
                fscanf(fp, "%08x\n", &sp->memory_image[addr]);
                addr++;
                if (feof(fp))
                        break;
        }
	sp->memory_image_size = addr;

        fprintf(inst_trace_fp, "program %s loaded, %d lines\n\n", program_name, addr);        

	for (i = 0; i < sp->memory_image_size; i++)
		llsim_mem_inject(sp->sram, i, sp->memory_image[i], 31, 0);
}

static void sp_register_all_registers(sp_t *sp)
{
	sp_registers_t *spro = sp->spro, *sprn = sp->sprn;

	// registers
	llsim_register_register("sp", "r_0", 32, 0, &spro->r[0], &sprn->r[0]);
	llsim_register_register("sp", "r_1", 32, 0, &spro->r[1], &sprn->r[1]);
	llsim_register_register("sp", "r_2", 32, 0, &spro->r[2], &sprn->r[2]);
	llsim_register_register("sp", "r_3", 32, 0, &spro->r[3], &sprn->r[3]);
	llsim_register_register("sp", "r_4", 32, 0, &spro->r[4], &sprn->r[4]);
	llsim_register_register("sp", "r_5", 32, 0, &spro->r[5], &sprn->r[5]);
	llsim_register_register("sp", "r_6", 32, 0, &spro->r[6], &sprn->r[6]);
	llsim_register_register("sp", "r_7", 32, 0, &spro->r[7], &sprn->r[7]);

	llsim_register_register("sp", "pc", 16, 0, &spro->pc, &sprn->pc);
	llsim_register_register("sp", "inst", 32, 0, &spro->inst, &sprn->inst);
	llsim_register_register("sp", "opcode", 5, 0, &spro->opcode, &sprn->opcode);
	llsim_register_register("sp", "dst", 3, 0, &spro->dst, &sprn->dst);
	llsim_register_register("sp", "src0", 3, 0, &spro->src0, &sprn->src0);
	llsim_register_register("sp", "src1", 3, 0, &spro->src1, &sprn->src1);
	llsim_register_register("sp", "alu0", 32, 0, &spro->alu0, &sprn->alu0);
	llsim_register_register("sp", "alu1", 32, 0, &spro->alu1, &sprn->alu1);
	llsim_register_register("sp", "aluout", 32, 0, &spro->aluout, &sprn->aluout);
	llsim_register_register("sp", "immediate", 32, 0, &spro->immediate, &sprn->immediate);
	llsim_register_register("sp", "cycle_counter", 32, 0, &spro->cycle_counter, &sprn->cycle_counter);
	llsim_register_register("sp", "ctl_state", 3, 0, &spro->ctl_state, &sprn->ctl_state);
}

void sp_init(char *program_name)
{
	llsim_unit_t *llsim_sp_unit;
	llsim_unit_registers_t *llsim_ur;
	sp_t *sp;

	llsim_printf("initializing sp unit\n");

	inst_trace_fp = fopen("inst_trace.txt", "w");
	if (inst_trace_fp == NULL) {
		printf("couldn't open file inst_trace.txt\n");
		exit(1);
	}

	cycle_trace_fp = fopen("cycle_trace.txt", "w");
	if (cycle_trace_fp == NULL) {
		printf("couldn't open file cycle_trace.txt\n");
		exit(1);
	}

	llsim_sp_unit = llsim_register_unit("sp", sp_run);
	llsim_ur = llsim_allocate_registers(llsim_sp_unit, "sp_registers", sizeof(sp_registers_t));
	sp = llsim_malloc(sizeof(sp_t));
	llsim_sp_unit->private = sp;
	sp->spro = llsim_ur->old;
	sp->sprn = llsim_ur->new;

	sp->sram = llsim_allocate_memory(llsim_sp_unit, "sram", 32, SP_SRAM_HEIGHT, 0);
	sp_generate_sram_memory_image(sp, program_name);

	sp->start = 1;

	sp_register_all_registers(sp);
}
