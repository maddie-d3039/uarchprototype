module gp_generator (
    input a,
    input b,
    output g,
    output p
);
    assign g = a & b;
    assign p = a ^ b;
endmodule

module black_cell (
    input g_kj, p_kj,
    input g_ij, p_ij,
    output g_out, p_out
);
    assign g_out = g_kj | (p_kj & g_ij);
    assign p_out = p_kj & p_ij;
endmodule

module gray_cell (
    input g_kj, p_kj,
    input g_ij,
    output g_out
);
    assign g_out = g_kj | (p_kj & g_ij);
endmodule

module kogge_stone_adder #(parameter N = 4)(
    input  [N-1:0] a,
    input  [N-1:0] b,
    output [N-1:0] sum,
    output         carry_out
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
    localparam STAGES = $clog2(N);
    wire [N-1:0] G [0:STAGES-1];
    wire [N-1:0] P [0:STAGES-1];

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
        for (s = 1; s < STAGES; s = s + 1) begin : PREFIX_LEVEL
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
                    .g_kj(G[STAGES-1][i]),
                    .p_kj(P[STAGES-1][i]),
                    .g_ij(c[0]),
                    .g_out(c[i+1])
                );
            end
        end
    endgenerate

    // Final sum computation
    generate
        for (i = 0; i < N; i = i + 1) begin : SUM_GEN
            assign sum[i] = p[i] ^ c[i];
        end
    endgenerate

    assign carry_out = c[N];
endmodule
