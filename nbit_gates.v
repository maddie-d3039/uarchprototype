/* 
 * Do not change Module name 
*/

module  nand2$(out, in0, in1);
	input	in0, in1;
	output	out;

	nand (out, in0, in1);

	specify
            (in0 *> out) = (0.18:0.2:0.22, 0.18:0.2:0.22);
            (in1 *> out) = (0.18:0.2:0.22, 0.18:0.2:0.22);
	endspecify
endmodule

module  nand3$(out, in0, in1, in2);
	input	in0, in1, in2;
	output	out;

	nand (out, in0, in1, in2);

	specify
            (in0 *> out) = (0.18:0.2:0.22, 0.18:0.2:0.22);
            (in1 *> out) = (0.18:0.2:0.22, 0.18:0.2:0.22);
            (in2 *> out) = (0.18:0.2:0.22, 0.18:0.2:0.22);
	endspecify
endmodule

module nand2_nbit #(parameter N = 8) (
    input  [N-1:0] in0,
    input  [N-1:0] in1,
    output [N-1:0] out
);
    genvar i;
    generate
        for (i = 0; i < N; i = i + 1) begin : gen_nand
            nand2$ u_nand (
                .in0(in0[i]),
                .in1(in1[i]),
                .out(out[i])
            );
        end
    endgenerate
endmodule

module nand3_nbit #(parameter N = 8) (
    input  [N-1:0] in0,
    input  [N-1:0] in1,
    input  [N-1:0] in2,
    output [N-1:0] out
);
    genvar i;
    generate
        for (i = 0; i < N; i = i + 1) begin : gen_nand
            nand3$ u_nand (
                .in0(in0[i]),
                .in1(in1[i]),
                .in2(in2[i]),
                .out(out[i])
            );
        end
    endgenerate
endmodule

module main;
  // Parameter
    parameter N = 4;

    // Inputs
    reg [N-1:0] in0;
    reg [N-1:0] in1;
    reg [N-1:0] in2;

    // Output
    wire [N-1:0] out;

    // DUT (Device Under Test)
    nand3_nbit #(N) hi(
        .in0(in0),
        .in1(in1),
        .in2(in2),
        .out(out)
    );

    initial begin
        $display("Time\tin0\tin1 \tin2 \tout");
        $monitor("%0t\t%b\t%b\t%b\t%b", $time, in0, in1, in2, out);

        // Apply test vectors
        in0 = 16'b1011; in1 = 16'b1110; in2 = 16'b1101; #10;
        in0 = 4'b1111; in1 = 4'b1111; #10;
        in0 = 4'b1010; in1 = 4'b1100; #10;
        in0 = 4'b1111; in1 = 4'b0000; #10;
        in0 = 4'b0101; in1 = 4'b1010; #10;

        $finish;
    end
endmodule

