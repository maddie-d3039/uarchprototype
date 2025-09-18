#include "inc/memory_controller.h"
void memory_controller::operate{
       // will need to probe the metadatabus to see the current state to see what itll need to do this cycle

    // if serializer is sending data, call the deserializer inserter
    if (metadata_bus.is_serializer_sending_data == TRUE)
    {
        deserializer_inserter();
    }

    // handle incoming read requests
    if (metadata_bus.is_mshr_sending_addr == TRUE)
    {
        int bkbits = get_bank_bits(metadata_bus.mshr_address);
        addresses_to_banks[bkbits] = metadata_bus.mshr_address;
        addr_to_bank_cycle[bkbits] = 0;
        reqID_to_bank[bkbits] = metadata_bus.to_mem_request_ID;
        metadata_bus.bank_status[bkbits] = 3;
        metadata_bus.bank_destinations[bkbits] = metadata_bus.origin;
    }

    // check deserializer to start any stores if possible
    if (metadata_bus.deserializer_can_store == TRUE &&
        deserializer.entries[metadata_bus.deserializer_next_entry].receiving_data_from_data_bus == FALSE &&
        deserializer.entries[metadata_bus.deserializer_next_entry].writing_data_to_DRAM == FALSE && deserializer.entries[metadata_bus.deserializer_next_entry].valid == TRUE)
    {
        int bkbits = get_bank_bits(deserializer.entries[metadata_bus.deserializer_next_entry].address);
        deserializer.entries[metadata_bus.deserializer_next_entry].writing_data_to_DRAM = TRUE;
        metadata_bus.bank_status[bkbits] = 2;
        addr_to_bank_cycle[bkbits] = 0;
        entry_to_bank[bkbits] = metadata_bus.deserializer_next_entry;
        deserializer.entries[metadata_bus.deserializer_next_entry].old_bits = -1; // this causes a new next entry to be selected so a not busy bank can be stored into if need be
    }

    // iterate through the banks
    for (int i = 0; i < banks_in_DRAM; i++)
    {
        // handle in progress read requests
        if (metadata_bus.bank_status[i] == 1)
        {
            if (addr_to_bank_cycle[i] == cycles_to_access_bank)
            {
                // initiate load send to cpu
                if ((metadata_bus.is_serializer_sending_data == FALSE) && (metadata_bus.serializer_available == TRUE) && (metadata_bus.receive_enable == FALSE))
                {
                    metadata_bus.receive_enable = TRUE;
                    metadata_bus.burst_counter = 0;
                    metadata_bus.next_burst_counter = 0;
                    metadata_bus.store_address = addresses_to_banks[i];
                    metadata_bus.to_cpu_request_ID = reqID_to_bank[i];
                    metadata_bus.destination = metadata_bus.bank_destinations[i];

                    data_bus.byte_wires[0] = dram.banks[get_bank_bits(addresses_to_banks[i])].rows[get_row_bits(addresses_to_banks[i])].columns[get_column_bits(addresses_to_banks[i])].bytes[0];
                    data_bus.byte_wires[1] = dram.banks[get_bank_bits(addresses_to_banks[i])].rows[get_row_bits(addresses_to_banks[i])].columns[get_column_bits(addresses_to_banks[i])].bytes[1];
                    data_bus.byte_wires[2] = dram.banks[get_bank_bits(addresses_to_banks[i])].rows[get_row_bits(addresses_to_banks[i])].columns[get_column_bits(addresses_to_banks[i])].bytes[2];
                    data_bus.byte_wires[3] = dram.banks[get_bank_bits(addresses_to_banks[i])].rows[get_row_bits(addresses_to_banks[i])].columns[get_column_bits(addresses_to_banks[i])].bytes[3];

                    addr_to_bank_cycle[i]++;
                }
                else
                {
                    continue; // wait until you can send stuff back
                }
            } // continue load send to cpu
            else if (addr_to_bank_cycle[i] > cycles_to_access_bank)
            {
                metadata_bus.burst_counter = metadata_bus.next_burst_counter;

                data_bus.byte_wires[0] = dram.banks[get_bank_bits(addresses_to_banks[i])].rows[get_row_bits(addresses_to_banks[i])].columns[get_column_bits(addresses_to_banks[i])].bytes[metadata_bus.burst_counter * 4 + 0];
                data_bus.byte_wires[1] = dram.banks[get_bank_bits(addresses_to_banks[i])].rows[get_row_bits(addresses_to_banks[i])].columns[get_column_bits(addresses_to_banks[i])].bytes[metadata_bus.burst_counter * 4 + 1];
                data_bus.byte_wires[2] = dram.banks[get_bank_bits(addresses_to_banks[i])].rows[get_row_bits(addresses_to_banks[i])].columns[get_column_bits(addresses_to_banks[i])].bytes[metadata_bus.burst_counter * 4 + 2];
                data_bus.byte_wires[3] = dram.banks[get_bank_bits(addresses_to_banks[i])].rows[get_row_bits(addresses_to_banks[i])].columns[get_column_bits(addresses_to_banks[i])].bytes[metadata_bus.burst_counter * 4 + 3];
                if (metadata_bus.next_burst_counter == 4)
                {
                    // end it
                    metadata_bus.bank_status[i] = 0;
                    metadata_bus.burst_counter = 0;
                    metadata_bus.receive_enable = FALSE;
                    continue;
                }
            }
            else
            {
                addr_to_bank_cycle[i]++;
            }
        }
        else if (metadata_bus.bank_status[i] == 2)
        { // handle in progress store requests, assume it takes 5 cycles to open the row and then just paste the data in it
            if (addr_to_bank_cycle[i] == cycles_to_access_bank)
            {
                dram.banks[i].rows[get_row_bits(deserializer.entries[entry_to_bank[i]].address)].columns[get_column_bits(deserializer.entries[entry_to_bank[i]].address)].bytes[0] = deserializer.entries[entry_to_bank[i]].data[0];
                dram.banks[i].rows[get_row_bits(deserializer.entries[entry_to_bank[i]].address)].columns[get_column_bits(deserializer.entries[entry_to_bank[i]].address)].bytes[1] = deserializer.entries[entry_to_bank[i]].data[1];
                dram.banks[i].rows[get_row_bits(deserializer.entries[entry_to_bank[i]].address)].columns[get_column_bits(deserializer.entries[entry_to_bank[i]].address)].bytes[2] = deserializer.entries[entry_to_bank[i]].data[2];
                dram.banks[i].rows[get_row_bits(deserializer.entries[entry_to_bank[i]].address)].columns[get_column_bits(deserializer.entries[entry_to_bank[i]].address)].bytes[3] = deserializer.entries[entry_to_bank[i]].data[3];
                dram.banks[i].rows[get_row_bits(deserializer.entries[entry_to_bank[i]].address)].columns[get_column_bits(deserializer.entries[entry_to_bank[i]].address) + 1].bytes[0] = deserializer.entries[entry_to_bank[i]].data[4];
                dram.banks[i].rows[get_row_bits(deserializer.entries[entry_to_bank[i]].address)].columns[get_column_bits(deserializer.entries[entry_to_bank[i]].address) + 1].bytes[1] = deserializer.entries[entry_to_bank[i]].data[5];
                dram.banks[i].rows[get_row_bits(deserializer.entries[entry_to_bank[i]].address)].columns[get_column_bits(deserializer.entries[entry_to_bank[i]].address) + 1].bytes[2] = deserializer.entries[entry_to_bank[i]].data[6];
                dram.banks[i].rows[get_row_bits(deserializer.entries[entry_to_bank[i]].address)].columns[get_column_bits(deserializer.entries[entry_to_bank[i]].address) + 1].bytes[3] = deserializer.entries[entry_to_bank[i]].data[7];
                dram.banks[i].rows[get_row_bits(deserializer.entries[entry_to_bank[i]].address)].columns[get_column_bits(deserializer.entries[entry_to_bank[i]].address) + 2].bytes[0] = deserializer.entries[entry_to_bank[i]].data[8];
                dram.banks[i].rows[get_row_bits(deserializer.entries[entry_to_bank[i]].address)].columns[get_column_bits(deserializer.entries[entry_to_bank[i]].address) + 2].bytes[1] = deserializer.entries[entry_to_bank[i]].data[9];
                dram.banks[i].rows[get_row_bits(deserializer.entries[entry_to_bank[i]].address)].columns[get_column_bits(deserializer.entries[entry_to_bank[i]].address) + 2].bytes[2] = deserializer.entries[entry_to_bank[i]].data[10];
                dram.banks[i].rows[get_row_bits(deserializer.entries[entry_to_bank[i]].address)].columns[get_column_bits(deserializer.entries[entry_to_bank[i]].address) + 2].bytes[3] = deserializer.entries[entry_to_bank[i]].data[11];
                dram.banks[i].rows[get_row_bits(deserializer.entries[entry_to_bank[i]].address)].columns[get_column_bits(deserializer.entries[entry_to_bank[i]].address) + 3].bytes[0] = deserializer.entries[entry_to_bank[i]].data[12];
                dram.banks[i].rows[get_row_bits(deserializer.entries[entry_to_bank[i]].address)].columns[get_column_bits(deserializer.entries[entry_to_bank[i]].address) + 3].bytes[1] = deserializer.entries[entry_to_bank[i]].data[13];
                dram.banks[i].rows[get_row_bits(deserializer.entries[entry_to_bank[i]].address)].columns[get_column_bits(deserializer.entries[entry_to_bank[i]].address) + 3].bytes[2] = deserializer.entries[entry_to_bank[i]].data[14];
                dram.banks[i].rows[get_row_bits(deserializer.entries[entry_to_bank[i]].address)].columns[get_column_bits(deserializer.entries[entry_to_bank[i]].address) + 3].bytes[3] = deserializer.entries[entry_to_bank[i]].data[15];

                metadata_bus.bank_status[i] = 0;
                deserializer.entries[entry_to_bank[i]].valid = FALSE;
            }
            else
            {
                addr_to_bank_cycle[i]++;
            }
        }
        else if (metadata_bus.bank_status[i] == 3)
        {
            // attempt revival by searching through deserializer for an address match
            // if revival succeeds, modify deserializer entry such that it'll send the data through the data bus
            // if revival fails, change status to 1
            bool revival_success = false;
            for (int j = 0; j < num_deserializer_entries; j++)
            {
                if (deserializer.entries[j].valid == TRUE && (deserializer.entries[j].address == addresses_to_banks[i]))
                {
                    deserializer.entries[j].revival_destination = metadata_bus.bank_destinations[i];
                    deserializer.entries[j].revival_lock = 1;
                    deserializer.entries[j].revival_reqID = reqID_to_bank[i];
                    metadata_bus.bank_status[i] = 0;
                    revival_success = true;
                    break;
                }
            }
            if (revival_success == false)
            {
                metadata_bus.bank_status[i] = 1;
            }
        }
    }

    // stage revivals here, only one revival can be staged in a singular cycle and they take 4 cycles to burst
    for (int i = 0; i < num_deserializer_entries; i++)
    {
        if ((metadata_bus.revival_in_progress == FALSE) &&
            (deserializer.entries[i].revival_lock == TRUE) &&
            (metadata_bus.is_serializer_sending_data == FALSE) &&
            (metadata_bus.serializer_available == TRUE) &&
            (metadata_bus.receive_enable == FALSE))
        {
            metadata_bus.revival_in_progress = TRUE;
            metadata_bus.receive_enable = TRUE;
            metadata_bus.current_revival_entry = i;
            metadata_bus.burst_counter = 0;
            metadata_bus.destination = deserializer.entries[metadata_bus.current_revival_entry].revival_destination;
            metadata_bus.to_cpu_request_ID = deserializer.entries[metadata_bus.current_revival_entry].revival_reqID;

            metadata_bus.store_address = deserializer.entries[metadata_bus.current_revival_entry].address;
            data_bus.byte_wires[0] = deserializer.entries[metadata_bus.current_revival_entry].data[0];
            data_bus.byte_wires[1] = deserializer.entries[metadata_bus.current_revival_entry].data[1];
            data_bus.byte_wires[2] = deserializer.entries[metadata_bus.current_revival_entry].data[2];
            data_bus.byte_wires[3] = deserializer.entries[metadata_bus.current_revival_entry].data[3];

            return;
        }
        else if (metadata_bus.revival_in_progress == TRUE)
        {

            metadata_bus.burst_counter = metadata_bus.next_burst_counter;

            data_bus.byte_wires[0] = deserializer.entries[metadata_bus.current_revival_entry].data[metadata_bus.burst_counter * 4 + 0];
            data_bus.byte_wires[1] = deserializer.entries[metadata_bus.current_revival_entry].data[metadata_bus.burst_counter * 4 + 1];
            data_bus.byte_wires[2] = deserializer.entries[metadata_bus.current_revival_entry].data[metadata_bus.burst_counter * 4 + 2];
            data_bus.byte_wires[3] = deserializer.entries[metadata_bus.current_revival_entry].data[metadata_bus.burst_counter * 4 + 3];

            if (metadata_bus.burst_counter == 3)
            {
                metadata_bus.burst_counter = 0;
                metadata_bus.receive_enable = FALSE;
                metadata_bus.revival_in_progress = FALSE;
                deserializer.entries[metadata_bus.current_revival_entry].revival_lock = 0;
                continue;
            }
        }
    }
}

