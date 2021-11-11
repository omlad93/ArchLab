module fifo(clk, reset, in, push, pop, out, full, debug_queue);
   parameter N=4; // determines the maximum number of words in queue.
   parameter M=2; // determines the bit-width of each word, stored in the queue.

   
   input clk, reset, push, pop;
   input [M-1:0] in;
   output [M-1:0] out;
   output [M*N-1:0] debug_queue;
   output full;

   
   // Fill Here
   
   reg [M*N-1:0] queue = 0;

   integer n = 0;   // n==0 means empty queue!
   reg [M-1:0] word_out = 0;
   integer is_full = 0;
   
	always @(posedge clk or reset) begin : active
		if(reset == 1) begin : reset_all
			queue <= 0;
			n <= 0;
			is_full <= 0;
			word_out = 0; 
		end
		else begin : operations

			word_out <=  (((1 << M) -1) & queue); // create M times '1' in LSB's and padded with 32-M '0' LSB's

			if (n == 0) begin : empty_queue
				if (push == 1) begin : first_push
					queue <= (in << (M*n)) | queue ;
					n <= n + 1;
				end
			end
			else begin
				if(n < N) begin
					if (pop == 1 && push == 0) begin : pop_common
						queue <= (queue >>> M);
						n <= n - 1;
					end else;
					if (pop == 0 && push == 1) begin : push_common
						queue <= (in << (M*n)) | queue ;
						n <= n + 1;
					end else;
					if (pop == 1 && push == 1) begin : push_pop_common
						queue <= (in << (M*(n-1))) | (queue >>> M);
						n <= n;
					end else;
					if (pop == 0 && push == 0) begin : nop_common
						n <= n;
						queue <= queue;
					end else;
				end
				else begin 
					if(pop == 1 && push == 0) begin : pop_edge
						queue <= (queue >> M);
						n <= n - 1;
					end else;
					if(pop == 1 && push == 1) begin : push_pop_edge
						queue <= (in << (M*(n-1))) | (queue >>> M);
						n <= n;
					end else;
					if(pop == 0) begin : nop_edge
						n <= n;
						queue <= queue;
					end else;
				end
			end

			is_full = (n < N - 1) ? 0 :1;				
			
		end
    end
	
	assign debug_queue = queue;
    assign out = word_out;
    assign full = is_full; // note that full only updates on posedge clk or reset so full[t+1]='1' iff n[t] == N 
endmodule