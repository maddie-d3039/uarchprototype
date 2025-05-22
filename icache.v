module icache(
    input invalidate, input icache_enable,input read_enable, input write_enable, 
    input [31:0] write_data, input valid_data, input dirty_data,
    input [31:0] VA_to_access, input [23:0] physical_tag, input tag_write_enable,
    output [511:0] data_minibus, output [23:0] tag_minibus, output [1:0] metadata,
    output [31:0] overwritten_address, output [511:0] overwritten_data, output [2:0] overwritten_destination,
    output overwritten_valid
);
    //need to output all the information associated with the cache line that was just overwritten
    //we also need to convert all of the binary operators to actual gate level

    wire [1:0] idx = VA_to_access[7:6];
    wire [3:0] valid_bits;
    wire [3:0] dirty_bits;
    wire [3:0] not_valid_bits;
    wire [3:0] not_dirty_bits;

    wire[5:0] offset = VA_to_access[5:0];

    //data store
    genvar i;
        generate
            for (i = 0; i < 64; i=i+1) begin : DATA_CELLS
                wire write_enable_this_byte = write_enable && (i>=offset) && (i<=offset+3);
                //convert to gate logic        
                ram8b4w$ data_store(.A(idx),.DIN(write_data[(i-offset)*8+7:(i-offset)*8]),.OE(icache_enable),.WR(write_enable_this_byte),.DOUT(data_minibus[i*8+7:i*8]));    
            end
        endgenerate

    //tag store tag bits
    genvar j;
        generate
            for (j = 0; j < 3; j=j+1) begin : TAG_CELLS
                ram8b4w$ tag_store(.A(idx),.DIN(physical_tag[j*8+7:j*8]),.OE(icache_enable),.WR(tag_write_enable),.DOUT(tag_minibus[i*8+7:i*8]));
            end
        endgenerate

    //tag store metadata bits
    genvar k;
        generate
            for (k = 0; k < 4; k=k+1) begin : METADATA_CELL
                wire enable = write_enable && (idx==k); // need to rewrite with gates and nand
                //also need to put invalidate in the inverter and into r and s, noting that r and s are in negative logic
                latch$ valid(.d(valid_data), .en(enable), .q(valid_bits), .qbar(not_valid_bits), r(!invalidate), s(invalidate));
                latch$ dirty(.d(dirty_data), .en(enable), q(dirty_bits), qbar(not_dirty_bits), r(!invalidate), s(invalidate));
            end
        endgenerate

    metadata[1]=valid_bits[idx];
    metadata[0]=dirty_bits[idx];

    overwritten_valid = (write_enable) && (tag_minibus != physical_tag) && (metadata[1]) && (metadata[0]);
    overwritten_address[31:8] = tag_minibus;
    overwritte_address[7:6] = idx;
    overwritten_address[5:0] = 0b000000;
    //reconstruct overwritten address from tag minibus and its index
    overwritten_data=data_minibus;
    //get overwritten data from the data minibus
    overwritte_destination=0b000;
    //hardcode overwritten destination

endmodule

module icache_management_unit(
    input [31:0] address, input is_PC_new,
    input data, input hit, input boundary_cross,
    output invalidate, output icache_enable, output read_enable, output write_enable,
    output [31:0] write_data, output valid_data, output dirty_data, 
    output [31:0] VA_to_access, output [23:0] physical_tag, output tag_write_enable
);

//this module must keep track of in progress cache reads, cache writes, and prefetches

endmodule
