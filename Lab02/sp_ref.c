#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <pthread.h>

#include "llsim.h"

#define sp_printf(a...)						\
	do {							\
		llsim_printf("sp: clock %d: ", llsim->clock);	\
		llsim_printf(a);				\
	} while (0)

//DMA declarations
int mem_availablity = 0; // = 1 if memory available for the next 2 clock cycles
int dma_start = 0; // = 1 if we will need to start the dma module

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
	
	// 2 bit control state machine state register
	int dma_state;

	// data length for DMA
	int dma_length;

	// counts how many transfer made
	int dma_transfer_counter;

	// 1 bit dma-busy indicator (0 or 1)
	int dma_busy;

	// 32 bit dma source address
	int dma_src;

	// 32 bit dma destination address
	int dma_dst;

	// control states
	#define CTL_STATE_IDLE		0
	#define CTL_STATE_FETCH0	1
	#define CTL_STATE_FETCH1	2
	#define CTL_STATE_DEC0		3
	#define CTL_STATE_DEC1		4
	#define CTL_STATE_EXEC0		5
	#define CTL_STATE_EXEC1		6

	// DMA states
	#define DMA_STATE_IDLE		0
	#define DMA_STATE_WAIT		1
	#define DMA_STATE_READ		2
	#define DMA_STATE_WRITE		3

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
#define CPY 25 //DMA Copy -> new opcode, gets: CPY  0, SRC, DST, LENGTH
#define CPO 26 //DMA Copy -> new opcode, gets: CPO DST, 0, 0, 0

static char opcode_name[32][4] = {"ADD", "SUB", "LSF", "RSF", "AND", "OR", "XOR", "LHI",
				 "LD", "ST", "U", "U", "U", "U", "U", "U",
				 "JLT", "JLE", "JEQ", "JNE", "JIN", "U", "U", "U",
				 "HLT", "CPY", "CPO", "U", "U", "U", "U", "U"};

void dma_function(sp_registers_t *spro, sp_registers_t *sprn, sp_t *sp) //DMA function
{
	int ram_q = 0;

	switch (spro->dma_state) 
	{
	case DMA_STATE_IDLE:
		if (dma_start) 
		{
			sprn->dma_state = DMA_STATE_WAIT;
			sprn->dma_busy = 1;
		}
		else 
		{
			sprn->dma_state = DMA_STATE_IDLE; //Switching to next state
		}
		break;

	case DMA_STATE_READ:
		llsim_mem_read(sp->sram, spro->dma_src);

		sprn->dma_state = DMA_STATE_WRITE; //Switching to next state
		break;

	case DMA_STATE_WRITE:
		ram_q = llsim_mem_extract_dataout(sp->sram, 31, 0);

		llsim_mem_set_datain(sp->sram, ram_q, 31, 0);
		llsim_mem_write(sp->sram, spro->dma_dst);

		sprn->dma_transfer_counter = sprn->dma_transfer_counter + 1;
		sprn->dma_dst = spro->dma_dst + 1;
		sprn->dma_src = spro->dma_src + 1;

		if (spro->dma_transfer_counter == spro->dma_length - 1)
		{ //last transfer for the dma instruction
			sprn->dma_busy = 0;
			sprn->dma_state = CTL_STATE_IDLE;
			sprn->dma_transfer_counter = 0;
		}
		else if (mem_availablity)
		{
			sprn->dma_state = DMA_STATE_READ; //Switching to next state
		}
		else 
		{
			sprn->dma_state = DMA_STATE_WAIT; //Switching to next state
		}
		break;

	case DMA_STATE_WAIT:
		if (!mem_availablity)
		{
			sprn->dma_state = DMA_STATE_WAIT; //Switching to next state
		}
		else 
		{
			sprn->dma_state = DMA_STATE_READ; //Switching to next state
		}
		break;
	}

}

