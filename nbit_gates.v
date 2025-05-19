`celldefine
module  nand2$(out, in0, in1);
	input	in0, in1;
	output	out;

	nand (out, in0, in1);

	specify
            (in0 *> out) = (0.18:0.2:0.22, 0.18:0.2:0.22);
            (in1 *> out) = (0.18:0.2:0.22, 0.18:0.2:0.22);
	endspecify
endmodule
`endcelldefine

`celldefine
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
`endcelldefine

`celldefine
module  nand4$(out, in0, in1, in2, in3);
	input in0, in1, in2, in3;
	output out;

	nand (out, in0, in1, in2, in3);

	specify
            (in0 *> out) = (0.22:0.25:0.28, 0.22:0.25:0.28);
            (in1 *> out) = (0.22:0.25:0.28, 0.22:0.25:0.28);
            (in2 *> out) = (0.22:0.25:0.28, 0.22:0.25:0.28);
            (in3 *> out) = (0.22:0.25:0.28, 0.22:0.25:0.28);
	endspecify
endmodule
`endcelldefine

`celldefine
module  and2$(out, in0, in1);
	input in0, in1;
	output out;

	and (out, in0, in1);

	specify
            (in0 *> out) = (0.32:0.35:0.38, 0.32:0.35:0.38);
            (in1 *> out) = (0.32:0.35:0.38, 0.32:0.35:0.38);
	endspecify
endmodule
`endcelldefine

`celldefine
module  and3$(out, in0, in1, in2);
	input	in0, in1, in2;
	output	out;

	and (out, in0, in1, in2);

	specify
            (in0 *> out) = (0.32:0.35:0.38, 0.32:0.35:0.38);
            (in1 *> out) = (0.32:0.35:0.38, 0.32:0.35:0.38);
            (in2 *> out) = (0.32:0.35:0.38, 0.32:0.35:0.38);
	endspecify
endmodule
`endcelldefine

`celldefine
module  and4$(out, in0, in1, in2, in3);
	input in0, in1, in2, in3;
	output out;

	and (out, in0, in1, in2, in3);

	specify
            (in0 *> out) = (0.36:0.40:0.44, 0.36:0.40:0.44);
            (in1 *> out) = (0.36:0.40:0.44, 0.36:0.40:0.44);
            (in2 *> out) = (0.36:0.40:0.44, 0.36:0.40:0.44);
            (in3 *> out) = (0.36:0.40:0.44, 0.36:0.40:0.44);
	endspecify
endmodule
`endcelldefine

`celldefine
module  nor2$(out, in0, in1);
	input in0, in1;
	output out;

	nor (out, in0, in1);

	specify
            (in0 *> out) = (0.18:0.2:0.22, 0.18:0.2:0.22);
            (in1 *> out) = (0.18:0.2:0.22, 0.18:0.2:0.22);
	endspecify
endmodule
`endcelldefine

`celldefine
module  nor3$(out, in0, in1, in2);
	input in0, in1, in2;
	output out;

	nor (out, in0, in1, in2);

	specify
            (in0 *> out) = (0.22:0.25:0.28, 0.22:0.25:0.28);
            (in1 *> out) = (0.22:0.25:0.28, 0.22:0.25:0.28);
            (in2 *> out) = (0.22:0.25:0.28, 0.22:0.25:0.28);
	endspecify
endmodule
`endcelldefine

`celldefine
module  nor4$(out, in0, in1, in2, in3);
	input in0, in1, in2, in3;
	output out;

	nor (out, in0, in1, in2, in3);

	specify
            (in0 *> out) = (0.32:0.35:0.38, 0.32:0.35:0.38);
            (in1 *> out) = (0.32:0.35:0.38, 0.32:0.35:0.38);
            (in2 *> out) = (0.32:0.35:0.38, 0.32:0.35:0.38);
            (in3 *> out) = (0.32:0.35:0.38, 0.32:0.35:0.38);
	endspecify
endmodule
`endcelldefine

                                             
`celldefine
module  or2$(out, in0, in1);
	input in0, in1;
	output out;

	or (out, in0, in1);

	specify
            (in0 *> out) = (0.32:0.35:0.38, 0.32:0.35:0.38);
            (in1 *> out) = (0.32:0.35:0.38, 0.32:0.35:0.38);
	endspecify
endmodule
`endcelldefine


