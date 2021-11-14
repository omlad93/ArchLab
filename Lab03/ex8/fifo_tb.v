module main;
reg clk, reset, push, pop;
reg [1:0] in;
wire [1:0] out;
wire [7:0] queue;
wire full;
integer fail,print;

wire[1:0] q3,q2,q1,q0;
assign q3 = queue[7:6];
assign q2 = queue[5:4];
assign q1 = queue[3:2];
assign q0 = queue[1:0];

fifo #(4,2) fifo_test (clk, reset, in, push, pop, out, full, queue);

always@(print)
    begin : print_trace
		$display(" \t > queue: %b %b %b %b \n\t out=%b, full=%b\n %dth cycle\n  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n", q3,q2,q1,q0,out,full, $time);
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
#10; push =1; pop = 0; in = 1; #10; push=0;

//push 10
#10; push =1; pop = 0; in = 2; #10; push=0; print = print+1; #20

// push 11
#10; push =1; pop = 0; in = 3; #10; push=0; print = print+1; #20

// push 01
#10; push =1; pop = 0; in = 1; #10; push=0; print = print+1; #20

//push when empty (nop)
#10; push =1; pop = 0; in = 3; #10; push=0; print = print+1; #20

// pop (01)
#10; pop =1; push = 0; in = 0; #10; pop=0; print = print+1; #20

// pop (10)
#10; pop =1; push = 0; in = 0; #10; pop=0; print = print+1; #20

// pop (11) & push (10)
#10; pop =1; push = 1; in = 2; #10; pop=0; push=0; print = print+1; #20

// pop (01) & push (11)
#10; pop =1; push = 1; in = 3; #10; pop=0; push=0; print = print+1; #20

// push 01
#10; push =1; pop = 0; in = 1; #10; push=0; print = print+1; #20

//push 10
#10; push =1; pop = 0; in = 2; #10; push=0; print = print+1; #20

// pop (10) & push (00)
#10; pop =1; push = 1; in = 0 ; #10; pop=0; push=0; print = print+1; #20

// pop (11) & push (01)
#10; pop =1; push = 1; in = 1 ; #10; pop=0; push=0; print = print+1; #20


// #10
// // push 3
// in = 3;
// push = 1;
// #10
// print = print +1;
// if (full != 0 || out != 0) fail <= 2;

// #10 
// // pop 3
// in = 0; push = 0; pop = 1; 
// #10
// print = print +1;
// if (full != 0 || out != 3) fail <= 3;

// #10
// // push 3 (1)
// pop = 0; push =1;
// in = 3;
// #10
// print = print +1;
// if (full != 0 || out != 0) fail <= 4;

// #10
// // push 2 (2)
// in = 2;
// #10
// print = print +1;
// if (full != 0 || out != 0) fail <= 5;

// #10
// // push 1 (3)
// in = 1;
// #10

// print = print +1;
// if (full != 0 || out != 0) fail <= 6;

// #10
// // push 3 (4)
// in = 3;
// #10
// print = print +1;
// if (full != 1 || out != 0) fail <= 7;

// #10
// // push 0 (5 - no possible)
// in = 3;
// #10
// print = print +1;
// if (full != 1 || out != 0) fail <= 7;

// #10
// // push 1 & pop 3 
// pop = 1;
// in = 1;
// #10
// print = print +1;
// if (full != 1 || out != 1) fail <= 8;

// #10
// // pop 1 
// push = 0;
// #10
// print = print +1;
// if (full != 0 || out != 1) fail <= 9;


// #10
// // reset it again
// reset = 0 ;
// pop = 0; push = 0;
// in = 0;
// #10
// print = print +1;
// if (full != 0 || out != 0) fail <= 10;

// #10
// // push 3 (1)
// push = 1;
// in = 3;
// #10
// print = print +1;
// if (full != 0 || out != 0) fail <= 11;

// #10
// // pop 3
// push = 0; pop = 1;
// in = 0;
// #10
// print = print +1;
// if (full != 0 || out != 3) fail <= 12;

#10 
if(fail == 0)
	$display("PASSED ALL TESTS");
else
	$display("DID NOT PASS ALL TESTS. last failed in %d:", fail);
	
$finish;
end
always #5 clk = ~clk;
endmodule
