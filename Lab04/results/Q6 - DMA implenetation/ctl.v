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
	reg          dma_work; // kick dma
	reg          dma_wait; // when high dma doesnt start a new transfer
	reg          dma_status; // 1 for work, 0 for resting
	reg [2:0]    dma_state; // state
	reg [31:0]	 dma_src; //src of copy
	reg [31:0]   dma_dest; // dest of copy
   	reg [31:0]   dma_size; // size of transfer (# of words)
   	reg [31:0]   dma_data; // specfic data
   	reg [31:0]   dma_counter; // counter of transactions


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
			dma_work <= 0;
			dma_wait <= 0;
			dma_status <= 0;
			dma_state <= 0;
			dma_src <= 0;
			dma_dest <= 0;
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
			$fdisplay(verilog_trace_fp, "dma_status %08x", dma_status); 				
			//$fdisplay(verilog_trace_fp, "dma_transfer_size %08x", dma_size); 
			$fdisplay(verilog_trace_fp, "dma_transfer_counter %08x\n", dma_counter);


			cycle_counter <= cycle_counter + 1;

			case (ctl_state)
				`CTL_STATE_IDLE: begin
					pc <= 0;
					if (start)
					ctl_state <= `CTL_STATE_FETCH0;
				end
				
				// Our implementation: 

				`CTL_STATE_FETCH0: begin : load_op_from_mem

					dma_wait <= 0;
					ctl_state <= `CTL_STATE_FETCH1;
				end 

				`CTL_STATE_FETCH1: begin : load_op_to_reg
					inst <= sram_DO;
					// dma_wait = (sram_DO[29:25] == `LD); // LD will use SRAM in 2 cycles
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
					dma_wait <= (inst[29:25] == `LD);
					ctl_state <= `CTL_STATE_DEC1;
				end

				`CTL_STATE_DEC1: begin : load_alus

					case(opcode)
						`CMB: begin

							if (dma_status == 0) begin :kick_dma_now
								dma_size = immediate;
								dma_status = 1;
								dma_work = 1;
								// load src
								case (src0)
									0: dma_src <= 0;
									1: dma_src <= immediate;
									2: dma_src <= r2;
									3: dma_src <= r3;
									4: dma_src <= r4;
									5: dma_src <= r5;
									6: dma_src <= r6;
									7: dma_src <= r7;
								endcase
							end else begin
								dma_status = 1;
							end

								// load dest
								case (src1)
									0: dma_dest <= 0;
									1: dma_dest <= immediate;
									2: dma_dest <= r2;
									3: dma_dest <= r3;
									4: dma_dest <= r4;
									5: dma_dest <= r5;
									6: dma_dest <= r6;
									7: dma_dest <= r7;
								endcase

					

						end
						`POL: begin : do_nop
						end

						`ST, `LD: begin : take_mem
							dma_wait = 1;
						end


						default: begin : free_mem
							dma_wait = 0;
						end
					endcase

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
					dma_wait <= 1;
					case(opcode)
						`ST, `HLT, `LD: begin : nop
							//
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

						`ADD, `SUB, `AND, `OR , `XOR, `LSF, `RSF, `LHI, `CMB : begin : alu_result
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


						`POL : begin : polling
							case(dst)
								2: r2 <= dma_status;
								3: r3 <= dma_status;	
								4: r4 <= dma_status;				  			     		
								5: r5 <= dma_status;	
								6: r6 <= dma_status;	
 								7: r7 <= dma_status;
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
		end // !reset
    end // @posedge(clk)

	always@(ctl_state) begin : sram_routine
     	case (ctl_state)
			`CTL_STATE_FETCH0: begin
				sram_ADDR = pc[15:0];
				// sram_DI = 0;
				sram_EN = 1;
				sram_WE = 0;
			end
			`CTL_STATE_EXEC0: begin
				if (opcode == `LD) begin
					dma_wait = 1;
					sram_ADDR = alu1[15:0];
					// sram_DI = 0;
					sram_EN = 1;
					sram_WE = 0;
				end
			end
			`CTL_STATE_EXEC1: begin
				if (opcode == `LD) begin : stop_read
					// sram_ADDR = 0;
					// sram_DI = 0;
					sram_EN = 0;
					sram_WE = 0;		
				end else if (opcode == `ST) begin
					sram_ADDR = alu1[15:0];
					sram_DI = alu0;
					sram_EN = 1;
					sram_WE = 1;
				end
			end
			default: begin
				// sram_ADDR = 0;
				// sram_DI = 0;
				if (! dma_status) begin 
					sram_EN = 0;
					sram_WE = 0;
				end
				
			end
     	endcase
		case (dma_state)
			`DMA_IDLE: begin
				if (dma_work) begin : come_on
					dma_status = 1;
					dma_state = `DMA_HOLD;
				end
			end

			`DMA_HOLD: begin
				if (!dma_wait) begin
					dma_state = `DMA_READ;
					sram_ADDR = dma_src[15:0];
					// sram_DI = 0;
					sram_EN = 1;
					sram_WE = 0;
				end else begin
					dma_state = `DMA_HOLD;
				end
			end

			`DMA_READ: begin
				
				sram_EN = 1;
				sram_WE = 1;
				sram_ADDR = dma_dest[15:0];

				dma_state = `DMA_CPY;
			end

			`DMA_CPY: begin 
				dma_data = sram_DO;
				// sram_DI = sram_DO;
				if (dma_counter < dma_size-1) begin :copy_dma
					dma_src <= dma_src +1;
					dma_dest <= dma_dest +1;
					dma_counter <= dma_counter +1;
					
					if (!dma_wait) begin
						sram_ADDR = dma_src + 1;
						// sram_DI = 0;
						sram_EN = 1;
						sram_WE = 0;
						dma_state = `DMA_READ;
					end else begin
						sram_WE = 0;
						sram_EN = 0;
						// sram_EN = (ctl_state == `CTL_STATE_FETCH0) or ((ctl_state == `CTL_STATE_EXEC0) and (opcode=`LD)) or ((ctl_state == `CTL_STATE_EXEC1) and (opcode=`ST));
						dma_state = `DMA_HOLD ;
					end
					
				end else begin : finished_dma
					dma_counter = 0;
					dma_size = 0;
					dma_status = 0;
					dma_work = 0;
					dma_state = `DMA_IDLE;
				end
			end
		endcase
	end //sram_routine

	always @(sram_DO) begin
		if (dma_state == `DMA_CPY) begin
			sram_DI <= sram_DO;
		end	
	end

endmodule // CTL