//-----------------------------------------------------------
`celldefine
module  or3$(out, in0, in1, in2);
	input in0, in1, in2;
	output out;

	or (out, in0, in1, in2);

	specify
            (in0 *> out) = (0.36:0.40:0.44, 0.36:0.40:0.44);
            (in1 *> out) = (0.36:0.40:0.44, 0.36:0.40:0.44);
            (in2 *> out) = (0.36:0.40:0.44, 0.36:0.40:0.44);
	endspecify
endmodule
`endcelldefine

`celldefine
module  or4$(out, in0, in1, in2, in3);
	input in0, in1, in2, in3;
	output out;

	or (out, in0, in1, in2, in3);

	specify
            (in0 *> out) = (0.46:0.50:0.54, 0.46:0.50:0.54);
            (in1 *> out) = (0.46:0.50:0.54, 0.46:0.50:0.54);
            (in2 *> out) = (0.46:0.50:0.54, 0.46:0.50:0.54);
            (in3 *> out) = (0.46:0.50:0.54, 0.46:0.50:0.54);
	endspecify
endmodule
`endcelldefine



//-----------------------------------------------------------
`celldefine
module  xor2$ (out, in0, in1);
	input in0, in1;
	output out;

	xor (out, in0, in1);

	specify
            (in0 *> out) = (0.27:0.30:0.33, 0.27:0.30:0.33);
            (in1 *> out) = (0.27:0.30:0.33, 0.27:0.30:0.33);
	endspecify
endmodule
`endcelldefine


//-----------------------------------------------------------
`celldefine
module  xnor2$ (out, in0, in1);
	input in0, in1;
	output out;

	xnor (out, in0, in1);

	specify
            (in0 *> out) = (0.22:0.25:0.28, 0.22:0.25:0.28);
            (in1 *> out) = (0.22:0.25:0.28, 0.22:0.25:0.28);
	endspecify
endmodule
`endcelldefine


//-----------------------------------------------------------
// Timing specs taken from page 1-47.
`celldefine
module  inv1$ (out, in);
	input in;
	output out;

	not (out, in);

	specify
            (in *> out) = (0.13:0.15:0.17, 0.13:0.15:0.17);
	endspecify
endmodule
`endcelldefine

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

module nand4_nbit #(parameter N = 8) (
    input  [N-1:0] in0,
    input  [N-1:0] in1,
    input  [N-1:0] in2,
    input  [N-1:0] in3,
    output [N-1:0] out
);
    genvar i;
    generate
        for (i = 0; i < N; i = i + 1) begin : gen_nand
            nand4$ u_nand (
                .in0(in0[i]),
                .in1(in1[i]),
                .in2(in2[i]),
                .in3(in3[i]),
                .out(out[i])
            );
        end
    endgenerate
endmodule


module and2_nbit #(parameter N = 8) (
    input  [N-1:0] in0,
    input  [N-1:0] in1,
    output [N-1:0] out
);
    genvar i;
    generate
        for (i = 0; i < N; i = i + 1) begin : gen_nand
            and2$ u_nand (
                .in0(in0[i]),
                .in1(in1[i]),
                .out(out[i])
            );
        end
    endgenerate
endmodule

module and3_nbit #(parameter N = 8) (
    input  [N-1:0] in0,
    input  [N-1:0] in1,
    input  [N-1:0] in2,
    output [N-1:0] out
);
    genvar i;
    generate
        for (i = 0; i < N; i = i + 1) begin : gen_nand
            and3$ u_nand (
                .in0(in0[i]),
                .in1(in1[i]),
                .in2(in2[i]),
                .out(out[i])
            );
        end
    endgenerate
endmodule

module and4_nbit #(parameter N = 8) (
    input  [N-1:0] in0,
    input  [N-1:0] in1,
    input  [N-1:0] in2,
    input  [N-1:0] in3,
    output [N-1:0] out
);
    genvar i;
    generate
        for (i = 0; i < N; i = i + 1) begin : gen_nand
            and4$ u_nand (
                .in0(in0[i]),
                .in1(in1[i]),
                .in2(in2[i]),
                .in3(in3[i]),
                .out(out[i])
            );
        end
    endgenerate