// called upon eviction from cache/tlb
void serializer_inserter(int address, int data[cache_line_size])
{
    for (int i = 0; i < num_serializer_entries; i++)
    {
        if (serializer.entries[i].valid == FALSE)
        {
            serializer.entries[i].valid = TRUE;
            serializer.entries[i].address = address;
            for (int j = 0; j < cache_line_size; j++)
            {
                serializer.entries[i].data[j] = data[j];
            }
            serializer.entries[i].old_bits = serializer.occupancy;
            break;
        }
        else
        {
            continue;
        }
    }

    serializer.occupancy++;

    if (serializer.occupancy < max_serializer_occupancy)
    {
        metadata_bus.serializer_available = 1;
    }
    else
    {
        metadata_bus.serializer_available = 0;
    }
}

// called when the serializer is sending data
void deserializer_inserter()
{
    if (metadata_bus.burst_counter == 0)
    {
        deserializer.entries[metadata_bus.deserializer_target].address = metadata_bus.serializer_address;
        deserializer.entries[metadata_bus.deserializer_target].receiving_data_from_data_bus = TRUE;
    }
    deserializer.entries[metadata_bus.deserializer_target].data[metadata_bus.burst_counter * 4 + 0] = data_bus.byte_wires[0];
    deserializer.entries[metadata_bus.deserializer_target].data[metadata_bus.burst_counter * 4 + 1] = data_bus.byte_wires[1];
    deserializer.entries[metadata_bus.deserializer_target].data[metadata_bus.burst_counter * 4 + 2] = data_bus.byte_wires[2];
    deserializer.entries[metadata_bus.deserializer_target].data[metadata_bus.burst_counter * 4 + 3] = data_bus.byte_wires[3];
    metadata_bus.next_burst_counter = metadata_bus.burst_counter++;
    if (metadata_bus.burst_counter == 3)
    {
        deserializer.entries[metadata_bus.deserializer_target].old_bits = deserializer.occupancy;
        deserializer.occupancy++;
        deserializer.entries[metadata_bus.deserializer_target].valid = 1;
        deserializer.entries[metadata_bus.deserializer_target].receiving_data_from_data_bus = FALSE;
    }
}

