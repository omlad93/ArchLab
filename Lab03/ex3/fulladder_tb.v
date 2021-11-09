module main;
  reg a, b, ci;
  wire sum, co;

  fulladder fulladder( sum, co, a, b, ci);

  initial begin
  	$monitor("time=%d: a %b, b %b, ci %b, sum %b, co %b\n", $time, a, b, ci, sum, co);
	  $dumpfile("waves.vcd");
	  $dumpvars;
    a = 0; b = 0; ci = 0;
    #10;
    a = 1; b = 0; ci = 0;
    #10;
    a = 0; b = 1; ci = 0;
    #10;
    a = 0; b = 0; ci = 1;
    #10;
    a = 1; b = 1; ci = 0;
    #10;
    a = 1; b = 0; ci = 1;
    #10;
    a = 1; b = 1; ci = 1;
    #10;
    a = 0; b = 1; ci = 1;
    #10;
  	$finish;
     end

endmodule
