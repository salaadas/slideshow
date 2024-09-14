#pragma once

#include "common.h"

struct Slide_Item
{
    _type_Type type = _make_Type(Slide_Item); // @Ughh: @Fixme: Get rid of this.
};

enum class Justification
{
    LEFT,
    CENTER,
    RIGHT
};

struct Text_Item
{
    Slide_Item    base;
    String        text;
    String        font_name;
    Vector4       color;
    f32           y_coordinate;
    f32           size; // In terms of render_target_height percent (size=10 means 10 lines would fit on the screen)
    Justification justification;

    // @Hack: Could be factored elsewhere
    f32           line_spacing_factor = 1.0f;
};

struct Image_Item
{
    Slide_Item base;
    String     name;
    Vector2    position;
    f32        scale;
    f32        rotation;
    Vector4    color;

    bool       cropping_set; // Whether 'draw' should consider cropping as it messes with the xy coords
    f32        crop_left;
    f32        crop_right;
    f32        crop_top;
    f32        crop_bottom;

    Vector4    border_color;
    f32        border_width;

    Vector2    uv_scale;
};

struct Slide;

struct Slide_Used_Slide
{
    Slide_Item base;
    Slide     *other_slide;
};

struct Slide
{
    Vector4 background_color;

    f32 left_margin;
    f32 right_margin;

    RArr<Slide_Item*> items;
};

#include "table.h"
#include "font.h"

struct Slideshow
{
    f32 aspect_x;
    f32 aspect_y;

    RArr<Slide*> visible_slides; // @Rename: These are only the slides that will be shown;
    RArr<Slide*> all_slides; // This stores all slides, for the purposes of freeing them.

    Table<String, String>         name_to_font_name;
    Table<String, Text_Item*>     style_table;
    Table<String, Slide*>         slide_name_table;
    Table<String, String>         texture_name_table;
};



template <class Target_Type>
inline
Target_Type *CastSlideItem(Slide_Item *slide_item)
{
    // Check just in case I forgot to do cmp...
    bool b = cmp_var_type_to_type(slide_item->type, Target_Type);
    assert(b);
    return (Target_Type*)(slide_item);
}

Slide *get_current_slide();
Slideshow *load_slideshow(String full_path);
void unload_slideshow(Slideshow *slideshow);
Dynamic_Font *get_font(Slideshow *show, Slide *slide, Text_Item *text);

