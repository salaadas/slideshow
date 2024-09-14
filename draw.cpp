#include "draw.h"

#include "main.h"
#include "opengl.h"
#include "time_info.h"
#include "cursor.h"
#include "file_utils.h"

Shader *shader_argb_no_texture;
Shader *shader_argb_and_texture;
Shader *shader_text;

#define cat_find(catalog, name) \
    catalog_find(&catalog, String(name));

void init_shaders()
{
    shader_argb_no_texture  = cat_find(shader_catalog, "argb_no_texture"); assert(shader_argb_no_texture);
    shader_argb_and_texture = cat_find(shader_catalog, "argb_and_texture"); assert(shader_argb_and_texture);
    shader_text             = cat_find(shader_catalog, "text"); assert(shader_text);

    shader_argb_no_texture->backface_cull  = false;
    shader_argb_and_texture->backface_cull = false;
}

#undef cat_find

void rendering_2d_right_handed()
{
    f32 w = render_target_width;
    f32 h = render_target_height;
    if (h < 1) h = 1;

    // This is a GL-style projection matrix mapping to [-1, 1] for x and y
    Matrix4 tm = Matrix4(1.0);
    tm[0][0] = 2 / w;
    tm[1][1] = 2 / h;
    tm[0][3] = -1;
    tm[1][3] = -1;

    view_to_proj_matrix    = tm;
    world_to_view_matrix   = Matrix4(1.0);
    object_to_world_matrix = Matrix4(1.0);

    refresh_transform();
}

void rendering_2d_right_handed_unit_scale()
{
    // @Note: cutnpaste from rendering_2d_right_handed
    f32 h = render_target_height / (f32)render_target_width;

    // This is a GL-style projection matrix mapping to [-1, 1] for x and y
    auto tm = Matrix4(1.0);
    tm[0][0] = 2;
    tm[1][1] = 2 / h;
    tm[0][3] = -1;
    tm[1][3] = -1;

    view_to_proj_matrix    = tm;
    world_to_view_matrix   = Matrix4(1.0);
    object_to_world_matrix = Matrix4(1.0);

    refresh_transform();
}

void draw_gradient()
{
    rendering_2d_right_handed();

    f32 w = render_target_width;
    f32 h = render_target_height;

    constexpr i32 PADDING = 100;

    auto p0 = Vector2(0 + PADDING, 0 + PADDING);
    auto p1 = Vector2(w - PADDING, 0 + PADDING);
    auto p2 = Vector2(w - PADDING, h - PADDING);
    auto p3 = Vector2(0 + PADDING, h - PADDING);

    f32 r0 = 61.0f / 255.0f;
    f32 g0 = 29.0f / 255.0f;
    f32 b0 = 29.0f / 255.0f;

    f32 r1 = 35.0f / 255.0f;
    f32 g1 = 19.0f / 255.0f;
    f32 b1 = 19.0f / 255.0f;

    f32 k0 = 0.7f;
    f32 k1 = 0.3f;

    r0 *= k0;
    g0 *= k0;
    b0 *= k0;

    r1 *= k1;
    g1 *= k1;
    b1 *= k1;

    auto z = 0.0f;

    auto c0 = argb_color(Vector3(r0, g0, b0));
    auto c1 = argb_color(Vector3(r1, g1, b1));

    auto background_color = argb_color(Vector3(1, 1, .12));

    set_shader(shader_argb_no_texture);
    immediate_begin();
    immediate_quad(p0, p1, p2, p3, background_color);

    {
        auto my_r = (f32)((i32)(timez.ui_time * 14) & 255) / 255.0;
        auto my_g = (f32)((i32)(timez.ui_time * 10) & 255) / 255.0;

        immediate_triangle(p0, p1, p3, argb_color(Vector3(my_r, my_g, .5)));
    }

    immediate_flush();
}

void draw_text_with_backing(Dynamic_Font *font, i64 x, i64 y, String text, Vector4 color);

