module main;
reg clk, reset, push, pop;
reg [1:0] in;
wire [1:0] out;
wire full;
integer fail;
// Correct the parameter assignment
fifo #(4,2) fifo_test (clk, reset, in, push, pop, out, full);

    always@(out or full)
        begin
			if (pop & full) $display("time=%d: popped %b from fifo. fifo is full", $time, out);
			if (pop & ~full) $display("time=%d: popped %b from fifo. fifo is not full", $time, out);
			if (push & full) $display("time=%d: pushed %b to fifo. fifo is full", $time, in);
			if (push & ~full) $display("time=%d: pushed %b to fifo. fifo is full", $time, in);
        end

initial begin
$dumpfile("waves.vcd");
$dumpvars;
clk = 1; fail = 0;
#10
//reset params
reset = 1;
in = 0;
push = 0; pop = 0; 
#10
if (full != 0 || out != 0) fail <= 1;

#10
// push 3
in = 3;
push = 1;
#10
if (full != 0 || out != 0) fail <= 2;

#10 
// pop 3
in = 0; push = 0; pop = 1; 
#10
if (full != 0 || out != 3) fail <= 3;

#10
// push 3 (1)
in = 3;
#10
if (full != 0 || out != 0) fail <= 4;

#10
// push 2 (2)
in = 2;
#10
if (full != 0 || out != 0) fail <= 5;

#10
// push 1 (3)
in = 1;
#10
if (full != 0 || out != 0) fail <= 6;

#10
// push 3 (4)
in = 3;
#10
if (full != 1 || out != 0) fail <= 7;

#10
// push 0 (5 - no possible)
in = 3;
#10
if (full != 1 || out != 0) fail <= 7;

#10
// push 1 & pop 3 
pop = 1;
in = 1;
#10
if (full != 1 || out != 1) fail <= 8;

#10
// pop 1 
push = 0;
#10
if (full != 0 || out != 1) fail <= 9;


#10
// reset it again
reset = 0 ;
pop = 0; push = 0;
in = 0;
#10
if (full != 0 || out != 0) fail <= 10;

#10
// push 3 (1)
push = 1;
in = 3;
#10
if (full != 1 || out != 0) fail <= 11;

#10
// pop 3
push = 0; pop = 1;
in = 0;
#10
if (full != 1 || out != 3) fail <= 11;

#10 
if(fail == 0)
	$display("PASSED ALL TESTS");
else
	$display("DID NOT PASS ALL TESTS. last failed in %d:", fail);
	
$finish;
end
always #5 clk = ~clk;
endmodule
