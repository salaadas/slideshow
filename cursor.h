#pragma once

#include "common.h"

enum class Cursor_Type
{
    DOT = 0,
    ARROW,
};

extern Cursor_Type cursor_type;
extern i32  integer_cursor_size;
extern bool cursor_visible;


void update_cursor();
void draw_cursor();
void set_point_of_interest();
