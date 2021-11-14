module fifo(clk, reset, in, push, pop, out, full, debug_queue);
   parameter N=4; // determines the maximum number of words in queue.
   parameter M=2; // determines the bit-width of each word, stored in the queue.

	localparam zero = 0;
   
   input clk, reset, push, pop;
   input [M-1:0] in;
   output [M-1:0] out;
   output [M*N-1:0] debug_queue;
   output full;

   
   // Fill Here
   
   reg [M*N-1:0] queue = 0;

   integer n = 0;   // n==0 means empty queue!
   reg [M-1:0] word_out = 0;
   reg is_full = 0;
   
	always @(posedge clk or reset) begin : active
		if(reset == 1) begin : reset_all
			queue <= 0;
			n <= 0;
			is_full <= 0;
			word_out <= 0; 
		end
		else begin : operations

			// word_out <=  (((1 << M) -1) << & queue); // create M times '1' in LSB's and padded with 32-M '0' LSB's

			if (n == 0) begin : empty_queue
				if (push == 1) begin : first_push // verified
					queue <= (in << (M*(N-1))) | queue ;
					n <= 1;
				end
			end
			else begin
				if( n < N) begin : common_rutine // n from 1 to N-1
					if (pop == 1 && push == 0) begin : pop_common // verified
						word_out <= (queue >>> (N-n)*M) ;
						queue <= ((queue >>> (N-n+1)*M) <<< (N-n+1)*M);
						n <= n - 1;
					end else;
					if (pop == 0 && push == 1) begin : push_common // verified
						queue <= (in << (M*(N-1))) | queue >>> M ;
						n <= n + 1;
						if (n == N-1) begin: raise_full
							is_full <= 1'b1;
						end
					end else;
					if (pop == 1 && push == 1) begin : push_pop_common // verified
						word_out <= (queue >>> (N-n)*M) ;
						queue <= (in << (M*(N-1))) | (((queue >>> (N-n+1)*M) <<< (N-n+1)*M) >>> M);
					end else;
					if (pop == 0 && push == 0) begin : nop_common
						n <= n;
						queue <= queue;
					end else;
				end
				else begin : edge_routine
					if(pop == 1 && push == 0) begin : pop_edge  // verified
						word_out <= queue ;
						queue <= ((queue >>> M) <<< M);
						n <= n - 1;
						is_full <= 1'b0;
					end else;
					if(pop == 1 && push == 1) begin : push_pop_edge //verified
						word_out <= queue ;
						queue <= (in << (M*(N-1))) | (queue >>> M); 
					end else;
					if(pop == 0) begin : nop_edge
			
					end else;
				end
			end			
		end
	end
	assign full = is_full;
	assign debug_queue = queue;
    assign out = (pop) ? word_out : zero;
	
endmodule