void draw_centered(Dynamic_Font *font, f32 y_unit_scale, String text, Vector4 color)
{
    auto width = prepare_text(font, text);

    i64 x = (i64)((render_target_width - width) / 2.0f);
    i64 y = (i64)(y_unit_scale * render_target_height);

    draw_text_with_backing(font, x, y, text, color);
}

void draw_text_with_backing(Dynamic_Font *font, i64 x, i64 y, String text, Vector4 color)
{
    auto ox = (i64)(font->character_height * 0.03f);
    auto oy = -ox;

    auto bg_color = Vector4(0, 0, 0, 0.6f * color.w);

    draw_prepared_text(font, x + ox, y + oy, bg_color);
    draw_prepared_text(font,      x,      y,    color);
}

f32 get_left_margin(Slide *slide)
{
    if (slide->left_margin >= 0) return slide->left_margin;

    return 0.1f;
}

f32 get_right_margin(Slide *slide)
{
    if (slide->right_margin >= 0) return slide->right_margin;

    return 0.1f;
}

void draw_text_item(Slideshow *show, Text_Item *text_item, Slide *slide)
{
    Vector4 text_color = text_item->color;

    auto font = get_font(show, slide, text_item);

    f32  sx;
    f32  sy   = render_target_height * text_item->y_coordinate;
    auto text = text_item->text;

    auto justification = text_item->justification;

    auto width = prepare_text(font, text);

    switch (justification)
    {
        case Justification::LEFT:
        {
            sx = get_left_margin(slide) * render_target_width;
        } break;
        case Justification::CENTER:
        {
            auto left   = get_left_margin(slide);
            auto right  = get_right_margin(slide);
            auto center = (left + (1 - right)) / 2;
            sx = center*render_target_width - width/2.0;
        } break;
        case Justification::RIGHT:
        {
            sx = (1 - get_right_margin(slide)) * render_target_width - width;
        } break;
    }

    draw_text_with_backing(font, (i64)sx, (i64)sy, text, text_item->color);
}

void draw_single_slide(Slideshow *show, Slide *slide);

void draw_used_slide(Slideshow *show, Slide_Used_Slide *used_slide)
{
    auto other_slide = used_slide->other_slide;

    draw_single_slide(show, other_slide);
}