// needs to be called every cycle, will set availability of deserializer on metadata bus
void deserializer_handler()
{
    // iterate through deserializers and find the first available one, use that as the target
    // also indicate which banks the deserializer wants to store into
    // also indicate which deserializer entry will store next
    bool found_deserializer_target = false;
    int temp_deserializer_bank_targets[banks_in_DRAM];
    for (int i = 0; i < banks_in_DRAM; i++)
    {
        temp_deserializer_bank_targets[i] = 0;
    }
    for (int i = 0; i < max_deserializer_occupancy; i++)
    {
        if (deserializer.entries[i].valid == FALSE && deserializer.entries[i].revival_lock == FALSE)
        {
            // target selection
            metadata_bus.deserializer_target = i;
            metadata_bus.deserializer_full = 0;
            found_deserializer_target = true;
            // which banks are targetted for stores
            temp_deserializer_bank_targets[get_bank_bits(deserializer.entries[i].address)] = 1;
        }
    }
    int search_age = 0;
    for (int i = 0; i < max_deserializer_occupancy; i++)
    {
        if (deserializer.entries[i].valid == TRUE)
        {
            // next deserializer entry that gets to store
            if (deserializer.entries[i].old_bits == search_age)
            {
                if (metadata_bus.bank_status[get_bank_bits(deserializer.entries[i].address)] == 0)
                {
                    metadata_bus.deserializer_next_entry = i;
                    metadata_bus.deserializer_can_store = 1;
                    break;
                }
                else if (search_age == num_deserializer_entries)
                {
                    metadata_bus.deserializer_can_store = 0;
                    break;
                }
                else
                {
                    search_age++;
                    i = -1;
                }
            }
        }
    }
    for (int i = 0; i < banks_in_DRAM; i++)
    {
        metadata_bus.deserializer_bank_targets[i] = temp_deserializer_bank_targets[i];
    }
    if (!found_deserializer_target)
    {
        metadata_bus.deserializer_full = 1;
    }
}

