#include "user_control.h"

#include "main.h"
#include "events.h"
#include "cursor.h"

bool should_ignore_input = false;

void slide_navigate(i32 delta)
{
    current_slide_index += delta;
}

void slide_set_absolute(i32 index)
{
    current_slide_index = index;
}

void handle_event(Event *event)
{
    if (event->type == EVENT_KEYBOARD)
    {
        auto key     = event->key_code;
        auto pressed = event->key_pressed;

        if (pressed)
        {
            switch (key)
            {
                case CODE_F1:          cursor_type = Cursor_Type::DOT;   cursor_visible = true; break;
                case CODE_F2:          cursor_type = Cursor_Type::ARROW; cursor_visible = true; break;

                case CODE_MOUSE_LEFT:  set_point_of_interest(); break;

                case CODE_ARROW_LEFT:  slide_navigate(-1); break;
                case CODE_ARROW_RIGHT: slide_navigate(+1); break;
                case CODE_ARROW_UP:    slide_navigate(-1); break;
                case CODE_ARROW_DOWN:  slide_navigate(+1); break;

                case CODE_ENTER:       toggle_fullscreen(); break;

                case CODE_HOME:        slide_set_absolute(0); break;
                case CODE_END:         if (the_slideshow) slide_set_absolute(the_slideshow->visible_slides.count - 1); break;
                case CODE_PAGE_UP:     slide_navigate(-1); break;
                case CODE_PAGE_DOWN:   slide_navigate(+1); break;

                case CODE_H:           slide_navigate(-1); break;
                case CODE_J:           slide_navigate(+1); break;
                case CODE_K:           slide_navigate(-1); break;
                case CODE_L:           slide_navigate(+1); break;

                case CODE_1:           integer_cursor_size = 1; cursor_visible = true; break;
                case CODE_2:           integer_cursor_size = 2; cursor_visible = true; break;
                case CODE_3:           integer_cursor_size = 3; cursor_visible = true; break;
                case CODE_4:           integer_cursor_size = 4; cursor_visible = true; break;
                case CODE_5:           integer_cursor_size = 5; cursor_visible = true; break;
                case CODE_SPACEBAR:    cursor_visible = !cursor_visible; break;
            }
        }
    }
}

void read_input()
{
    for (auto event : events_this_frame)
    {
        if (should_ignore_input) break;

        handle_event(&event);
    }
}
