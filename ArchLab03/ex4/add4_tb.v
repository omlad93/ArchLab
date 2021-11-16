module main;
    reg [3:0] a,b;
    reg ci;
    wire [3:0]sum;
    add4 adder4(sum, co, a, b, ci);

    always@(sum or co)
        begin
            $display("time=%d:%b + %b + %b = %b, carry = %b\n", $time, a, b, ci, sum, co);
        end

    initial
        begin
            $dumpfile("waves.vcd");
            $dumpvars;
            a = 4'b0000; b = 4'b1000; ci = 1'b0;
            #5
            a = 4'b0000; b = 4'b1001; ci = 1'b1;
            #5
            a = 4'b1001; b = 4'b1001; ci = 1'b0;
            #5
            a = 4'b1111; b = 4'b1111; ci = 1'b1;
            #5
            $finish;
        end
endmodule
