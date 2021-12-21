#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

#include "llsim.h"

#define sp_printf(a...)						\
	do {							\
		llsim_printf("sp: clock %d: ", llsim->clock);	\
		llsim_printf(a);				\
	} while (0)


int nr_simulated_instructions = 0;
FILE *inst_trace_fp = NULL, *cycle_trace_fp = NULL;

int branch_predictor = 0;

typedef struct sp_registers_s {
	// 6 32 bit registers (r[0], r[1] don't exist)
	int r[8];

	// 32 bit cycle counter
	int cycle_counter;

	// fetch0
	int fetch0_active; // 1 bit
	int fetch0_pc; // 16 bits

	// fetch1
	int fetch1_active; // 1 bit
	int fetch1_pc; // 16 bits

	// dec0
	int dec0_active; // 1 bit
	int dec0_pc; // 16 bits
	int dec0_inst; // 32 bits

	// dec1
	int dec1_active; // 1 bit
	int dec1_pc; // 16 bits
	int dec1_inst; // 32 bits
	int dec1_opcode; // 5 bits
	int dec1_src0; // 3 bits
	int dec1_src1; // 3 bits
	int dec1_dst; // 3 bits
	int dec1_immediate; // 32 bits

	// exec0
	int exec0_active; // 1 bit
	int exec0_pc; // 16 bits
	int exec0_inst; // 32 bits
	int exec0_opcode; // 5 bits
	int exec0_src0; // 3 bits
	int exec0_src1; // 3 bits
	int exec0_dst; // 3 bits
	int exec0_immediate; // 32 bits
	int exec0_alu0; // 32 bits
	int exec0_alu1; // 32 bits

	// exec1
	int exec1_active; // 1 bit
	int exec1_pc; // 16 bits
	int exec1_inst; // 32 bits
	int exec1_opcode; // 5 bits
	int exec1_src0; // 3 bits
	int exec1_src1; // 3 bits
	int exec1_dst; // 3 bits
	int exec1_immediate; // 32 bits
	int exec1_alu0; // 32 bits
	int exec1_alu1; // 32 bits
	int exec1_aluout;
} sp_registers_t;

/*
 * Master structure
 */
