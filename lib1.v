//-----------------------------------------------------------
// Library parts I
// ---------------
// EE382N-14945, Spring 2000.
// (Modified from those provided by Cascade Design Automation Corp).
// All module parts' name are appended with a "$" character.
// This part of the library consists of the following gates and flip flop:
//
// nand2$	-  2 inputs nand
// nand3$	-  3 inputs nand
// nand4$	-  4 inputs nand
// and2$	-  2 inputs and
// and3$	-  3 inputs and
// and4$	-  4 inputs and
// nor2$	-  2 inputs nor 
// nor3$	-  3 inputs nor 
// nor4$	-  4 inputs nor 
// or2$		-  2 inputs or 
// or3$		-  3 inputs or 
// or4$		-  4 inputs or 
// xor2$	-  2 inputs xor 
// xnor2$	-  2 inputs xnor 
// inv1$	-  1 input inverter 
// dff$		-  edge-triggered D-ff with set/reset
//
// Timing specs are taken from page 1-47.
//-----------------------------------------------------------
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



//-----------------------------------------------------------
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

   

//-----------------------------------------------------------
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


//-----------------------------------------------------------
// Edge-Triggered D-ff
// Timing specs taken from page 1-10.
primitive table_dffq(q, d, clk, s, r);
    input d, clk, s, r;
    output q;
    reg q;

    table
//  d  clk  s  r : q  : q+
	?   ?   0  0 : ?  : x;

	?   ?   1  0 : ?  : 0; //clear logic
	?   ?   1  * : 0  : 0;

	?   ?   0  1 : ?  : 1; //preset logic
	?   ?   *  1 : 1  : 1;

    1   r   1  1 : ?  : 1; //normal clocking
    0   r   1  1 : ?  : 0;
	
	?   f   1  1 : ?  : -; //ignore negative edge of clock
	*   ?   1  1 : ?  : -; //ignore data changes on a steady clock
    endtable
endprimitive

`celldefine
module  dff$(clk, d, q, qbar, r, s);
	input  s, d, r, clk;
	output qbar, q;

	table_dffq(q, d, clk, s, r);
	not(qbar, q);

	specify
            (clk *> q)    = (0.06:0.08:0.10);
	    // assume t_plh(Q) = t_plh(QBAR)
	    //        t_phl(Q) = t_phl(QBAR)
            (clk *> qbar) = (0.06:0.08:0.10);
            (s *> q)      = (0.36:0.40:0.44);
            (s *> qbar)   = (0.36:0.40:0.44);
            (r *> q)      = (0.32:0.35:0.38);
            (r *> qbar)   = (0.32:0.35:0.38);
            $setup(d, edge[01,x1] clk,0.18:0.2:0.22);
            $width(edge[01,x1] s,     0.32:0.35:0.38);
            $width(edge[01,x1] r,     0.32:0.35:0.38);
            $width(edge[01,x1] clk,   0.32:0.35:0.38);
	endspecify
endmodule
`endcelldefine

