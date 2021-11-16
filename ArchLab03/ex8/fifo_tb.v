module main;
reg clk, reset, push, pop;
reg [1:0] in;
wire [1:0] out;
wire full;
integer fail,print;

// wire [7:0] queue;
// wire[1:0] q3,q2,q1,q0;
// assign q3 = queue[7:6];
// assign q2 = queue[5:4];
// assign q1 = queue[3:2];
// assign q0 = queue[1:0];

fifo #(4,2) fifo_test (clk, reset, in, push, pop, out, full);

always@(print)
    begin : print_trace
		if (pop && ~push) begin : single_pop
			$display("popped %b",out);
		end 
		else if (~pop && push) begin : single_push
				$display ("pushed %b", in);
		end
		else if (pop && push) begin : push_n_pop
				$display ("poped %b & pushed %b", out, in);
		end else
				$display("nothing happend...");
		// $display(" \t > queue: %b %b %b %b \n\t out=%b, full=%b\n %dth cycle\n  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n", q3,q2,q1,q0,out,full, $time);
    end


initial begin
$dumpfile("waves.vcd");
$dumpvars;
print = 0;
clk = 1; fail = 0;
#10

//reset params
reset = 1;
in = 0;
push = 0; pop = 0; 
#10
print = print +1;
if (full != 0 || out != 0) fail <= 1;

#10 reset =0;

// push 01
#10; push =1; pop = 0; in = 1;  print = print+1; #10; push=0; #20
if (out != 2'b01 || full != 0) fail <= 2;
//push 10
#10; push =1; pop = 0; in = 2;  print = print+1; #10; push=0; #20
if (out != 2'b01 || full != 0) fail <= 3;
// push 11
#10; push =1; pop = 0; in = 3;  print = print+1; #10; push=0; #20
if (out != 2'b01 || full != 0) fail <= 4;
// push 01
#10; push =1; pop = 0; in = 1;  print = print+1; #10; push=0; #20
if (out != 2'b01 || full != 1) fail <= 5;
//push when empty (nop)
#10; push =1; pop = 0; in = 3;  print = print+1; #10; push=0; #20
if (out != 2'b01 || full != 1) fail <= 6;
// pop (01)
#10; pop =1; push = 0; in = 0;  print = print+1; #10; pop=0; #20
if (out != 2'b01 || full != 0) fail <= 7;
// pop (10)
#10; pop =1; push = 0; in = 0;  print = print+1; #10; pop=0; #20
if (out != 2'b10 || full != 0) fail <= 8;
// pop (11) & push (10)
#10; pop =1; push = 1; in = 2;  print = print+1; #10; pop=0; push=0; #20
if (out != 2'b11 || full != 0) fail <= 9;
// pop (01) & push (11)
#10; pop =1; push = 1; in = 3;  print = print+1; #10; pop=0; push=0; #20
if (out != 2'b01 || full != 0) fail <= 10;
// push 01
#10; push =1; pop = 0; in = 1;  print = print+1; #10; push=0; #20
if (out != 2'b10 || full != 0) fail <= 11;
//push 10
#10; push =1; pop = 0; in = 2;  print = print+1; #10; push=0; #20
if (out != 2'b10 || full != 1) fail <= 12;
// pop (10) & push (00)
#10; pop =1; push = 1; in = 0 ;  print = print+1; #10; pop=0; push=0; #20
if (out != 2'b10 || full != 1) fail <= 13;
// pop (11) & push (01)
#10; pop =1; push = 1; in = 1 ;  print = print+1; #10; pop=0; push=0; #20
if (out != 2'b11 || full != 1) fail <= 14;


#10 
if(fail == 0)
	$display("PASSED ALL TESTS");
else
	$display("DID NOT PASS ALL TESTS. last failed in %d:", fail);
	
$finish;
end
always #5 clk = ~clk;
endmodule
