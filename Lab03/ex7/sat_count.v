`include "../ex5/addsub.v"

module sat_count(clk, reset, branch, taken, prediction);
   parameter N=2;
   input clk, reset, branch, taken;
   output prediction;

   reg [3:0] counter;
   wire [3:0] next_counter;
   reg [3:0] exp_N = {N{1'b1}};
   reg [3:0] one = 4'b1;

   addsub addsub( next_counter, counter, one, ~taken );

   always @ (posedge clk) begin
      if (reset) begin
         counter = 4'b0;
      end
      else begin
         if (branch) begin
            if (taken) begin
               if (exp_N > counter + 1) begin
                  counter <= next_counter;
               end
               else begin
                  counter <= exp_N;
               end
            end
            else begin
               if (counter > 0) begin
                  counter <= next_counter;
               end
               else begin
                  counter <= 4'b0;
               end
            end
         end
         else begin
            counter <= counter;
         end
      end
   end



assign prediction = counter[N-1];

   
endmodule