void draw_image_item(Slideshow *show, Image_Item *image, Slide *slide)
{
    rendering_2d_right_handed();

    // @Speed: We re-look-up the asset name every call to draw_image_item, and then do the catalog
    // lookup. This is bad. Instead, we could just store the pointer in the slideshow, but that can
    // make it not stable to changing the name of the short_name??
    auto [full_name, found] = table_find(&show->texture_name_table, image->name);

    if (found)
    {
        auto map = catalog_find(&texture_catalog, full_name);
        assert(map);

        Vector2 center = Vector2(image->position.x * render_target_width  * 0.01f,
                                 image->position.y * render_target_height * 0.01f);
        f32  scale = image->scale * 0.01f;

        auto denom = (f32)(map->height);
        if (denom == 0) denom = 1;
        auto h = scale * render_target_height * .5;
        auto w = scale * render_target_height * .5 * (map->width / denom);

        u32 icolor = argb_color(image->color);

        auto p0 = center + Vector2(-w, -h);
        auto p1 = center + Vector2(+w, -h);
        auto p2 = center + Vector2(+w, +h);
        auto p3 = center + Vector2(-w, +h);

        auto uu = image->uv_scale.x;
        auto vv = image->uv_scale.y;
        auto u0 = Vector2( 0,  0);
        auto u1 = Vector2(uu,  0);
        auto u2 = Vector2(uu, vv);
        auto u3 = Vector2( 0, vv);

        // Do the crop first
        if (image->cropping_set)
        {
            auto op0 = p0; auto op1 = p1; auto op2 = p2; auto op3 = p3;
            auto ou0 = u0; auto ou1 = u1; auto ou2 = u2; auto ou3 = u3;

            auto crop_scale = 0.01f;

            p0 = lerp(op0, op1, image->crop_left  * crop_scale);
            p3 = lerp(op3, op2, image->crop_left  * crop_scale);
            p1 = lerp(op1, op0, image->crop_right * crop_scale);
            p2 = lerp(op2, op3, image->crop_right * crop_scale);

            u0 = lerp(ou0, ou1, image->crop_left  * crop_scale);
            u3 = lerp(ou3, ou2, image->crop_left  * crop_scale);
            u1 = lerp(ou1, ou0, image->crop_right * crop_scale);
            u2 = lerp(ou2, ou3, image->crop_right * crop_scale);

            // @Cleanup: Now we do the same thing, but vertically :(
            op0 = p0; op1 = p1; op2 = p2; op3 = p3;
            ou0 = u0; ou1 = u1; ou2 = u2; ou3 = u3;

            p0 = lerp(op0, op3, image->crop_bottom * crop_scale);
            p1 = lerp(op1, op2, image->crop_bottom * crop_scale);
            p2 = lerp(op2, op1, image->crop_top    * crop_scale);
            p3 = lerp(op3, op0, image->crop_top    * crop_scale);

            u0 = lerp(ou0, ou3, image->crop_bottom * crop_scale);
            u1 = lerp(ou1, ou2, image->crop_bottom * crop_scale);
            u2 = lerp(ou2, ou1, image->crop_top    * crop_scale);
            u3 = lerp(ou3, ou0, image->crop_top    * crop_scale);
        }

        // Then comes the rotation
        // @Note: Cropping changes the center of rotation!!
        if (image->rotation)
        {
            auto theta = image->rotation * (TAU / 360.0f);

            auto center = (p0 + p2) * .5f;

            p0 -= center;
            p1 -= center;
            p2 -= center;
            p3 -= center;

            p0 = rotate(p0, theta) + center;
            p1 = rotate(p1, theta) + center;
            p2 = rotate(p2, theta) + center;
            p3 = rotate(p3, theta) + center;
        }

        if (image->border_width)
        {
            auto border_color = argb_color(image->border_color);
            auto border_scale = 0.01f;

            auto bx = image->border_width * border_scale * render_target_height * unit_vector(p2 - p3);
            auto by = image->border_width * border_scale * render_target_height * unit_vector(p2 - p1);

            auto np0 = p0 - bx - by;
            auto np1 = p1 + bx - by;
            auto np2 = p2 + bx + by;
            auto np3 = p3 - bx + by;

            immediate_begin();            
            set_shader(shader_argb_no_texture);

            immediate_quad(np0, np1,  p1,  p0, border_color);
            immediate_quad( p1, np1, np2,  p2, border_color);
            immediate_quad( p3,  p2, np2, np3, border_color);
            immediate_quad(np0,  p0,  p3, np3, border_color);

            immediate_flush();
        }

        immediate_begin();
        set_shader(shader_argb_and_texture);
        set_texture(String("diffuse_texture"), map);
        immediate_quad(p0, p1, p2, p3, u0, u1, u2, u3, icolor);
        immediate_flush();
    }
    else
    {
        logprint("draw_image_item", "??? Could not find the fullname for image '%s'. This should have been caught during loading the slideshow\n", temp_c_string(image->name));
    }
}

void draw_single_slide(Slideshow *show, Slide *slide)
{
    if (!slide) return;

    if (slide->items.count)
    {
        for (auto item : slide->items)
        {
            if (cmp_var_type_to_type(item->type, Text_Item))
            {
                draw_text_item(show, CastSlideItem<Text_Item>(item), slide);
            }
            if (cmp_var_type_to_type(item->type, Image_Item))
            {
                draw_image_item(show, CastSlideItem<Image_Item>(item), slide);
            }
            if (cmp_var_type_to_type(item->type, Slide_Used_Slide))
            {
                draw_used_slide(show, CastSlideItem<Slide_Used_Slide>(item));
            }
        }
    }
}

