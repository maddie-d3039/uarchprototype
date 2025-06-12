#include "config.h"

class RAT
{
    enum Registers
    {
        EAX_idx,
        EBX_idx,
        ECX_idx,
        EDX_idx,
        ESI_idx,
        EDI_idx,
        EBP_idx,
        ESP_idx,
        GPR_Count
    } Registers;

    struct RegisterAliasPool
    {
        int tags[REGISTER_ALIAS_POOL_ENTRIES];
        bool valid[REGISTER_ALIAS_POOL_ENTRIES];

        RegisterAliasPool()
        {
            // Initialize each slot so aliases[i] = (base + i), and mark all as unused
            for (int i = 0; i < REGISTER_ALIAS_POOL_ENTRIES; ++i)
            {
                tags[i] = REGISTER_ALIAS_BASE + i;
                valid[i] = false;
            }
        }
    };

    // Returns one free alias in [0..63], or -1 if none left
    int getAlias()
    {
        for (int i = 0; i < REGISTER_ALIAS_POOL_ENTRIES; ++i)
        {
            if (!valid[i])
            {
                valid[i] = true;
                return pool_aliases->tags[i];
            }
        }
        return -1;
    }

    // Frees a previously allocated alias.
    // If `alias` is out of [0..63] or wasn't actually allocated, an assert fires.
    void deallocateAlias(int alias)
    {
        // Compute local index
        int index = alias - REGISTER_ALIAS_BASE;

        // 1) Assert that alias is within this pool's global range
        assert(index >= 0 && index < REGISTER_ALIAS_POOL_ENTRIES && "RegisterAliasPool::free(): alias out of range!");

        // 2) Assert that this slot was previously allocated (valid == true)
        assert(valid[index] && "RegisterAliasPool::free(): alias was not allocated!");

        // Finally, mark it free again
        pool_aliases->valid[index] = false;
    }

    RegisterAliasPool *pool_aliases = new RegisterAliasPool();
    int valid[GPR_Count] = {}; // this is the RAT, not the pool
    int tag[GPR_Count] = {};   // this is the RAT, not the pool
    int val[GPR_Count] = {};   // this is the RAT, not the pool

    int RegisterFile[GPR_Count];

    void update(int tag, int value, enum Registers Reg)
    {
        if (tag < REGISTER_ALIAS_POOL_ENTRIES && tag > REGISTER_ALIAS_BASE)
        {
            deallocateAlias(tag);
            RegisterFile[Reg] = value;
        }
    }
};