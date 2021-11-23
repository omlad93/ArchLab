`include "defines.vh"


/***********************************
 * CTL module
 **********************************/
module CTL(
	   clk,
	   reset,
	   start,
	   sram_ADDR,
	   sram_DI,
	   sram_EN,
	   sram_WE,
	   sram_DO,
	   opcode,
	   alu0,
	   alu1,
	   aluout_wire
	   );

   // inputs
	input clk;
   	input reset;
   	input start;
   	input [31:0] sram_DO;
	input [31:0] aluout_wire;

	// outputs
	output [15:0] sram_ADDR;
	output [31:0] sram_DI;
	output 	 	  sram_EN;
	output 	 	  sram_WE;
	output [31:0] alu0;
	output [31:0] alu1;
	output [4:0]  opcode;

	// registers
	reg [31:0] 	 r2;
	reg [31:0] 	 r3;
	reg [31:0] 	 r4;
	reg [31:0] 	 r5;
	reg [31:0] 	 r6;
	reg [31:0] 	 r7;
	reg [15:0] 	 pc;
	reg [31:0] 	 inst;
	reg [4:0] 	 opcode;
	reg [2:0] 	 dst;
	reg [2:0] 	 src0;
	reg [2:0] 	 src1;
	reg [31:0] 	 alu0;
	reg [31:0] 	 alu1;
	reg [31:0] 	 aluout;
	reg [31:0] 	 immediate;
	reg [31:0] 	 cycle_counter;
	reg [2:0] 	 ctl_state;

	integer 	 verilog_trace_fp, rc;

	initial begin : trace_file
		verilog_trace_fp = $fopen("verilog_trace.txt", "w");
	end

	/***********************************
		* set up sram inputs (outputs from sp)
		**********************************/
	reg [15:0] 	sram_ADDR;
	reg [31:0] 	sram_DI;
	reg 	 	sram_EN;
	reg 	 	sram_WE;


	// synchronous instructions
	always@(posedge clk) begin
		if (reset) begin : reset_on
			// registers reset
			r2 <= 0;
			r3 <= 0;
			r4 <= 0;
			r5 <= 0;
			r6 <= 0;
			r7 <= 0;
			pc <= 0;
			inst <= 0;
			opcode <= 0;
			dst <= 0;
			src0 <= 0;
			src1 <= 0;
			alu0 <= 0;
			alu1 <= 0;
			aluout <= 0;
			immediate <= 0;
			cycle_counter <= 0;
			ctl_state <= 0;
		end else begin :non_reset
			// generate cycle trace
			$fdisplay(verilog_trace_fp, "cycle %0d", cycle_counter);
			$fdisplay(verilog_trace_fp, "r2 %08x", r2);
			$fdisplay(verilog_trace_fp, "r3 %08x", r3);
			$fdisplay(verilog_trace_fp, "r4 %08x", r4);
			$fdisplay(verilog_trace_fp, "r5 %08x", r5);
			$fdisplay(verilog_trace_fp, "r6 %08x", r6);
			$fdisplay(verilog_trace_fp, "r7 %08x", r7);
			$fdisplay(verilog_trace_fp, "pc %08x", pc);
			$fdisplay(verilog_trace_fp, "inst %08x", inst);
			$fdisplay(verilog_trace_fp, "opcode %08x", opcode);
			$fdisplay(verilog_trace_fp, "dst %08x", dst);
			$fdisplay(verilog_trace_fp, "src0 %08x", src0);
			$fdisplay(verilog_trace_fp, "src1 %08x", src1);
			$fdisplay(verilog_trace_fp, "immediate %08x", immediate);
			$fdisplay(verilog_trace_fp, "alu0 %08x", alu0);
			$fdisplay(verilog_trace_fp, "alu1 %08x", alu1);
			$fdisplay(verilog_trace_fp, "aluout %08x", aluout);
			$fdisplay(verilog_trace_fp, "cycle_counter %08x", cycle_counter);
			$fdisplay(verilog_trace_fp, "ctl_state %08x\n", ctl_state);

			cycle_counter <= cycle_counter + 1;

			case (ctl_state)
				`CTL_STATE_IDLE: begin
					pc <= 0;
					if (start)
					ctl_state <= `CTL_STATE_FETCH0;
				end
				
				// Our implementation: 
				`CTL_STATE_FETCH0: begin : load_op_from_mem
					sram_ADDR = pc[15:0];
					sram_DI = 	 0;
					sram_EN = 	 1;
					sram_WE = 	 0;
					ctl_state <= `CTL_STATE_FETCH1;
				end 

				`CTL_STATE_FETCH1: begin : load_op_to_reg
					inst <= sram_DO;
					ctl_state <= `CTL_STATE_DEC0;
					sram_ADDR = 0; //reseet sram inputs
					sram_DI = 0;   //reseet sram inputs
					sram_EN = 0;	//reseet sram inputs
					sram_WE = 0;	//reseet sram inputs
					
				end

				`CTL_STATE_DEC0: begin : parse_opcode_to_regs
					immediate <= inst[15];
					src1 <= inst[18:16];
					src0 <= inst[21:19];
					dst <= inst[24:22];
					opcode <= inst[29:25];
					ctl_state <= `CTL_STATE_DEC1;
				end

				`CTL_STATE_DEC1: begin : load_alus
					// load alu0
					case (src0)
						0: alu0 <= 0;
						1: alu0 <= immediate;
						2: alu0 <= r2;
						3: alu0 <= r3;
						4: alu0 <= r4;
						5: alu0 <= r5;
						6: alu0 <= r6;
						7: alu0 <= r7;
					endcase
					// load alu1
					case (src1)
						0: alu1 <= 0;
						1: alu1 <= immediate;
						2: alu1 <= r2;
						3: alu1 <= r3;
						4: alu1 <= r4;
						5: alu1 <= r5;
						6: alu1 <= r6;
						7: alu1 <= r7;
					endcase
					ctl_state <= `CTL_STATE_EXEC0;
				end

				`CTL_STATE_EXEC0: begin : exec0
					pc <= pc+1;
					case(opcode)
						`ST, `HLT: begin : nop
							// nothing
						end
							
						`LD: begin : load
							sram_ADDR = alu1[15:0];
							sram_DI = 	 0;
							sram_EN = 	 1;
							sram_WE = 	 0;
						end

						default:  begin: compute
							aluout <= aluout_wire;
						end

					endcase
				end

				`CTL_STATE_EXEC1: begin
					pc <= pc+1;
					case(opcode)
						`JEQ,`JLE,`JNE,`JIN: begin : jumps
							if (aluout == 1) begin : jump_routine
								r7 <= pc;
								pc <= immediate;
							end
						end

						`ADD, `SUB, `AND, `OR , `XOR, `LSF, `RSF, `LHI : begin : alu_result
							case (dst)
								2: r2 <= aluout_wire;
								3: r3 <= aluout_wire;	
								4: r4 <= aluout_wire;				  			     		
								5: r5 <= aluout_wire;	
								6: r6 <= aluout_wire;	
 								7: r7 <= aluout_wire;
							endcase 
						end

						`LD : begin : loading
							case(dst)
								2: r2 <= sram_DO;
								3: r3 <= sram_DO;	
								4: r4 <= sram_DO;				  			     		
								5: r5 <= sram_DO;	
								6: r6 <= sram_DO;	
 								7: r7 <= sram_DO;
							endcase
						end

						`ST: begin : storing
							sram_ADDR = alu1[15:0];
							sram_DI =	 alu0;
							sram_EN = 	 1;
							sram_WE = 	 1;
						end

						`HLT: begin : halt
							ctl_state <= `CTL_STATE_IDLE;
							$fclose(verilog_trace_fp);
							$writememh("verilog_sram_out.txt", top.SP.SRAM.mem);
							$finish;
						end

					endcase
					
					ctl_state <= `CTL_STATE_FETCH0;	
				end

			endcase

			// if (opcode == `HLT) begin
			// 	$fclose(verilog_trace_fp);
			// 	$writememh("verilog_sram_out.txt", top.SP.SRAM.mem);
			// 	$finish;
			// end

		end // !reset
    end // @posedge(clk)

endmodule // CTL