typedef struct sp_s {
	// local srams
#define SP_SRAM_HEIGHT	64 * 1024
	llsim_memory_t *srami, *sramd;

	unsigned int memory_image[SP_SRAM_HEIGHT];
	int memory_image_size;

	int start;

	sp_registers_t *spro, *sprn;
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
#define CMB 10 // Copy Memory Background
#define POL 11 // Poll DMA transaction
#define JLT 16
#define JLE 17
#define JEQ 18
#define JNE 19
#define JIN 20
#define HLT 24
#define NOP 31

static char opcode_name[32][4] = {"ADD", "SUB", "LSF", "RSF", "AND", "OR", "XOR", "LHI",
				 "LD", "ST", "CMB", "POL", "U", "U", "U", "U",
				 "JLT", "JLE", "JEQ", "JNE", "JIN", "U", "U", "U",
				 "HLT", "U", "U", "U", "U", "U", "U", "NOP"};

//////////////////////////////////////////////////////////////
// Our additional functionality start here
#define increse_predictor() (branch_predictor = (branch_predictor <= 2) ? branch_predictor++ : branch_predictor)
#define decrease_predictor() (branch_predictor = (branch_predictor > 0) ? branch_predictor-- : branch_predictor)
#define branch_predicted() (branch_predictor > 1)
#define is_branch(opcode) ((opcode <=JNE) && (opcode >= JLT))
#define is_jump(opcode) ((opcode <=JNE) && (opcode >= JIN))
#define is_dma_op(opcode) ((opcode == CMB) || (opcode==POL))
#define is_alu_op(opcode) ((opcode <= LHI) || is_dma_op(opcode))
#define structural_hazard_expected(opcode) ((spro->dec1_opcode == ST) && (opcode == LD) && (spro->dec1_active))
#define src_is_real(src_reg) ((src_reg != 0) && (src_reg != 1))
#define data_hazard_mem(spro,src) ((spro->exec0_active) && (spro->exec0_opcode == LD) && (spro->exec0_dst == src))

#define FE0_MASK 0x0000ffff
#define DE0_MASK 0x0000001f
#define DE0_SHIFT 0x19
#define REG_MASK 0x7
#define SR1_SHIFT 0x10
#define SR0_SHIFT 0x13
#define DST_SHIFT 0x16
#define IMM_MASK 0xffff
#define IMM_SHIFT 0x14


int stall_dec1_is_needed(sp_registers_t* spro, sp_t *sp){
	int condition1 = (src_is_real(spro->dec1_src0) && (data_hazard_mem(spro, spro->dec1_src0)));
	int condition2 = (src_is_real(spro->dec1_src1) && (data_hazard_mem(spro, spro->dec1_src1)));
	return (condition1 || condition2);
}

void stall_dec1(sp_registers_t *spro,sp_registers_t *sprn){
	sprn->fetch0_active = spro->fetch0_active;
	sprn->fetch0_pc = spro->fetch0_pc;
	sprn->fetch1_active = spro->fetch1_active;
	sprn->fetch1_pc = spro->fetch1_pc;
	sprn->dec0_active = spro->dec0_active;
	sprn->dec0_pc = spro->dec0_pc;
	sprn->dec0_inst = spro->dec0_inst;
	sprn->dec1_active = spro->dec1_active;
	sprn->dec1_pc = spro->dec1_pc;
	sprn->dec1_inst = spro->dec1_inst;
	sprn->dec1_opcode = spro->dec1_opcode;
	sprn->dec1_src0 = spro->dec1_src0;
	sprn->dec1_src1 = spro->dec1_src1;
	sprn->dec1_dst = spro->dec1_dst;
	sprn->dec1_immediate = spro->dec1_immediate;
	sprn->exec0_pc = 0;
	sprn->exec0_inst = 0;
	sprn->exec0_opcode = NOP;
	sprn->exec0_dst = 0;
	sprn->exec0_src0 = 0;
	sprn->exec0_src1 = 0;
	sprn->exec0_immediate = 0;
	sprn->exec0_active = 1;
	sprn->exec1_active = 0;
}

void alu_preperation_dec1(sp_registers_t* spro,sp_registers_t* sprn){
	// TODO - Iris
	// check data hazards
}

void alu_execute(sp_registers_t *spro,sp_registers_t *sprn, int a0, int a1){
	// TODO - Iris
	// like lab 2


	//  also {if (spro->exec0_opcode == LD) llsim_mem_read(sp->sramd, alu1);}
}

void jump_execute(sp_registers_t *spro,sp_registers_t *sprn){
	//  TODO - Iris
}


// end of our additional functionality
//////////////////////////////////////////////////////////////

static void dump_sram(sp_t *sp, char *name, llsim_memory_t *sram)
{
	FILE *fp;
	int i;

	fp = fopen(name, "w");
	if (fp == NULL) {
                printf("couldn't open file %s\n", name);
                exit(1);
	}
	for (i = 0; i < SP_SRAM_HEIGHT; i++)
		fprintf(fp, "%08x\n", llsim_mem_extract(sram, i, 31, 0));
	fclose(fp);
}

static void sp_ctl(sp_t *sp)
{
	sp_registers_t *spro = sp->spro;
	sp_registers_t *sprn = sp->sprn;
	int i;

	fprintf(cycle_trace_fp, "cycle %d\n", spro->cycle_counter);
	fprintf(cycle_trace_fp, "cycle_counter %08x\n", spro->cycle_counter);
	for (i = 2; i <= 7; i++)
		fprintf(cycle_trace_fp, "r%d %08x\n", i, spro->r[i]);

	fprintf(cycle_trace_fp, "fetch0_active %08x\n", spro->fetch0_active);
	fprintf(cycle_trace_fp, "fetch0_pc %08x\n", spro->fetch0_pc);

	fprintf(cycle_trace_fp, "fetch1_active %08x\n", spro->fetch1_active);
	fprintf(cycle_trace_fp, "fetch1_pc %08x\n", spro->fetch1_pc);

	fprintf(cycle_trace_fp, "dec0_active %08x\n", spro->dec0_active);
	fprintf(cycle_trace_fp, "dec0_pc %08x\n", spro->dec0_pc);
	fprintf(cycle_trace_fp, "dec0_inst %08x\n", spro->dec0_inst); // 32 bits

	fprintf(cycle_trace_fp, "dec1_active %08x\n", spro->dec1_active);
	fprintf(cycle_trace_fp, "dec1_pc %08x\n", spro->dec1_pc); // 16 bits
	fprintf(cycle_trace_fp, "dec1_inst %08x\n", spro->dec1_inst); // 32 bits
	fprintf(cycle_trace_fp, "dec1_opcode %08x\n", spro->dec1_opcode); // 5 bits
	fprintf(cycle_trace_fp, "dec1_src0 %08x\n", spro->dec1_src0); // 3 bits
	fprintf(cycle_trace_fp, "dec1_src1 %08x\n", spro->dec1_src1); // 3 bits
	fprintf(cycle_trace_fp, "dec1_dst %08x\n", spro->dec1_dst); // 3 bits
	fprintf(cycle_trace_fp, "dec1_immediate %08x\n", spro->dec1_immediate); // 32 bits

	fprintf(cycle_trace_fp, "exec0_active %08x\n", spro->exec0_active);
	fprintf(cycle_trace_fp, "exec0_pc %08x\n", spro->exec0_pc); // 16 bits
	fprintf(cycle_trace_fp, "exec0_inst %08x\n", spro->exec0_inst); // 32 bits
	fprintf(cycle_trace_fp, "exec0_opcode %08x\n", spro->exec0_opcode); // 5 bits
	fprintf(cycle_trace_fp, "exec0_src0 %08x\n", spro->exec0_src0); // 3 bits
	fprintf(cycle_trace_fp, "exec0_src1 %08x\n", spro->exec0_src1); // 3 bits
	fprintf(cycle_trace_fp, "exec0_dst %08x\n", spro->exec0_dst); // 3 bits
	fprintf(cycle_trace_fp, "exec0_immediate %08x\n", spro->exec0_immediate); // 32 bits
	fprintf(cycle_trace_fp, "exec0_alu0 %08x\n", spro->exec0_alu0); // 32 bits
	fprintf(cycle_trace_fp, "exec0_alu1 %08x\n", spro->exec0_alu1); // 32 bits

	fprintf(cycle_trace_fp, "exec1_active %08x\n", spro->exec1_active);
	fprintf(cycle_trace_fp, "exec1_pc %08x\n", spro->exec1_pc); // 16 bits
	fprintf(cycle_trace_fp, "exec1_inst %08x\n", spro->exec1_inst); // 32 bits
	fprintf(cycle_trace_fp, "exec1_opcode %08x\n", spro->exec1_opcode); // 5 bits
	fprintf(cycle_trace_fp, "exec1_src0 %08x\n", spro->exec1_src0); // 3 bits
	fprintf(cycle_trace_fp, "exec1_src1 %08x\n", spro->exec1_src1); // 3 bits
	fprintf(cycle_trace_fp, "exec1_dst %08x\n", spro->exec1_dst); // 3 bits
	fprintf(cycle_trace_fp, "exec1_immediate %08x\n", spro->exec1_immediate); // 32 bits
	fprintf(cycle_trace_fp, "exec1_alu0 %08x\n", spro->exec1_alu0); // 32 bits
	fprintf(cycle_trace_fp, "exec1_alu1 %08x\n", spro->exec1_alu1); // 32 bits
	fprintf(cycle_trace_fp, "exec1_aluout %08x\n", spro->exec1_aluout);

	fprintf(cycle_trace_fp, "\n");

	sp_printf("cycle_counter %08x\n", spro->cycle_counter);
	sp_printf("r2 %08x, r3 %08x\n", spro->r[2], spro->r[3]);
	sp_printf("r4 %08x, r5 %08x, r6 %08x, r7 %08x\n", spro->r[4], spro->r[5], spro->r[6], spro->r[7]);
	sp_printf("fetch0_active %d, fetch1_active %d, dec0_active %d, dec1_active %d, exec0_active %d, exec1_active %d\n",
		  spro->fetch0_active, spro->fetch1_active, spro->dec0_active, spro->dec1_active, spro->exec0_active, spro->exec1_active);
	sp_printf("fetch0_pc %d, fetch1_pc %d, dec0_pc %d, dec1_pc %d, exec0_pc %d, exec1_pc %d\n",
		  spro->fetch0_pc, spro->fetch1_pc, spro->dec0_pc, spro->dec1_pc, spro->exec0_pc, spro->exec1_pc);

	sprn->cycle_counter = spro->cycle_counter + 1;

	// ctl FSM including our additions:

	if (sp->start)
		sprn->fetch0_active = 1;

	// fetch0
	sprn->fetch1_active = 0; 	
	if (spro->fetch0_active) {
		// sprn->fetch1_active = 1;					// next cycle fetch 1
		sprn->fetch0_pc = (spro->fetch0_pc + 1) & FE0_MASK; //next pc for fetch 0 is pc+1
		llsim_mem_read(sp->srami, spro->fetch0_pc); // read request of pc from SRAMi
		sprn->fetch1_pc = spro->fetch0_pc;			// next cycle pc is the same for fetch 1
		sprn->fetch1_active = 1;
	}

	// fetch1
	sprn->dec0_active = spro->fetch1_active;	//next cycle 
	if (spro->fetch1_pc){
		sprn->dec0_inst = llsim_mem_extract(sp->srami, spro->fetch1_pc, 31, 0); //actual read of insruction
		sprn->dec0_pc = spro->fetch1_pc;  //next cycle pc is the same for fetch dec0
	}
	
	// dec0
	sprn->dec1_active = spro->dec0_active ;
	if (spro->dec0_active) {
		opcode = (spro->dec0_inst >> DE0_SHIFT) & DE0_MASK;
		if (is_branch(opcode) && branch_predicted()){
				// reset instructions in pipe line (branch taken)
				sprn->fetch0_pc = spro->dec0_inst & DE0_MASK;
				sprn->fetch0_active = 1;
				sprn->dec0_active = 0;
				sprn->dec1_active = 0;
				sprn->fetch1_active = 0;
				
		}
		else if (structural_hazard_expected(opcode)){
			sprn->fetch1_active = 0;
			sprn->dec1_active = 0;
			sprn->dec0_pc = spro->dec0_pc;
			sprn->dec0_inst = spro->dec0_inst;
			sprn->dec0_active = spro->dec0_active;
			sprn->fetch0_active = spro->fetch1_active;
			sprn->fetch0_pc = spro->fetch1_pc;
		}
		else {
			instruction = spro->dec0_inst;
			sprn->dec1_opcode = op;
			sprn->dec1_dst = (instruction >> DST_SHIFT) & REG_MASK;
			sprn->dec1_src0 = (instruction >> SR0_SHIFT) & REG_MASK;
			sprn->dec1_src1 = (instruction >> SR1_SHIFT) & REG_MASK;
			sprn->dec1_immediate = spro->dec0_inst & IMM_MASK;
			sprn->dec1_immediate = ( sprn->dec1_immediate << IMM_SHIFT) >> IMM_SHIFT; // Sign Extention
			sprn->dec1_inst = instruction;
			sprn->dec1_pc = spro->dec0_pc;

		}
	}
	
	// dec1
	sprn->exec0_active = spro->dec1_active;
	if (spro->dec1_active) {
		if (stall_dec1_is_needed(spro,sp)){
			stall_dec1(spro, sprn);
		} else {
			alu_preperation_dec1(spro, sprn);
			sprn->exec0_pc = spro->dec1_pc;
			sprn->exec0_inst = spro->dec1_inst;
			sprn->exec0_opcode = spro->dec1_opcode;
			sprn->exec0_dst = spro->dec1_dst;
			sprn->exec0_src0 = spro->dec1_src0;
			sprn->exec0_src1 = spro->dec1_src1;
			sprn->exec0_immediate = spro->dec1_immediate;
		}

	// exec0
	sprn->exec1_active = spro->exec0_active;
	int a0 = spro->exec0_alu0;
	int a1 = spro->exec0_alu1;
	if (spro->exec0_active){
		if(spro->exec0_opcode != NOP){
			if (src_is_real(spro->exec0_src0)){
				if ((spro->exec1_active) && (is_jump(spro->exec1_opcode) && (spro->exec0_src0 == 7)){
					a0 = spro->exec1_pc;
				} else if ((spro->exec1_active) && (is_alu_op(spro->exec1_opcode)) && (spro->exec1_dst == spro->exec0_src0) ){
					a0 = spro->exec1_aluout;
				}
			}
			if (src_is_real(spro->exec0_src1)){
				if ((spro->exec1_active) && (is_jump(spro->exec1_opcode) && (spro->exec0_src1 == 7)){
					a1 = spro->exec1_pc;
				} else if ((spro->exec1_active) && (is_alu_op(spro->exec1_opcode)) && (spro->exec1_dst == spro->exec0_src1) ){
					a1 = spro->exec1_aluout;
				}

			}

			alu_execute(spro, sprn, a0, a1);
			// TODO - Omri - DMA behavioural 
			sprn->exec1_inst = spro->exec0_inst;
			sprn->exec1_pc = spro->exec0_pc;
			sprn->exec1_opcode = spro->exec0_opcode;
			sprn->exec1_alu0 = alu0;
			sprn->exec1_alu1 = alu1;
			sprn->exec1_dst = spro->exec0_dst;
			sprn->exec1_src0 = spro->exec0_src0;
			sprn->exec1_src1 = spro->exec0_src1;
			sprn->exec1_immediate = spro->exec0_immediate;

		} else {
			sprn->exec1_active = 0;
			sprn->exec1_pc = spro->exec1_pc;
			sprn->exec1_inst = spro->exec1_inst;
			sprn->exec1_opcode = spro->exec1_opcode;
			sprn->exec1_alu0 = spro->exec1_alu0;
			sprn->exec1_alu1 = spro->exec1_alu1;
			sprn->exec1_dst = spro->exec1_dst;
			sprn->exec1_src0 = spro->exec1_src0;
			sprn->exec1_src1 = spro->exec1_src1;
			sprn->exec1_immediate = spro->exec1_immediate;
		}

	}

	// exec1
	if (spro->exec1_active) {
		// FILL HERE
		if (spro->exec1_opcode == HLT) {
			llsim_stop();
			dump_sram(sp, "srami_out.txt", sp->srami);
			dump_sram(sp, "sramd_out.txt", sp->sramd);
		} else {
			instructions++;
			if (spro->exec1_opcode == ST){
				llsim_mem_set_datain(sp->sramd, spro->exec1_alu0, 31, 0); 
				llsim_mem_write(sp->sramd, spro->exec1_alu1);
			}
			else if (src_is_real(spro->exec1_dst) && (spro->exec1_opcode == LD)){
				sprn->r[spro->exec1_dst] = llsim_mem_extract(sp->sramd, spro->exec1_alu1, 31, 0);
			}
			else if(spro->exec1_opcode == POL){
				sprn->r[spro->exec1_dst] = spro->exec1_aluout;
			}
			} else if (is_jump(spro->exec1_opcode)){
				jump_execute(spro,sprn);

			} else if (src_is_real(spro->exec1_dst) && is_alu_op(spro->exec1_opcode)){
				sprn->r[spro->exec1_dst] = spro->exec1_aluout;
			}

		}
	}


	// always:
	// TODO - Omri - DMA behavioral 
}

static void sp_run(llsim_unit_t *unit)
{
	sp_t *sp = (sp_t *) unit->private;
	//	sp_registers_t *spro = sp->spro;
	//	sp_registers_t *sprn = sp->sprn;

	//	llsim_printf("-------------------------\n");

	if (llsim->reset) {
		sp_reset(sp);
		return;
	}

	sp->srami->read = 0;
	sp->srami->write = 0;
	sp->sramd->read = 0;
	sp->sramd->write = 0;

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
                //              printf("addr %x: %08x\n", addr, sp->memory_image[addr]);
                addr++;
                if (feof(fp))
                        break;
        }
	sp->memory_image_size = addr;

        fprintf(inst_trace_fp, "program %s loaded, %d lines\n", program_name, addr);

	for (i = 0; i < sp->memory_image_size; i++) {
		llsim_mem_inject(sp->srami, i, sp->memory_image[i], 31, 0);
		llsim_mem_inject(sp->sramd, i, sp->memory_image[i], 31, 0);
	}
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

	sp->srami = llsim_allocate_memory(llsim_sp_unit, "srami", 32, SP_SRAM_HEIGHT, 0);
	sp->sramd = llsim_allocate_memory(llsim_sp_unit, "sramd", 32, SP_SRAM_HEIGHT, 0);
	sp_generate_sram_memory_image(sp, program_name);

	sp->start = 1;
	
	// c2v_translate_end
}
