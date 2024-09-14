#include "events.h"

// Per-frame mouse deltas:
i32         mouse_delta_x;
i32         mouse_delta_y;
Wheel_Delta mouse_wheel_delta;

RArr<Event>                               events_this_frame;
// SArr<Key_Current_State, CODE_TOTAL_COUNT> input_button_states;

#include "table.h"
Table<u32, bool> key_down_state_table;

void input_per_frame_event_and_flag_update()
{
    // Called once per frame, probably

    array_reset(&events_this_frame);

    // auto mask     = ~KSTATE_START;
    // auto end_mask = ~(KSTATE_END | KSTATE_DOWN | KSTATE_START);

    // for (i64 i = 0; i < input_button_states.count; ++i)
    // {
    //     auto it = &input_button_states.data[i];

    //     if (*it & KSTATE_END)
    //     {
    //         *it = (Key_Current_State)(*it & end_mask);
    //     }
    //     else
    //     {
    //         *it = (Key_Current_State)(*it & mask);
    //     }
    // }

    mouse_delta_x = 0;
    mouse_delta_y = 0;
    mouse_wheel_delta = (Wheel_Delta){};
}

void update_linux_events()
{
    input_per_frame_event_and_flag_update();
}

#define check_kc_and_ret(glfw_key, code_name)                   \
    if (ikey == GLFW_KEY_##glfw_key) {return CODE_##code_name;}

// https://www.glfw.org/docs/3.3/group__keys.html
Key_Code get_key_code(i32 ikey)
{
    check_kc_and_ret(BACKSPACE, BACKSPACE);
    check_kc_and_ret(TAB,       TAB);
    check_kc_and_ret(ENTER,     ENTER);
    check_kc_and_ret(ESCAPE,    ESCAPE);
    check_kc_and_ret(SPACE,     SPACEBAR);

    check_kc_and_ret(0, 0);
    check_kc_and_ret(1, 1);
    check_kc_and_ret(2, 2);
    check_kc_and_ret(3, 3);
    check_kc_and_ret(4, 4);
    check_kc_and_ret(5, 5);
    check_kc_and_ret(6, 6);
    check_kc_and_ret(7, 7);
    check_kc_and_ret(8, 8);
    check_kc_and_ret(9, 9);

    check_kc_and_ret(A, A);
    check_kc_and_ret(B, B);
    check_kc_and_ret(C, C);
    check_kc_and_ret(D, D);
    check_kc_and_ret(E, E);
    check_kc_and_ret(F, F);
    check_kc_and_ret(G, G);
    check_kc_and_ret(H, H);
    check_kc_and_ret(I, I);
    check_kc_and_ret(J, J);
    check_kc_and_ret(K, K);
    check_kc_and_ret(L, L);
    check_kc_and_ret(M, M);
    check_kc_and_ret(N, N);
    check_kc_and_ret(O, O);
    check_kc_and_ret(P, P);
    check_kc_and_ret(Q, Q);
    check_kc_and_ret(R, R);
    check_kc_and_ret(S, S);
    check_kc_and_ret(T, T);
    check_kc_and_ret(U, U);
    check_kc_and_ret(V, V);
    check_kc_and_ret(W, W);
    check_kc_and_ret(X, X);
    check_kc_and_ret(Y, Y);
    check_kc_and_ret(Z, Z);

    check_kc_and_ret(DELETE, DELETE);

    check_kc_and_ret(UP,    ARROW_UP);
    check_kc_and_ret(DOWN,  ARROW_DOWN);
    check_kc_and_ret(LEFT,  ARROW_LEFT);
    check_kc_and_ret(RIGHT, ARROW_RIGHT);

    check_kc_and_ret(PAGE_UP,   PAGE_UP);
    check_kc_and_ret(PAGE_DOWN, PAGE_DOWN);

    check_kc_and_ret(HOME, HOME);
    check_kc_and_ret(END,  END);

    check_kc_and_ret(INSERT, INSERT);

    check_kc_and_ret(PAUSE, PAUSE);
    check_kc_and_ret(SCROLL_LOCK, SCROLL_LOCK);

    check_kc_and_ret(LEFT_ALT,  ALT);
    check_kc_and_ret(RIGHT_ALT, ALT);

    check_kc_and_ret(LEFT_CONTROL,  CTRL);
    check_kc_and_ret(RIGHT_CONTROL, CTRL);

    check_kc_and_ret(LEFT_SHIFT,  SHIFT);
    check_kc_and_ret(RIGHT_SHIFT, SHIFT);

    check_kc_and_ret(F1,  F1);
    check_kc_and_ret(F2,  F2);
    check_kc_and_ret(F3,  F3);
    check_kc_and_ret(F4,  F4);
    check_kc_and_ret(F5,  F5);
    check_kc_and_ret(F6,  F6);
    check_kc_and_ret(F7,  F7);
    check_kc_and_ret(F8,  F8);
    check_kc_and_ret(F9,  F9);
    check_kc_and_ret(F10, F10);
    check_kc_and_ret(F11, F11);
    check_kc_and_ret(F12, F12);

    // assert(0);
    return CODE_UNKNOWN;
}

#undef check_kc_and_ret

// Keyboard handling callback for GLFW
void glfw_keyboard_callback(GLFWwindow *window, i32 ikey, i32 scancode, i32 action, i32 mods);
// Mouse position callback for GLFW
void glfw_mouse_position_callback(GLFWwindow *window, f64 x, f64 y);
// Mouse wheel callback for GLFW
void glfw_mouse_wheel_callback(GLFWwindow *window, f64 x_offset, f64 y_offset);

void glfw_mouse_button_callback(GLFWwindow *window, i32 button, i32 action, i32 mods);

void hook_our_input_event_system_to_glfw(GLFWwindow *glfw_window)
{
    // Set keyboard callback
    // return the value of the previous callback function (used to chain callbacks)
    glfwSetKeyCallback(glfw_window, glfw_keyboard_callback);

    // Set mouse position callback
    // return the value of the previous callback function (used to chain callbacks)
    glfwSetCursorPosCallback(glfw_window, glfw_mouse_position_callback);

    // Set mouse wheel callback
    // return the value of the previous callback function (used to chain callbacks)
    glfwSetScrollCallback(glfw_window, glfw_mouse_wheel_callback);

    glfwSetMouseButtonCallback(glfw_window, glfw_mouse_button_callback);
}

// @Fixme: Modifiers keys are not handled correctly right now!!!
void glfw_keyboard_callback(GLFWwindow *window, i32 ikey, i32 scancode, i32 action, i32 mods)
{
    // key is the keyboard key that was PRESSED or RELEASED
    // scancode is the platform-dependent scan code for the key
    // action is one of GLFW_PRESS, GLFW_RELEASE, or GLFW_REPEAT
    // mods contains flags describing which modifier keys (such as Shift/Ctrl) were pressed at the same time

    u32 ukey = (u32)ikey;

    auto is_down = (action == GLFW_PRESS) || (action == GLFW_REPEAT);
    auto was_down = table_find_pointer(&key_down_state_table, ukey) != NULL;

    if (is_down && !was_down)
    {
        // printf("Key %c was pressed!\n", (char)ukey);
        table_add(&key_down_state_table, ukey, true);
        // printf("[add key] table size: %ld\n", key_down_state_table.count);
    }
    else if (was_down && !is_down)
    {
        // printf("Removing key %c from table because it is not pressed\n", (char)ukey);
        table_remove(&key_down_state_table, ukey);
        // printf("[remove key] table size: %ld\n", key_down_state_table.count);
    }

    auto repeat = action == GLFW_REPEAT;

    // printf("isdown(%d)\twasdown(%d)\trepeat(%d)\n", is_down, was_down, repeat);
    // When release key, isdown == 0, wasdown == 1, repeat == 0

    if (!is_down && !was_down)
    {
        // Redundant key_up event
        return;
    }

    if (is_down && repeat && !was_down)
    {
        // @Check: is is relevant?
        // key was pressed while we didn't have focus so the first
        // event we see is incorrectly labeled as a repeat.
        repeat = false;
    }

    // printf("Setting key event\n");

    bool shift_state, ctrl_state, alt_state;
    shift_state = false;
    ctrl_state  = false;
    alt_state   = false;

    auto key_code = get_key_code(ikey);

    if (key_code == CODE_UNKNOWN)
    {
        printf("Unknown keycode %d, returning...\n", ikey);
        return;
    }

    if (key_code == CODE_ALT)   alt_state   = true;
    if (key_code == CODE_CTRL)  ctrl_state  = true;
    if (key_code == CODE_SHIFT) shift_state = true;

    Event event;
    event.type          = EVENT_KEYBOARD;
    event.key_code      = key_code;
    event.key_pressed   = (u32)is_down;
    event.shift_pressed = shift_state;
    event.ctrl_pressed  = ctrl_state;
    event.alt_pressed   = alt_state;
    event.repeat        = repeat;

    array_add(&events_this_frame, event);
    // printf("events count %ld\n", events_this_frame.count);
}

void glfw_mouse_position_callback(GLFWwindow *window, f64 x, f64 y)
{
    // @Important: the x and y parameters contain the new position of the mouse cursor
    // relative to the TOP LEFT of the window
    //
    // !!! Not the case of OpenGL because the origin there starts at the BOTTOM LEFT corner of the window !!!!
    // BE WARY OF THIS WHEN HANDLING THE MOUSE ^
}

void glfw_mouse_wheel_callback(GLFWwindow *window, f64 x_offset, f64 y_offset)
{
    // @Note: this can take offset in the x- and y- direction, so 2 dimensional scrolling is supported
}

// @Cutnpaste from glfw_keyboard_callback
void glfw_mouse_button_callback(GLFWwindow *window, i32 button, i32 action, i32 mods)
{
    auto ukey = (u32)button;

    auto is_down = (action == GLFW_PRESS) || (action == GLFW_REPEAT);
    auto was_down = table_find_pointer(&key_down_state_table, ukey) != NULL;

    if (is_down && !was_down)
    {
        table_add(&key_down_state_table, ukey, true);
    }
    else if (was_down && !is_down)
    {
        table_remove(&key_down_state_table, ukey);
    }

    auto repeat = action == GLFW_REPEAT;

    if (!is_down && !was_down)
    {
        // Redundant key_up event
        return;
    }

    if (is_down && repeat && !was_down)
    {
        // @Check: is is relevant?
        // key was pressed while we didn't have focus so the first
        // event we see is incorrectly labeled as a repeat.
        repeat = false;
    }

    Key_Code code;

    if (button == GLFW_MOUSE_BUTTON_RIGHT)
    {
        code = CODE_MOUSE_RIGHT;
    }
    else if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        code = CODE_MOUSE_LEFT;
    }
    else
    {
        printf("Unknown keycode %d, returning...\n", ukey);
        return;
    }

    Event event;
    event.type          = EVENT_KEYBOARD;
    event.key_code      = code;
    event.key_pressed   = (u32)is_down;
    event.repeat        = repeat;
    array_add(&events_this_frame, event);
}

// // @Hack: @Fixme @Temporary
// bool is_fullscreen = false;
// void toggle_fullscreen()
// {
//     GLFWmonitor *monitor = NULL;

//     if (is_fullscreen)
//     {
//         monitor = glfwGetPrimaryMonitor();
//     }

//     is_fullscreen = !is_fullscreen;

//     glfwSetWindowMonitor(glfw_window, monitor, 0, 0, 1920/2, 1080/2, GLFW_DONT_CARE);
// }
