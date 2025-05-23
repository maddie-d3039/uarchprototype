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
    overwritten_destination=0b000;
    //hardcode overwritten destination

endmodule

module icache_management_unit(
    input clk,
    input [31:0] address, input is_PC_new,
    input [31:0] data, input cache_hit, input boundary_cross,
    input receiving_data, input received_word, input request_id
    output invalidate, output icache_enable, output read_enable, output write_enable,
    output [511:0] write_data, output valid_data, output dirty_data, 
    output [31:0] VA_to_access,
    output boundary_cross, output boundary_cross_stall, output queue_full
);

//how to allocate an entry, how to activate one entry at a time
//to allocate a new entry, find an invalid entry and set relevant values there. If there
//are no open spots, you must stall

//to request an icache read, we need to set icache enable to 1, read enable to 1, va to access to address 

wire [7:0] valid;
wire [8:0] pen_input;
wire pen_valid;
pen_valid = ~(valid[0]&&valid[1]&&valid[2]&&valid[3]&&valid[4]&&valid[5]&&valid[6]&&valid[7]);
pen_input[0] = ~valid[0];
pen_input[0] = ~(valid[0]&&valid[1]);
pen_input[0] = ~(valid[1]&&valid[2]);
pen_input[0] = ~(valid[2]&&valid[3]);
pen_input[0] = ~(valid[3]&&valid[4]);
pen_input[0] = ~(valid[4]&&valid[5]);
pen_input[0] = ~(valid[5]&&valid[6]);
pen_input[0] = ~(valid[6]&&valid[7]);
pen_input[0] = ~valid[7];
//pass through pen and the output is which entry to allocate in
wire pen_output;

//combinational logic to allocate a read
//you allocate a read when pc changes. To allocate a read:
//set valid, set request type, set address

wire [7:0] type; wire [7:0] hit_or_miss; wire [7:0] write_finished; wire [7:0] requested;
wire [31:0] address_output [7:0];
genvar i;
    generate
        for(i = 0; i<8; i=i+1) begin
            wire data_was_received;
            wire correct_entry = (i==pen_output)&&pen_valid&&;
            wire allocate_enable = correct_entry&&is_PC_new&&clk;
            wire data_enable = correct_entry&&data_was_received&&clk&&receiving_data;
            wire deallocate = ((~type[i]) && hit_or_miss[i]) || (type[i] && write_finished[i]);
            

            dff16b$ entry_valid(.CLK(allocate_enable), .D(1), .Q(valid[i]), .QBAR(0), .CLR(deallocate), .PRE(0), .A());
            dff16b$ read_requested(.CLK(), .D(), .Q(requested), .QBAR(0), .CLR(), .PRE(0), .A());
            //request_type = 0 means its a read, request_type = 1 means its a write
            dff16b$ request_type(.CLK(allocate_enable), .D(), .Q(type[i]), .QBAR(0), .CLR(), .PRE(0), .A());
            dff16b$ waiting_on_result(.CLK(), .D(), .Q(), .QBAR(0), .CLR(), .PRE(0), .A());
            dff16b$ hit(.CLK(), .D(cache_hit), .Q(hit_or_miss[i]), .QBAR(0), .CLR(), .PRE(0), .A());
            dff16b$ memory_requested(.CLK(), .D(), .Q(), .QBAR(0), .CLR(), .PRE(0), .A());
            dff16b$ has_received_data(.CLK(), .D(), .Q(data_was_received), .QBAR(0), .CLR(), .PRE(0), .A());
            dff16b$ request_id(.CLK(), .D(), .Q(), .QBAR(0), .CLR(), .PRE(0), .A());
            dff16b$ write_issued(.CLK(), .D(), .Q(), .QBAR(0), .CLR(), .PRE(0), .A());
            dff16b$ boundary_cross(.CLK(), .D(), .Q(), .QBAR(0), .CLR(), .PRE(0), .A());
            dff16b$ write_completed(.CLK(), .D(), .Q(write_finished[i]), .QBAR(0), .CLR(), .PRE(0), .A());
            dff32b$ address(.CLK(allocate_enable), .D(address), .Q(address_output[i]), .QBAR(0), .CLR(), .PRE(0), .A());
            dff32b$ data0(.CLK(data_enable&&(received_word==0)), .D(data), .Q(write_data[0:31]), .QBAR(), .CLR(), .PRE(0), .A());
            dff32b$ data1(.CLK(data_enable&&(received_word==1)), .D(data), .Q(write_data[32:63]), .QBAR(), .CLR(), .PRE(0), .A());
            dff32b$ data2(.CLK(data_enable&&(received_word==2)), .D(data), .Q(write_data[64:95]), .QBAR(), .CLR(), .PRE(0), .A());
            dff32b$ data3(.CLK(data_enable&&(received_word==3)), .D(data), .Q(write_data[96:127]), .QBAR(), .CLR(), .PRE(0), .A());
            dff32b$ data4(.CLK(data_enable&&(received_word==4)), .D(data), .Q(write_data[128:159]), .QBAR(), .CLR(), .PRE(0), .A());
            dff32b$ data5(.CLK(data_enable&&(received_word==5)), .D(data), .Q(write_data[160:191]), .QBAR(), .CLR(), .PRE(0), .A());
            dff32b$ data6(.CLK(data_enable&&(received_word==6)), .D(data), .Q(write_data[192:223]), .QBAR(), .CLR(), .PRE(0), .A());
            dff32b$ data7(.CLK(data_enable&&(received_word==7)), .D(data), .Q(write_data[224:255]), .QBAR(), .CLR(), .PRE(0), .A());
            dff32b$ data8(.CLK(data_enable&&(received_word==8)), .D(data), .Q(write_data[256:287]), .QBAR(), .CLR(), .PRE(0), .A());
            dff32b$ data9(.CLK(data_enable&&(received_word==9)), .D(data), .Q(write_data[288:319]), .QBAR(), .CLR(), .PRE(0), .A());
            dff32b$ data10(.CLK(data_enable&&(received_word==10)), .D(data), .Q(write_data[320:351]), .QBAR(), .CLR(), .PRE(0), .A());
            dff32b$ data11(.CLK(data_enable&&(received_word==11)), .D(data), .Q(write_data[352:383]), .QBAR(), .CLR(), .PRE(0), .A());
            dff32b$ data12(.CLK(data_enable&&(received_word==12)), .D(data), .Q(write_data[384:415]), .QBAR(), .CLR(), .PRE(0), .A());
            dff32b$ data13(.CLK(data_enable&&(received_word==13)), .D(data), .Q(write_data[416:447]), .QBAR(), .CLR(), .PRE(0), .A());
            dff32b$ data14(.CLK(data_enable&&(received_word==14)), .D(data), .Q(write_data[448:479]), .QBAR(), .CLR(), .PRE(0), .A());
            dff32b$ data15(.CLK(data_enable&&(received_word==15)), .D(data), .Q(write_data[480:511]), .QBAR(), .CLR(), .PRE(0), .A());
        end
    endgenerate

endmodule
