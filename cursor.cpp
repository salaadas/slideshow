#include "cursor.h"

#include "draw.h"
#include "opengl.h"
#include "time_info.h"
#include "main.h"

Cursor_Type cursor_type = Cursor_Type::DOT;

constexpr i32 INTEGER_CURSOR_SIZE_MAX = 5;
i32 integer_cursor_size = 3;

bool cursor_visible = false;

Vector2 cursor_back_buffer_position; // In pixel units.
f32     cursor_back_buffer_size; // In pixel units.
f32     cursor_unit_scale_size = 0; // 1.0 means the radius is equal to the height of the back buffer

Vector2 point_of_interest_unit_scale(.5f, .5f);

void set_point_of_interest()
{
    if (!the_back_buffer->height) return;
    auto ih = 1 / (f32)the_back_buffer->height;

    if (!the_back_buffer->width) return;
    auto iw = 1 / (f32)the_back_buffer->width;

    auto [x, y] = get_mouse_pointer_position(glfw_window, true);
    auto pos = Vector2(x * iw, y * ih);

    point_of_interest_unit_scale = pos;
}

f32 move_toward(f32 a, f32 b, f32 amount)
{
    if (a > b)
    {
        a -= amount;
        if (a < b) a = b;
    }
    else
    {
        a += amount;
        if (a > b) a = b;
    }

    return a;
}

Vector3 move_torward(Vector3 a, Vector3 b, f32 amount)
{
    Vector3 result;
    result.x = move_toward(a.x, b.x, amount);
    result.y = move_toward(a.y, b.y, amount);
    result.z = move_toward(a.z, b.z, amount);

    return result;
}

void update_cursor()
{
    Vector2 pos;

    auto [mouse_x, mouse_y] = get_mouse_pointer_position(glfw_window, true);

    pos.x = (f32)mouse_x;
    pos.y = (f32)mouse_y;
    cursor_back_buffer_position = pos;

    auto desired_size = 0.007 * (f32)integer_cursor_size;
    if (!cursor_visible) desired_size = 0;

    auto rate = .13f;
    if (desired_size < cursor_unit_scale_size)
    {
        rate *= 1.3f;
        if (desired_size == 0) rate *= 2;
    }

    auto dt   = timez.real_world_dt;
    cursor_unit_scale_size  = move_toward(cursor_unit_scale_size, desired_size, dt * rate);
    cursor_back_buffer_size = the_back_buffer->height * cursor_unit_scale_size;
}

Vector2 get_vec2(f32 theta)
{
    auto ct = cos(theta);
    auto st = sin(theta);
    return Vector2(ct, st);
}

void draw_cursor_dot()
{
    rendering_2d_right_handed();
    set_shader(shader_argb_no_texture);

    constexpr auto NUM_TRIANGLES = 100;

    auto dtheta = (f32)TAU / NUM_TRIANGLES;

    auto color  = argb_color(Vector4(1, 1, 1, 1));
    auto center = cursor_back_buffer_position;
    auto r      = cursor_back_buffer_size;
    
    immediate_begin();
    for (i32 i = 0; i < NUM_TRIANGLES; ++i)
    {
        auto theta0 = i       * dtheta;
        auto theta1 = (i + 1) * dtheta;

        auto v0 = get_vec2(theta0);
        auto v1 = get_vec2(theta1);

        auto p0 = center;
        auto p1 = center + r * v0;
        auto p2 = center + r * v1;

        immediate_triangle(p0, p1, p2, color);
    }
    immediate_flush();
}

void draw_cursor_arrow()
{
    rendering_2d_right_handed();
    set_shader(shader_argb_and_texture);

    auto map = catalog_find(&texture_catalog, String("red-arrow"));
    set_texture(String("diffuse_texture"), map);

    auto color  = argb_color(Vector4(1, 0, 0, 1));
    auto r      = cursor_back_buffer_size;

    auto target = cursor_back_buffer_position;
    auto poi = point_of_interest_unit_scale * Vector2(the_back_buffer->width, the_back_buffer->height);
    auto delta_pos = poi - target;
    auto len = length(delta_pos);

    auto theta = 0.0f;

    if (len)
    {
        theta = atan2(delta_pos.y, delta_pos.x);
    }

    auto va = get_vec2(theta) * r;
    auto vb = Vector2(-va.y, va.x);

    auto center = target - va;

    auto p0 = center - va - vb;
    auto p1 = center + va - vb;
    auto p2 = center + va + vb;
    auto p3 = center - va + vb;

    auto u0 = Vector2(0, 0);
    auto u1 = Vector2(1, 0);
    auto u2 = Vector2(1, 1);
    auto u3 = Vector2(0, 1);

    immediate_begin();
    immediate_quad(p0, p1, p2, p3, u0, u1, u2, u3, color);
    immediate_flush();
}

void draw_cursor()
{
    if (cursor_type == Cursor_Type::DOT)
    {
        draw_cursor_dot();
    }
    if (cursor_type == Cursor_Type::ARROW)
    {
        draw_cursor_arrow();
    }
}
