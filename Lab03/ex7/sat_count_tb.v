module main;

reg clk, reset, branch, taken;
reg ans, fail;
wire prediction;

sat_count counter(clk, reset, branch, taken, prediction);

always #5 clk = ~clk;

initial begin
	$dumpfile("waves.vcd");
	$dumpvars;
    clk = 0;
    ans = 1;
    fail = 1;
    branch = 0;
    taken = 0;

    reset = 1;
    #20;
    reset = 0;

    // FIRST TEST
    ans = ans & (~prediction); //prediction should be 0 --> ans should be 1
    fail = fail & ans;
    if (ans) begin
        $display("PASS first test");
    end
    #20

    // SECOND TEST
    taken = 1;
    branch = 1;
    ans = 1;
    #10;
    ans = ans & (~prediction); //prediction should be 0 --> ans should be 1
    #10;
    ans = ans & (prediction); //prediction should be 1 --> ans should be 1
    #10;
    ans = ans & (prediction); //prediction should be 1 --> ans should be 1
    #20;
    ans = ans & (prediction); //prediction should be 1 --> ans should be 1

    fail = fail & ans;

    if (ans) begin
        $display("PASS second test");
    end
    #20

    // THIRD TEST
    taken = 0;
    branch = 1;
    ans = 1;
    #10;
    ans = ans & (prediction); //prediction should be 1 --> ans should be 1
    #10;
    ans = ans & (~prediction); //prediction should be 0 --> ans should be 1
    #10;
    ans = ans & (~prediction); //prediction should be 0 --> ans should be 1
    #20;
    ans = ans & (~prediction); //prediction should be 0 --> ans should be 1

    fail = fail & ans;

    if (ans) begin
        $display("PASS third test");
    end

    // FOURTH TEST
    taken = 0;
    branch = 0;
    ans = 1;
    #20;
    ans = ans & (~prediction); //prediction should be 0 --> ans should be 1

    fail = fail & ans;

    if (ans) begin
        $display("PASS fourth test");
    end

    #10

    if (fail) begin
        $display("PASSED ALL TESTS");
    end

    #10
    $finish;
end
endmodule
