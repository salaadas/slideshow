#include "hash.h"

u32 get_hash(String s)
{
    u32 hash = 5381;

    for (auto it : s) hash = ((hash << 5) + hash) + it;

    return hash;
}

u32 get_hash(char c)
{
    // @Fixme: replace this madeup hash function with something real
    u32 hash = 5381;
    return (hash << 5) + (c << 2) + c;
}

// For key_down_state_table
u32 get_hash(u32 x)
{
    return x;
}

u32 get_hash(i64 x)
{
    if (x < 0) return -x;
    return x;
}

bool equal(i64 a, i64 b)
{
    return a == b;
}


bool equal(u32 a, u32 b)
{
    return a == b;
}
