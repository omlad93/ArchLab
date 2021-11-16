module mux1(a,b,select,result);
   input a;
   input b;
   input select;
   output result;

   assign result = select ? b : a ;
endmodule

module mux2(a,b,select,result);
   input a;
   input b;
   input select;
   output result;

   reg result;
   always @(a or b or select)
     begin
        if (select == 1) begin
            result = b;
         end
         else begin
            result = a;
         end
     end
endmodule