void rescue_stager()
{
    // if the destination is available for a write, stage the rescue
    // for this, a burst isn't needed
    // assume only one rescue allowed per cycle for now?
    bool success = false;
    for (int i = 0; i < num_serializer_entries; i++)
    {
        if (serializer.entries[i].rescue_lock == TRUE)
        {
            if (serializer.entries[i].rescue_destination == 0)
            {
                if (icache.bank_status[get_bank_bits(serializer.entries[i].address)] == 0)
                {
                    icache.bank_status[get_bank_bits(serializer.entries[i].address)] = 1;
                    success = icache_paste(serializer.entries[i].address, serializer.entries[i].data); // for now we'll assume that this is fast
                    if (success)
                        serializer.entries[i].rescue_lock == FALSE;
                    return;
                }
            }
            else if (serializer.entries[i].rescue_destination == 1)
            {
                if (dcache.bank_status[get_bank_bits(serializer.entries[i].address)] == 0)
                {
                    dcache.bank_status[get_bank_bits(serializer.entries[i].address)] = 1;
                    success = dcache_paste(serializer.entries[i].address, serializer.entries[i].data); // for now we'll assume that this is fast
                    if (success)
                        serializer.entries[i].rescue_lock == FALSE;
                    return;
                }
            }
        }
    }
}