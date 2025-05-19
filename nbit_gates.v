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
            nor2$ u_nand (
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
            nor3$ u_nand (
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
            nor4$ u_nand (
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
            or2$ u_nand (
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
            or3$ u_nand (
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
            or4$ u_nand (
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
            xor2$ u_nand (
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
            xnor2$ u_nand (
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
                .in(in[i]),
                .out(out[i])
            );
        end
    endgenerate
endmodule

module gp_generator (
    input a,
    input b,
    output g,
    output p
);
    and2$ hi(.in0(a),.in1(b),.out(g));
    xor2$ hi1(.in0(a),.in1(b),.out(p));
endmodule

module black_cell (
    input g_kj, p_kj,
    input g_ij, p_ij,
    output g_out, p_out
);

    wire intermediate;

    and2$ hi(.in0(p_kj),.in1(g_ij),.out(intermediate));
    or2$ hi1(.in0(g_kj),.in1(intermediate),.out(g_out));

    and2$ hi2(.in0(p_kj),.in1(p_kj),.out(p_out));
endmodule

module gray_cell (
    input g_kj, p_kj,
    input g_ij,
    output g_out
);
    wire intermediate;

    and2$ hi(.in0(p_kj),.in1(g_ij),.out(intermediate));
    or2$ hi1(.in0(g_kj),.in1(intermediate),.out(g_out));
endmodule

module kogge_stone_adder #(parameter N = 4)(
    input  [N-1:0] a,
    input  [N-1:0] b,
    output [N-1:0] sum,
    output carry_out
);
    wire [N-1:0] g, p;
    wire [N:0]   c; // Carry vector
    assign c[0] = 0;

    // Stage 0: Generate and Propagate
    genvar i;
    generate
        for (i = 0; i < N; i = i + 1) begin : GP_GEN
            gp_generator gp_u (
                .a(a[i]),
                .b(b[i]),
                .g(g[i]),
                .p(p[i])
            );
        end
    endgenerate

    // Prefix tree levels
    localparam stageNum = $clog2(N);
    wire [N-1:0] G [0:stageNum-1];
    wire [N-1:0] P [0:stageNum-1];

    // Initialize level 0
    generate
        for (i = 0; i < N; i = i + 1) begin : INIT_STAGE
            assign G[0][i] = g[i];
            assign P[0][i] = p[i];
        end
    endgenerate

    // Build the prefix tree
    genvar s, j;
    generate
        for (s = 1; s < stageNum; s = s + 1) begin : PREFIX_LEVEL
            for (j = 0; j < N; j = j + 1) begin : PREFIX_BITS
                if (j >= (1 << s)) begin
                    black_cell bc (
                        .g_kj(G[s-1][j]),
                        .p_kj(P[s-1][j]),
                        .g_ij(G[s-1][j - (1 << (s-1))]),
                        .p_ij(P[s-1][j - (1 << (s-1))]),
                        .g_out(G[s][j]),
                        .p_out(P[s][j])
                    );
                end else begin
                    assign G[s][j] = G[s-1][j];
                    assign P[s][j] = P[s-1][j];
                end
            end
        end
    endgenerate

    // Final carry generation using gray cells
    generate
        for (i = 0; i < N; i = i + 1) begin : CARRY_GEN
            if (i == 0)
                assign c[1] = g[0];
            else begin
                gray_cell gc (
                    .g_kj(G[stageNum-1][i]),
                    .p_kj(P[stageNum-1][i]),
                    .g_ij(c[0]),
                    .g_out(c[i+1])
                );
            end
        end
    endgenerate

    // Final sum computation
    generate
        xor2_nbit #(N+1) hi(
            .in0(p),
            .in1(c),
            .out(sum)
        );
    endgenerate

    assign carry_out = c[N];
endmodule


module main;
  // Parameter
    parameter N = 4;

    // Inputs
    reg [N-1:0] a;
    reg [N-1:0] b;

    // Output
    wire [N-1:0] sum;
    wire carry_out;

    // DUT (Device Under Test)
    kogge_stone_adder #(N) hi4(
        .a(a),
        .b(b),
        .sum(sum),
        .carry_out(carry_out)
    );

    initial begin
        $display("Time\ta\tb \tsum \t cout");
        $monitor("%0t\t%b\t%b\t%b\t%b", $time, a, b, sum, carry_out);

        // Apply test vectors
        a = 16'b1011; b = 16'b1110;
        a = 4'b1111; b = 4'b1111; #10;
        a = 4'b1010; b = 4'b1100; #10;
        a = 4'b1111; b = 4'b0000; #10;
        a = 4'b0101; b = 4'b1010; #10;

        $finish;
    end
endmodule
