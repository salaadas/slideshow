#pragma once

#include "common.h"

u32 get_hash(String s);
u32 get_hash(char s);
u32 get_hash(u32 x);
u32 get_hash(i64 x);

bool equal(i64 a, i64 b);
bool equal(u32 a, u32 b);
