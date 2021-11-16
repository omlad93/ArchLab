module parity(clk, in, reset, out);

  input clk, in, reset;
  output out;

  reg 	  out;
  reg 	  state;

  localparam zero = 0;
  localparam one  = 1;

  always @(posedge clk)
    begin
	    if (reset)
	      state <= zero;
	    else
	      case (state)
	        0: state <= in;         // if state is 0 change state to input  (xor(0,in) = in )
          1: state <= ~in;         // if state is 1 change state to ~input (xor(1,in) = ~in)
          default: state <= one;  // default is even (start at zero)
	      endcase
    end

  always @(state) //update output
    begin
	    case (state)
	      0: out <= zero;
        1: out <= one;
        default: out <= one;
	    endcase
    end

endmodule