void draw_slideshow()
{
    // Render the scene to a offscreen buffer
    set_render_target(0, the_offscreen_buffer);

    // @Note: Below here we draw the slideshow to the offscreen framebuffer
    auto slide = get_current_slide();

    if (slide)
    {
        auto c = slide->background_color;
        glClearColor(c.x, c.y, c.z, c.w);
    }
    else
    {
        // Should not happen!!!
        glClearColor(.1f, .69f, .1f, 1.0f);
    }
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    rendering_2d_right_handed();
    draw_single_slide(the_slideshow, slide);

    // @Note: Render that offscreen buffer as a quad onto the backbuffer
    set_render_target(0, the_back_buffer);

    rendering_2d_right_handed();
    set_shader(shader_argb_and_texture);

    // glClearColor(1, 0, 0, 1.0f);
    // glClearColor(0, 0, 0, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    f32 back_buffer_width  = the_back_buffer->width;
    f32 back_buffer_height = the_back_buffer->height;

    f32 w  = (f32)(the_offscreen_buffer->width);
    f32 h  = (f32)(the_offscreen_buffer->height);
    f32 bx = floorf(0.5 * (back_buffer_width - the_offscreen_buffer->width));
    f32 by = floorf(0.5 * (back_buffer_height - the_offscreen_buffer->height));

    auto p0 = Vector2(bx,     by);
    auto p1 = Vector2(bx + w, by);
    auto p2 = Vector2(bx + w, by + h);
    auto p3 = Vector2(bx,     by + h);

    auto u0 = Vector2(0, 0);
    auto u1 = Vector2(1, 0);
    auto u2 = Vector2(1, 1);
    auto u3 = Vector2(0, 1);

    set_texture(String("diffuse_texture"), the_offscreen_buffer);
    
    immediate_begin();
    immediate_quad(p0, p1, p2, p3, u0, u1, u2, u3, 0xffffffff);
    immediate_flush();

    draw_cursor();
}

void draw_generated_quads(Dynamic_Font *font, Vector4 color)
{
    rendering_2d_right_handed();
    set_shader(shader_text);

    // glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);
    // glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, 1.0);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    GLuint last_texture_id = 0xffffffff;

    immediate_begin();

    for (auto quad : font->current_quads)
    {
        auto page = quad.glyph->page;
        auto map  = &page->texture;

        if (page->dirty)
        {
            // printf("! Should be generating texture again\n");
            page->dirty = false;
            auto bitmap = page->bitmap_data;

            // @Fixme: Inspect this
            // if (map->width != bitmap->width || map->height != bitmap->height)
            {

                // @Fixme: Inspect this
                // if (map->id == 0xffffffff || !map->id)
                {
                    // printf("Generating a texture for font page\n");
                    glGenTextures(1, &map->id);
                    glBindTexture(GL_TEXTURE_2D, map->id);
                }

                map->width  = bitmap->width;
                map->height = bitmap->height;
                map->data   = bitmap;
                map->dirty  = true;
            }
        }

        if (map->id != last_texture_id)
        {
            // @Speed
            // This will cause a flush for every call to draw_text.
            // But if we don't do this then we won't set the texture.
            // Need to refactor the text rendering code so that we don't have to deal with this
            immediate_flush();
            last_texture_id = map->id;
            set_texture(String("diffuse_texture"), map);
        }

        immediate_letter_quad(quad, color);
        // immediate_flush();
    }

    immediate_flush();
}

void draw_prepared_text(Dynamic_Font *font, i64 x, i64 y, Vector4 color)
{
    generate_quads_for_prepared_text(font, x, y);
    draw_generated_quads(font, color);
}

i64 draw_text(Dynamic_Font *font, i64 x, i64 y, String text, Vector4 color)
{
    auto width = prepare_text(font, text);
    draw_prepared_text(font, x, y, color);

    return width;
}
