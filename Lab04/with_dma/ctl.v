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

	// DMA 
	reg [2:0]    dma_state;
   	reg [1:0]	 dma_wr;
	reg [31:0]   dma_start;
	reg [31:0]   dma_end;
   	reg [31:0]   dma_busy;
   	reg [31:0]   dma_size;
   	reg [31:0]   dma_data;
   	reg [31:0]   dma_counter;


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
			//dma dditions
			dma_state <= 0;
			dma_wr <= 0;
			dma_start <= 0;
			dma_end <= 0;
			dma_busy <= 0;
			dma_size <= 0;
			dma_data <= 0;
			dma_counter <= 0;


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
			//dma additions
			$fdisplay(verilog_trace_fp, "dma_state %08x", dma_state); 				
			$fdisplay(verilog_trace_fp, "dma_status %08x", dma_busy); 				
			$fdisplay(verilog_trace_fp, "DMA_transfer_counter %08x", dma_counter); 	


			cycle_counter <= cycle_counter + 1;

			case (ctl_state)
				`CTL_STATE_IDLE: begin
					pc <= 0;
					if (start)
					ctl_state <= `CTL_STATE_FETCH0;
				end
				
				// Our implementation: 

				`CTL_STATE_FETCH0: begin : load_op_from_mem

					ctl_state <= `CTL_STATE_FETCH1;
				end 

				`CTL_STATE_FETCH1: begin : load_op_to_reg
					inst <= sram_DO;
					ctl_state <= `CTL_STATE_DEC0;				
				end

				`CTL_STATE_DEC0: begin : parse_opcode_to_regs
					if (inst[15])
						immediate <= inst|((~(32'b0))<<16);
					else
						immediate <= inst[15:0];
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
					case(opcode)
						`ST, `HLT, `LD: begin : nop
							// nothing
						end

						default:  begin: compute
							aluout <= aluout_wire;
						end
					endcase
					ctl_state <= `CTL_STATE_EXEC1;
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

						`CMB : begin : dma_copy
							if (dma_state == `DMA_STATE_IDLE) begin : kick_dma
								dma_state <= `DMA_STATE_FETCH0;
								dma_size  <= immediate;
								dma_start <= alu0;
								dma_end	  <= alu1;
								dma_counter <= 0;
							end 
						end

						`POL : begin : polling
							case(dst)
								2: r2 <= dma_size;
								3: r3 <= dma_size;	
								4: r4 <= dma_size;				  			     		
								5: r5 <= dma_size;	
								6: r6 <= dma_size;	
 								7: r7 <= dma_size;
							endcase
						end

						`ST: begin : storing

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

			case (dma_state)
				`DMA_STATE_IDLE: begin : nothingg
					// nothing
				end

				`DMA_STATE_FETCH0: begin : fetch_dma
					dma_state <= `DMA_STATE_FETCH1;
				end 

				`DMA_STATE_FETCH1: begin : fetch1_dma
					dma_state <= `DMA_STATE_DEC0;
				end

				`DMA_STATE_DEC0: begin : dec0_dma
					dma_state <= `DMA_STATE_DEC1;
				end

				`DMA_STATE_DEC1: begin : dec1_dma
					dma_data <= (dma_wr == 0) ? sram_DO : dma_data;
					dma_state <= `DMA_STATE_EXEC0;
				end

				`DMA_STATE_EXEC0: begin : ex0_dma
					dma_state <= `DMA_STATE_EXEC1;
				end

				`DMA_STATE_EXEC1: begin : ex1_dma
					dma_size = (dma_size != 0) ? dma_size + 1 : dma_size;
					dma_counter = dma_counter +1;
					dma_start <= dma_start + 1;
					dma_end <= dma_end + 1;
					dma_state = (dma_size > 0) ? `DMA_STATE_FETCH0 :  `DMA_STATE_IDLE;
				end
			endcase

		end // !reset
    end // @posedge(clk)

	always@(ctl_state) begin : sram_routine
     	case (ctl_state)
			`CTL_STATE_FETCH0: begin
				sram_ADDR = pc[15:0];
				sram_DI = 0;
				sram_EN = 1;
				sram_WE = 0;
			end
			`CTL_STATE_EXEC0: begin
				if (opcode == `LD) begin
					sram_ADDR = alu1[15:0];
					sram_DI = 0;
					sram_EN = 1;
					sram_WE = 0;
				end
			end
			`CTL_STATE_EXEC1: begin
				if (opcode == `ST) begin
					sram_ADDR = alu1[15:0];
					sram_DI = alu0;
					sram_EN = 1;
					sram_WE = 1;
				end
			end
			default: begin
				sram_ADDR = 0;
				sram_DI = 0;
				sram_EN = 0;
				sram_WE = 0;
			end
     	endcase
	end //sram_routine

	always @(dma_state) begin : dma_routine

		dma_busy = dma_state != `DMA_STATE_IDLE;

		if ((dma_state == `DMA_STATE_DEC0) & (dma_wr == 0)) begin : set_dma_wr
				dma_wr = 1;
				sram_ADDR = dma_start[15:0];
				sram_DI = 0;
				sram_EN = 1;
				sram_WE = 0;
		end else if (dma_state == `DMA_STATE_DEC1) begin : when_dma_wr
			dma_wr = 0;
			sram_ADDR = dma_end[15:0];
			sram_DI = dma_data;
			sram_EN = 1;
			sram_WE = 1;
		end
		
	end //dma_routine

endmodule // CTL