endmodule

module nor2_nbit #(parameter N = 8) (
    input  [N-1:0] in0,
    input  [N-1:0] in1,
    output [N-1:0] out
);
    genvar i;
    generate
        for (i = 0; i < N; i = i + 1) begin : gen_nand
            and2$ u_nand (
                .in0(in0[i]),
                .in1(in1[i]),
                .out(out[i])
            );
        end
    endgenerate
endmodule

module nor3_nbit #(parameter N = 8) (
    input  [N-1:0] in0,
    input  [N-1:0] in1,
    input  [N-1:0] in2,
    output [N-1:0] out
);
    genvar i;
    generate
        for (i = 0; i < N; i = i + 1) begin : gen_nand
            and3$ u_nand (
                .in0(in0[i]),
                .in1(in1[i]),
                .in2(in2[i]),
                .out(out[i])
            );
        end
    endgenerate
endmodule

module nor4_nbit #(parameter N = 8) (
    input  [N-1:0] in0,
    input  [N-1:0] in1,
    input  [N-1:0] in2,
    input  [N-1:0] in3,
    output [N-1:0] out
);
    genvar i;
    generate
        for (i = 0; i < N; i = i + 1) begin : gen_nand
            and4$ u_nand (
                .in0(in0[i]),
                .in1(in1[i]),
                .in2(in2[i]),
                .in3(in3[i]),
                .out(out[i])
            );
        end
    endgenerate
endmodule

module or2_nbit #(parameter N = 8) (
    input  [N-1:0] in0,
    input  [N-1:0] in1,
    output [N-1:0] out
);
    genvar i;
    generate
        for (i = 0; i < N; i = i + 1) begin : gen_nand
            and2$ u_nand (
                .in0(in0[i]),
                .in1(in1[i]),
                .out(out[i])
            );
        end
    endgenerate
endmodule

module or3_nbit #(parameter N = 8) (
    input  [N-1:0] in0,
    input  [N-1:0] in1,
    input  [N-1:0] in2,
    output [N-1:0] out
);
    genvar i;
    generate
        for (i = 0; i < N; i = i + 1) begin : gen_nand
            and3$ u_nand (
                .in0(in0[i]),
                .in1(in1[i]),
                .in2(in2[i]),
                .out(out[i])
            );
        end
    endgenerate
endmodule

module or4_nbit #(parameter N = 8) (
    input  [N-1:0] in0,
    input  [N-1:0] in1,
    input  [N-1:0] in2,
    input  [N-1:0] in3,
    output [N-1:0] out
);
    genvar i;
    generate
        for (i = 0; i < N; i = i + 1) begin : gen_nand
            and4$ u_nand (
                .in0(in0[i]),
                .in1(in1[i]),
                .in2(in2[i]),
                .in3(in3[i]),
                .out(out[i])
            );
        end
    endgenerate
endmodule

module xor2_nbit #(parameter N = 8) (
    input  [N-1:0] in0,
    input  [N-1:0] in1,
    output [N-1:0] out
);
    genvar i;
    generate
        for (i = 0; i < N; i = i + 1) begin : gen_nand
            and2$ u_nand (
                .in0(in0[i]),
                .in1(in1[i]),
                .out(out[i])
            );
        end
    endgenerate
endmodule

module xnor2_nbit #(parameter N = 8) (
    input  [N-1:0] in0,
    input  [N-1:0] in1,
    output [N-1:0] out
);
    genvar i;
    generate
        for (i = 0; i < N; i = i + 1) begin : gen_nand
            and2$ u_nand (
                .in0(in0[i]),
                .in1(in1[i]),
                .out(out[i])
            );
        end
    endgenerate
endmodule

module inv1_nbit #(parameter N = 8) (
    input  [N-1:0] in,
    output [N-1:0] out
);
    genvar i;
    generate
        for (i = 0; i < N; i = i + 1) begin : gen_nand
            inv1$ u_nand (
                .in0(in[i]),
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

