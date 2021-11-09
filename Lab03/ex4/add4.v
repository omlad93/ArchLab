`include "../ex3/fulladder.v"

module  add4( sum, co, a, b, ci);

  input   [3:0] a, b;
  input   ci;
  output  [3:0] sum;
  output  co;
  wire c0, c1, c2;
  
  // module fulladder( sum, co, a, b, ci);
  fulladder fa0(sum[0], c0, a[0], b[0], ci);
  fulladder fa1(sum[1], c1, a[1], b[1], c0);
  fulladder fa2(sum[2], c2, a[2], b[2], c1);
  fulladder fa3(sum[3], co, a[3], b[3], c2);


endmodule