static void dump_sram(sp_t *sp)
{
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
	
static void sp_ctl(sp_t *sp)
{
	sp_registers_t *spro = sp->spro;
	sp_registers_t *sprn = sp->sprn;
	int i;
	int read_value = 0;
	
	typedef struct DMA{ //DMA new struct decleration
    int source_address;
    int dest_address;
    int len;
} DMA_arg;

	// sp_ctl

	fprintf(cycle_trace_fp, "cycle %d\n", spro->cycle_counter);
	for (i = 2; i <= 7; i++)
		fprintf(cycle_trace_fp, "r%d %08x\n", i, spro->r[i]);
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
	fprintf(cycle_trace_fp, "ctl_state %08x\n", spro->ctl_state);
	fprintf(cycle_trace_fp, "dma_state %08x\n", spro->dma_state); //DMA print
	fprintf(cycle_trace_fp, "dma_busy %08x\n", spro->dma_busy); //DMA print
	fprintf(cycle_trace_fp, "dma_transfer_counter %08x\n\n", spro->dma_transfer_counter); //DMA print

	sprn->cycle_counter = spro->cycle_counter + 1;

    switch (spro->ctl_state) {
        case CTL_STATE_IDLE:
            sprn->pc = 0;
            if (sp->start)
                sprn->ctl_state = CTL_STATE_FETCH0;
            break;

        case CTL_STATE_FETCH0:
			//DMA
			mem_availablity = 0;
			dma_function(spro, sprn, sp);

            llsim_mem_read(sp->sram, spro->pc);
            sprn->ctl_state = CTL_STATE_FETCH1;
            break;

        case CTL_STATE_FETCH1:
			//DMA
			mem_availablity = 1;
			dma_function(spro, sprn, sp);

            sprn->inst = llsim_mem_extract_dataout(sp->sram, 31, 0);
            sprn->ctl_state = CTL_STATE_DEC0; //Setting the next state
            break;

        case CTL_STATE_DEC0:
			sprn->opcode = (spro->inst >> 25) & 31; //Breaking the Order to paramaters with shift & mask (from ex1)
			sprn->dst = (spro->inst >> 22) & 7;
			sprn->src0 = (spro->inst >> 19) & 7;
			sprn->src1 = (spro->inst >> 16) & 7;
			sprn->immediate = spro->inst & 65535; // 2^15 -1
			if ((spro->inst & 32768) != 0)
				sprn->immediate = sprn->immediate + (65535 << 16);

			//DMA
			mem_availablity = 0;
			dma_function(spro, sprn, sp);

			sprn->ctl_state = CTL_STATE_DEC1; //Setting the next state
            break;

        case CTL_STATE_DEC1:

			//DMA
			if (spro->opcode == CPY && spro->dma_busy == 0) 
			{
				dma_start = 1;
				sprn->dma_src = spro->r[spro->src0];
				sprn->dma_dst = spro->r[spro->src1];
				sprn->dma_length = spro->immediate;
			}
			if (spro->opcode != LD && spro->opcode != ST) 
			{
				mem_availablity = 1;
			}
			else 
			{
				mem_availablity = 0;
			}
			dma_function(spro, sprn, sp);
			//END DMA

			if (spro->opcode == LHI) //LHI order
			{
				sprn->alu0 = spro->r[spro->dst];
				sprn->alu1 = spro->immediate;
			}
			else {
				if (spro->src0 == 0)
					sprn->alu0 = 0;
				else if (spro->src0 == 1)
					sprn->alu0 = spro->immediate;
				else
					sprn->alu0 = spro->r[spro->src0];
				if (spro->src1 == 0)
					sprn->alu1 = 0;
				else if (spro->src1 == 1)
					sprn->alu1 = spro->immediate;
				else
					sprn->alu1 = spro->r[spro->src1];
			}
			sprn->ctl_state = CTL_STATE_EXEC0; //Setting the next state
            break;

        case CTL_STATE_EXEC0:

			//DMA
			dma_start = 0;
			mem_availablity = 0;
			dma_function(spro, sprn, sp);

			switch (spro->opcode)
			{ //Prefoming the action itself
			case CPO: //DMA
				sprn->aluout = spro->dma_busy;
				break;
			case ADD:
				sprn->aluout = spro->alu0 + spro->alu1;
				break;
			case SUB:
				sprn->aluout = spro->alu0 - spro->alu1;
				break;
			case RSF:
				sprn->aluout = spro->alu1 >> spro->alu0;
				break;
			case LSF:
				sprn->aluout = spro->alu1 << spro->alu0;
				break;
			case OR:
				sprn->aluout = spro->alu0 | spro->alu1;
				break;
			case XOR:
				sprn->aluout = spro->alu0 ^ spro->alu1;
				break;
			case AND:
				sprn->aluout = spro->alu0 & spro->alu1;
				break;
			case LHI:
				sprn->aluout = (spro->alu0 & 65535) + (spro->alu1 << 16);
				break;
			case LD:
				llsim_mem_read(sp->sram, spro->alu1);
				break;
			case JLT:
				if (spro->alu0 < spro->alu1)
					sprn->aluout = 1;
				else
					sprn->aluout = 0;
				break;
			case ST:
				sprn->aluout = 0;
				break;
			case JLE:
				if (spro->alu0 <= spro->alu1)
					sprn->aluout = 1;
				else
					sprn->aluout = 0;
				break;
			case JNE:
				if (spro->alu0 != spro->alu1)
					sprn->aluout = 1;
				else
					sprn->aluout = 0;
				break;
			case JEQ:
				if (spro->alu0 == spro->alu1)
					sprn->aluout = 1;
				else
					sprn->aluout = 0;
				break;
			case JIN:
				sprn->aluout = 1;
				break;
			case HLT:
				sprn->aluout = 0;
				break;
			default:
                   break;
            }
            sprn->ctl_state = CTL_STATE_EXEC1;
            break;

        case CTL_STATE_EXEC1:
			//DMA
			mem_availablity = 0;
			dma_function(spro, sprn, sp);

            if (spro->opcode == HLT) //HULT
			{
                sp->start = 0;
                sprn->ctl_state = CTL_STATE_IDLE;
                dump_sram(sp);
                llsim_stop();
                break;
            }
			else if (spro->opcode == LD)
			{
				read_value = llsim_mem_extract_dataout(sp->sram, 31, 0);
				sprn->r[spro->dst] = read_value;
			}
			else if (spro->opcode == ST)
			{
				llsim_mem_set_datain(sp->sram, spro->alu0, 31, 0);
				llsim_mem_write(sp->sram, spro->alu1);
			}
			else if (spro->opcode == JLT || spro->opcode == JLE || spro->opcode == JEQ || spro->opcode == JNE || spro->opcode == JIN)
			{
				if (spro->aluout == 1)
				{
					sprn->pc = spro->immediate;
					sprn->ctl_state = CTL_STATE_FETCH0; //Setting the next state
					sprn->r[7] = spro->pc; //Saving the pc from the jump to r[7]
					break;
				}
			}
			else if (spro->opcode == ADD || spro->opcode == SUB || spro->opcode == LSF || spro->opcode == RSF || spro->opcode == AND || spro->opcode == OR || spro->opcode == XOR || spro->opcode == LHI)
			{
				sprn->r[spro->dst] = spro->aluout;
			}
			else if (spro->opcode == CPO) //DMA
			{
				
				sprn->r[spro->dst] = spro->aluout;
            }
            sprn->ctl_state = CTL_STATE_FETCH0; //Setting next state
            sprn->pc = sprn->pc + 1; //PC++
            break;
    }

	//Print to inst_trace file (taken from ex1)
	if (spro->ctl_state == 6) //printing at CTL_STATE_EXEC1 only
	{
		fprintf(inst_trace_fp, "--- instruction %i (%04x) @ PC %i (%04x) -----------------------------------------------------------\n", (spro->cycle_counter) / 6 - 1, (spro->cycle_counter) / 6 - 1, spro->pc, spro->pc);

		fprintf(inst_trace_fp, "pc = %04d, inst = %08x, opcode = %i (%s), dst = %i, src0 = %i, src1 = %i, immediate = %08x\n", spro->pc, spro->inst, spro->opcode, opcode_name[spro->opcode], spro->dst, spro->src0, spro->src1, spro->immediate);

		fprintf(inst_trace_fp, "r[0] = 00000000 r[1] = %08x r[2] = %08x r[3] = %08x \n", (spro->immediate != 0) ? spro->immediate : 0, spro->r[2], spro->r[3]);

		fprintf(inst_trace_fp, "r[4] = %08x r[5] = %08x r[6] = %08x r[7] = %08x \n\n", spro->r[4], spro->r[5], spro->r[6], spro->r[7]);

		if (spro->opcode == ADD || spro->opcode == SUB || spro->opcode == LSF || spro->opcode == RSF || spro->opcode == AND || spro->opcode == OR || spro->opcode == XOR || spro->opcode == LHI) {
			fprintf(inst_trace_fp, ">>>> EXEC: R[%i] = %i %s %i <<<<\n\n", spro->dst, spro->alu0, opcode_name[spro->opcode], spro->alu1);
		}
		else if (spro->opcode == LD)
		{
			fprintf(inst_trace_fp, ">>>> EXEC: R[%i] = MEM[%i] = %08i <<<<\n\n", spro->dst, (spro->src1 == 1) ? spro->immediate : spro->r[spro->src1], read_value);
		}
		else if (spro->opcode == ST)
		{
			fprintf(inst_trace_fp, ">>>> EXEC: MEM[%i] = R[%i] = %08x <<<<\n\n", (spro->src1 == 1) ? spro->immediate : spro->r[spro->src1], spro->src0, spro->r[spro->src0]);
		}
		else if (spro->opcode == JLT || spro->opcode == JLE || spro->opcode == JEQ || spro->opcode == JNE || spro->opcode == JIN)
		{
			fprintf(inst_trace_fp, ">>>> EXEC: %s %i, %i, %i <<<<\n\n", opcode_name[spro->opcode], spro->r[spro->src0], spro->r[spro->src1], (spro->aluout == 1) ? spro->immediate : spro->pc + 1);
		}
		else if (spro->opcode == HLT)
		{
			fprintf(inst_trace_fp, ">>>> EXEC: HALT at PC %04x<<<<\n", spro->pc);
			fprintf(inst_trace_fp, "sim finished at pc %i, %i instructions\n", spro->pc, (spro->cycle_counter) / 6);
		}
		//DMA
		else if (spro->opcode == CPY) 
		{
		  fprintf(inst_trace_fp, ">>>> EXEC: CPY from address %04x to adress %04x with length of %d words <<<\n", spro->dma_src, spro->dma_dst, spro->dma_length);
		}
		else if (spro->opcode == CPO) 
		{
		  fprintf(inst_trace_fp, ">>>> EXEC: CPO result saved to register %d <<<<\n", spro->dst);
		}
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