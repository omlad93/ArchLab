module fulladder( sum, co, a, b, ci);

  input   a, b, ci;
  output  sum, co;

  wire x,y,z;

  xor(x, a, b);
  xor(sum, x, ci);
  and(y, x, ci);
  and(z, a, b);
  or(co, y, z);
  

endmodule
