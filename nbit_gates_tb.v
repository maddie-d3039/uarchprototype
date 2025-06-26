`timescale 1ns/1ps

module kogge_stone_adder_tb;

    parameter N = 4;

    // Testbench signals
    reg  [N-1:0] a;
    reg  [N-1:0] b;
    wire [N-1:0] sum;
    wire         carry_out;

    // Instantiate the Kogge-Stone Adder
    kogge_stone_adder #(N) uut (
        .a(a),
        .b(b),
        .sum(sum),
        .carry_out(carry_out)
    );

    // Task to apply stimulus and display result
    task run_test(input [N-1:0] a_in, input [N-1:0] b_in);
        begin
            a = a_in;
            b = b_in;
            #10; // wait for signals to propagate
            $display("a = %b, b = %b --> sum = %b, carry_out = %b", a, b, sum, carry_out);
        end
    endtask

    // Test sequence
    initial begin
        $display("Starting Kogge-Stone Adder Testbench");

        run_test(4'b0000, 4'b0000); // 0 + 0
        run_test(4'b0001, 4'b0010); // 1 + 2
        run_test(4'b0101, 4'b0011); // 5 + 3
        run_test(4'b1111, 4'b0001); // 15 + 1 (overflow)
        run_test(4'b1010, 4'b0101); // 10 + 5
        run_test(4'b1111, 4'b1111); // 15 + 15

        $display("Testbench completed");
        $stop;
    end

endmodule
