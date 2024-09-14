// @Leak  @Leak  @Leak  @Leak @Important:
// @Leak  @Leak  @Leak  @Leak @Important:
// @Leak  @Leak  @Leak  @Leak @Important:

#include "slides.h"
#include "main.h"
#include "file_utils.h"
#include "opengl.h"

Slide *get_current_slide()
{
    auto total_slides = the_slideshow->visible_slides.count;

    if (!the_slideshow) return NULL;
    if (!total_slides)  return NULL;
        
    current_slide_index = std::clamp(current_slide_index, 0, (i32)(total_slides - 1));

    return the_slideshow->visible_slides[current_slide_index];
}

void reset_to_defaults(Text_Item *item)
{
    if (item->font_name)
    {
        free_string(&item->font_name);
    }

    if (item->text)
    {
        free_string(&item->text);
    }

    item->y_coordinate        = 0.5f;
    item->color               = Vector4(1, 1, 1, 1);
    item->size                = 10;
    item->justification       = Justification::CENTER;
    item->line_spacing_factor = 1.0f;
}

Slide_Used_Slide *make_slide_used_slide(Slide *other_slide)
{
    auto template_slide = New<Slide_Used_Slide>(false);

    _set_type_Type(template_slide->base.type, Slide_Used_Slide);
    template_slide->other_slide = other_slide;

    return template_slide;
}

inline
Text_Item *make_text(String s, Text_Item current_state)
{
    auto text_item = New<Text_Item>(false);

    *text_item = current_state;

    _set_type_Type(text_item->base.type, Text_Item);
    text_item->text      = copy_string(s);
    text_item->font_name = copy_string(current_state.font_name);

    return text_item;
}

void init_slide(Slide *slide)
{
    array_init(&slide->items);

    slide->left_margin  = -1;
    slide->right_margin = -1;
}

void copy(Text_Item source, Text_Item *dest)
{
    *dest           = source;
    dest->text      = copy_string(source.text);
    dest->font_name = copy_string(source.font_name);
}

#define error(c_agent, handler, ...)                                  \
    printf("[%s] Error at line %d: ", c_agent, handler.line_number);  \
    printf(__VA_ARGS__);                                              

bool check_slide_property(Slide *current_slide, Text_File_Handler *handler)
{
    if (!current_slide)
    {
        auto c_agent = (char*)temp_c_string(handler->log_agent);

        error(c_agent, (*handler), "Got a slide property when no slide was created.\n");
        return false;
    }

    return true;
}

bool check_text_property(Slide *current_slide, String defining_style, Text_File_Handler *handler)
{
    if (!(current_slide || defining_style))
    {
        auto c_agent = (char*)temp_c_string(handler->log_agent);

        error(c_agent, (*handler), "Got a text property when no slide or style was created.\n");
        return false;
    }

    return true;
}

void define_style(Text_File_Handler *handler, Slideshow *show, Text_Item state, String style_name)
{
    auto table = &show->style_table;
    auto [old_value, found] = table_find(table, style_name);

    if (found)
    {
        error(temp_c_string(handler->log_agent), (*handler), "Attempt to redeclare style '%s'.\n",
              temp_c_string(style_name));
        return;
    }

    auto state_copy = New<Text_Item>(false);
    copy(state, state_copy);

    table_add(table, copy_string(style_name), state_copy);
}

void end_style(Text_File_Handler *handler, Slideshow *show, Text_Item *state, String *style_name)
{
    define_style(handler, show, *state, *style_name);
    reset_to_defaults(state);
    free_string(style_name);
    *style_name = String("");
}

Dynamic_Font *get_font(Slideshow *show, Slide *slide, Text_Item *text)
{
    String font_name("KarminaBoldItalic.otf");

    if (text->font_name)
    {
        auto [asset_name, found] = table_find(&show->name_to_font_name, text->font_name);

        if (found)
        {
            font_name = asset_name;
        }
        else
        {
            logprint("slide_draw", "Error drawing slide %d: Unable to find an asset name for short name '%s'.\n",
                     current_slide_index, temp_c_string(text->font_name));
        }
    }

    auto integer_font_size = (i32)(text->size * render_target_height / 100.0);
    auto font = get_font_at_size(FONT_FOLDER, font_name, integer_font_size);
    return font;
}

// @Speed: May have to think about this because it has to load the dynamic font
// in order to get the line spacing.
f32 get_line_spacing(Slideshow *show, Slide *slide, Text_Item *text)
{
    auto font = get_font(show, slide, text);
    auto result = font->default_line_spacing / (f32)render_target_height;

    return result;
}

void add_line_to_slide(Slideshow *show, Slide *slide, Text_Item *state, String s)
{
    auto text_item = make_text(s, *state);
    array_add(&slide->items, &text_item->base);

    auto delta_y = -get_line_spacing(show, slide, state);
    delta_y *= state->line_spacing_factor;
    state->y_coordinate += delta_y;
}

Image_Item *parse_image_properties(Text_File_Handler handler, String input, String image_name)
{
    bool    loaded = true;
    f32     image_x = 50, image_y = 50;
    f32     image_rotation = 0;
    f32     image_scale = 100;
    Vector4 image_crop(0, 0, 0, 0); // Range is 0..100 in terms of 100%
    bool    image_should_crop = false;
    Vector4 image_color(1, 1, 1, 1);
    Vector4 image_border_color(1, 1, 1, 1);
    f32     image_border_width = 0.0f;
    Vector2 image_uvs(1, 1);

    char *c_agent = (char*)temp_c_string(handler.log_agent);

    while (input)
    {
        auto [command, args] = break_by_spaces(input);

        if (command == String("pos"))
        {
            bool success = false;
            auto [x, rhs] = string_to_float(args, &success);
            if (!success)
            {
                error(c_agent, handler, "Unable to parse X coordinate of the image.\n");
                loaded = false;
                break;
            }
            args = rhs;

            auto [y, rhs2] = string_to_float(args, &success);
            if (!success)
            {
                error(c_agent, handler, "Unable to parse Y coordinate of the image.\n");
                loaded = false;
                break;
            }
            args = rhs2;

            image_x = x;
            image_y = y;
        }
        else if (command == String("uv_scale"))
        {
            bool success = false;
            auto [u, rhs] = string_to_float(args, &success);
            if (!success)
            {
                error(c_agent, handler, "Unable to parse U scale of the image.\n");
                loaded = false;
                break;
            }
            args = rhs;

            auto [v, rhs2] = string_to_float(args, &success);
            if (!success)
            {
                error(c_agent, handler, "Unable to parse V scale of the image.\n");
                loaded = false;
                break;
            }
            args = rhs2;

            image_uvs.x = u;
            image_uvs.y = v;
        }
        else if (command == String("scale"))
        {
            bool success = false;
            auto [scale, rhs] = string_to_float(args, &success);
            if (!success)
            {
                error(c_agent, handler, "Unable to parse the scale of the image.\n");
                loaded = false;
                break;
            }
            args = rhs;

            image_scale = scale;
        }
        else if (command == String("border_width"))
        {
            bool success = false;
            auto [border_width, rhs] = string_to_float(args, &success);
            if (!success)
            {
                error(c_agent, handler, "Unable to parse the border width of the image.\n");
                loaded = false;
                break;
            }
            args = rhs;

            image_border_width = border_width;
        }
        else if (command == String("rotation"))
        {
            bool success = false;
            auto [dtheta, rhs] = string_to_float(args, &success);
            if (!success)
            {
                error(c_agent, handler, "Unable to parse the theta angle of the image (in degrees).\n");
                loaded = false;
                break;
            }
            args = rhs;

            image_rotation = dtheta;
        }
        else if (command == String("crop"))
        {
            bool success = false;
            auto [crop, rhs] = string_to_vec4(args, &success);
            if (!success)
            {
                error(c_agent, handler, "Unable to parse the crop of the image.\n");
                loaded = false;
                break;
            }
            args = rhs;

            image_crop = crop;
            image_should_crop = true;
        }
        else if (command == String("color"))
        {
            bool success = false;
            auto [color, rhs] = string_to_vec4(args, &success);
            if (!success)
            {
                error(c_agent, handler, "Unable to parse the color of the image.\n");
                loaded = false;
                break;
            }
            args = rhs;

            image_color = color;
        }
        else if (command == String("border_color"))
        {
            bool success = false;
            auto [color, rhs] = string_to_vec4(args, &success);
            if (!success)
            {
                error(c_agent, handler, "Unable to parse the border color of the image.\n");
                loaded = false;
                break;
            }
            args = rhs;

            image_border_color = color;
        }
        else
        {
            error(c_agent, handler, "Unknown image property '%s'.\n", temp_c_string(command));
            loaded = false;
            break;
        }

        input = args;
        eat_spaces(&input);
    }

    if (!loaded) return NULL;

    auto image = New<Image_Item>(false);

    _set_type_Type(image->base.type, Image_Item);
    image->name         = copy_string(image_name);
    image->position     = Vector2(image_x, image_y);
    image->scale        = image_scale;
    image->rotation     = image_rotation;
    image->color        = image_color;

    image->crop_left    = image_crop.x;
    image->crop_right   = image_crop.y;
    image->crop_top     = image_crop.z;
    image->crop_bottom  = image_crop.w;
    image->cropping_set = image_should_crop;

    image->border_color = image_border_color;
    image->border_width = image_border_width;

    image->uv_scale     = image_uvs;

    return image;
}

Slideshow *load_slideshow(String full_path)
{
    auto result = New<Slideshow>(false);
    result->aspect_x = 16.0f;
    result->aspect_y =  9.0f;
    array_init(&result->visible_slides);
    array_init(&result->all_slides);
    init(&result->name_to_font_name);
    init(&result->style_table);
    init(&result->texture_name_table);
    init(&result->slide_name_table);

    Text_File_Handler handler;

    String agent("load_slideshow");

    start_file(&handler, full_path, agent);
    if (handler.failed) return NULL;

    handler.strip_comments_from_ends_of_lines = false;

    String     defining_style_name;
    Slide     *current_slide = NULL;
    Text_Item  current_text_state;
    reset_to_defaults(&current_text_state);

    auto c_agent = (char*)temp_c_string(agent);

    while (true)
    {
        auto [line, found] = consume_next_line(&handler);

        if (!found) break;

        if (line[0] == ':')
        {
            advance(&line, 1);
            eat_spaces(&line);

            auto [command, remainder] = break_by_spaces(line);

            if (command == String("slide"))
            {
                if (defining_style_name)
                {
                    error(c_agent, handler, "Attempt to start a new style while defining a style ('%s'). Missing 'end_style'.\n", temp_c_string(defining_style_name));
                    end_style(&handler, result, &current_text_state, &defining_style_name);
                }

                String slide_name("");
                bool   visibility = true;

                if (remainder)
                {
                    auto [attempted_name, visibility_name] = break_by_spaces(remainder);

                    if (visibility_name == String("yes"))
                    {
                        visibility = true;
                    }
                    else if (visibility_name == String("no"))
                    {
                        visibility = false;
                    }
                    else if (visibility_name == String(""))
                    {
                    }
                    else
                    {
                        error(c_agent, handler, "Unable to recognize visibility name '%s' (Valid options are 'yes', 'no', and leaving it blank).\n", temp_c_string(visibility_name));
                    }

                    assert((attempted_name.count != 0));

                    auto [slide, found] = table_find(&result->slide_name_table, attempted_name);

                    if (found)
                    {
                        error(c_agent, handler, "Attempt to redefine slide '%s'.\n", temp_c_string(attempted_name))
                    }
                    else
                    {
                        slide_name = attempted_name;
                    }
                }

                auto slide = New<Slide>(false);
                init_slide(slide);

                if (slide_name)
                {
                    table_add(&result->slide_name_table, copy_string(slide_name), slide);
                }

                if (visibility)
                {
                    array_add(&result->visible_slides, slide);
                }

                array_add(&result->all_slides, slide);

                current_slide = slide;

                // Reset the text state for the new slide:
                reset_to_defaults(&current_text_state);
            }
            else if (command == String("background"))
            {
                if (!check_slide_property(current_slide, &handler)) continue;
                
                bool success = false;
                auto ret = string_to_vec4(remainder, &success);

                if (!success)
                {
                    error(c_agent, handler, "Unable to read background color.\n");
                    continue;
                }

                current_slide->background_color = ret.first;
                remainder = ret.second;
            }
            else if (command == String("text_color"))
            {
                if (!check_text_property(current_slide, defining_style_name, &handler)) continue;

                bool success = false;
                auto ret = string_to_vec4(remainder, &success);

                if (!success)
                {
                    error(c_agent, handler, "Unable to read text color.\n");
                    continue;
                }

                current_text_state.color = ret.first;
                remainder = ret.second;
            }
            else if (command == String("justify"))
            {
                if (!check_text_property(current_slide, defining_style_name, &handler)) continue;

                auto [type, rest] = break_by_spaces(remainder);

                if (rest)
                {
                    logprint(c_agent, "Junk at end of line %d: Got '%s'\n", handler.line_number, temp_c_string(rest));
                    continue;
                }

                Justification just;

                if (type == String("left"))
                    just = Justification::LEFT;
                else if (type == String("center"))
                    just = Justification::CENTER;
                else if (type == String("right"))
                    just = Justification::RIGHT;
                else
                {
                    error(c_agent, handler, "Justification type not supported '%s' (supported ones are 'left', 'center', 'right').\n", temp_c_string(type));
                    continue;
                }

                current_text_state.justification = just;
            }
            else if (command == String("y"))
            {
                if (!check_text_property(current_slide, defining_style_name, &handler)) continue;

                bool success = false;
                auto ret = string_to_float(remainder, &success);

                if (!success)
                {
                    error(c_agent, handler, "Unable to read y coordinates for text.\n");
                    continue;
                }

                current_text_state.y_coordinate = ret.first;
                remainder = ret.second;
            }
            else if (command == String("aspect"))
            {
                bool success = false;
                auto ret = string_to_float(remainder, &success);

                if (!success)
                {
                    error(c_agent, handler, "Unable to read width component of aspect ratio.\n");
                    continue;
                }

                result->aspect_x = ret.first;
                remainder = ret.second;

                ret = string_to_float(remainder, &success);

                if (!success)
                {
                    error(c_agent, handler, "Unable to read height component of aspect ratio.\n");
                    continue;
                }

                result->aspect_y = ret.first;
                remainder = ret.second;
            }
            else if (command == String("line_factor"))
            {
                if (!check_text_property(current_slide, defining_style_name, &handler)) continue;

                bool success = false;
                auto ret = string_to_float(remainder, &success);

                if (!success)
                {
                    error(c_agent, handler, "Unable to read line spacing factor for text.\n");
                    continue;
                }

                current_text_state.line_spacing_factor = ret.first;
                remainder = ret.second;
            }
            else if (command == String("size"))
            {
                if (!check_text_property(current_slide, defining_style_name, &handler)) continue;

                bool success = false;
                auto ret = string_to_float(remainder, &success);

                if (!success)
                {
                    error(c_agent, handler, "Unable to read font size for text.\n");
                    continue;
                }

                if (ret.first <= 0.0)
                {
                    error(c_agent, handler, "Invalid font size.\n");
                    continue;
                }

                current_text_state.size = ret.first;
                remainder = ret.second;
            }
            else if (command == String("left_margin"))
            {
                if (!check_slide_property(current_slide, &handler)) continue;

                bool success = false;
                auto ret = string_to_float(remainder, &success);

                if (!success)
                {
                    error(c_agent, handler, "Unable to left margin for slide.\n");
                    continue;
                }

                current_slide->left_margin = ret.first;
                remainder = ret.second;
            }
            else if (command == String("right_margin"))
            {
                if (!check_slide_property(current_slide, &handler)) continue;

                bool success = false;
                auto ret = string_to_float(remainder, &success);

                if (!success)
                {
                    error(c_agent, handler, "Unable to right margin for slide.\n");
                    continue;
                }

                current_slide->right_margin = ret.first;
                remainder = ret.second;
            }
            else if (command == String("load_image"))
            {
                auto [short_name, asset_name] = break_by_spaces(remainder);

                if (!short_name)
                {
                    error(c_agent, handler, "Did not find valid short name for image.\n");
                    continue;
                }

                if (!asset_name)
                {
                    error(c_agent, handler, "Got image short name but didn't find image full name.\n");
                    continue;
                }

                auto map = catalog_find(&texture_catalog, asset_name);
                if (map == NULL)
                {
                    error(c_agent, handler, "Couldn't find the requested name '%s' for image in the data folder.\n",
                          temp_c_string(asset_name));
                    continue;
                }

                auto table = &result->texture_name_table;
                auto [dummy, found] = table_find(table, short_name);

                if (found)
                {
                    error(c_agent, handler, "Attempt to redefine image '%s'.\n",
                          temp_c_string(short_name));
                    continue;
                }

                table_add(table, copy_string(short_name), copy_string(asset_name));
            }
            else if (command == String("image"))
            {
                if (!check_slide_property(current_slide, &handler)) continue;

                auto [image_name, rhs] = break_by_spaces(remainder);
                remainder = rhs;

                auto [asset_name, found] = table_find(&result->texture_name_table, image_name);

                if (!found)
                {
                    error(c_agent, handler, "Attempt to use undefined image '%s'!\n", temp_c_string(image_name));
                    continue;
                }

                auto image = parse_image_properties(handler, remainder, image_name);
                if (image)
                {
                    array_add(&current_slide->items, &image->base);
                }
            }
            else if (command == String("declare_font"))
            {
                auto [short_name, asset_name] = break_by_spaces(remainder);

                if (!short_name)
                {
                    error(c_agent, handler, "Did not find valid short name for font.\n");
                    continue;
                }

                if (!asset_name)
                {
                    error(c_agent, handler, "Got font short name but didn't find font path.\n");
                    continue;
                }

                auto table = &result->name_to_font_name;

                if (table_find_pointer(table, short_name) != NULL)
                {
                    error(c_agent, handler, "Redeclaration of font name '%s'!\n", temp_c_string(short_name));
                    continue;
                }

                table_add(table, copy_string(short_name), copy_string(asset_name));
            }
            else if (command == String("font"))
            {
                if (!check_text_property(current_slide, defining_style_name, &handler)) continue;

                auto query_font = remainder;

                auto [asset_name, found] = table_find(&result->name_to_font_name, query_font);

                if (!found)
                {
                    error(c_agent, handler, "Attempt to use invalid font name '%s'!\n", temp_c_string(query_font));
                    continue;
                }

                if (current_text_state.font_name)
                {
                    free_string(&current_text_state.font_name);
                }

                current_text_state.font_name = copy_string(query_font);
            }
            else if (command == String("use_slide"))
            {
                if (!check_slide_property(current_slide, &handler)) continue;

                auto query_slide = remainder;

                if (!query_slide)
                {
                    error(c_agent, handler, "Attempt to use slide but the name for the template slide is blank.\n");
                    continue;
                }

                auto [template_slide, found] = table_find(&result->slide_name_table, query_slide);

                if (!found)
                {
                    error(c_agent, handler, "Attempt to use undefined template slide '%s'!.\n", temp_c_string(query_slide));
                    continue;
                }

                auto included_slide = make_slide_used_slide(template_slide);
                array_add(&current_slide->items, &included_slide->base);

                current_slide->background_color = template_slide->background_color;
                current_slide->left_margin      = template_slide->left_margin;
                current_slide->right_margin     = template_slide->right_margin;
            }
            else if (command == String("begin_style"))
            {
                if (defining_style_name)
                {
                    error(c_agent, handler, "Already defining a style (named '%s').\n",
                          temp_c_string(defining_style_name));
                    define_style(&handler, result, current_text_state, defining_style_name);
                }

                if (defining_style_name)
                    free_string(&defining_style_name);
                defining_style_name = copy_string(remainder);
            }
            else if (command == String("end_style"))
            {
                if (remainder)
                {
                    error(c_agent, handler, "Junk at end of line.\n");
                }

                if (!defining_style_name)
                {
                    error(c_agent, handler, "'end_style' without a matching 'begin_style'.\n");
                    continue;
                }

                end_style(&handler, result, &current_text_state, &defining_style_name);
            }
            else if (command == String("style"))
            {
                auto query_style = remainder;

                if (!query_style)
                {
                    error(c_agent, handler, "Must supply a style name to use.\n");
                    continue;
                }

                auto [style, success] = table_find(&result->style_table, query_style);

                if (!success)
                {
                    error(c_agent, handler, "Could not find a style named '%s'.\n",
                          temp_c_string(query_style));
                    continue;
                }

                reset_to_defaults(&current_text_state);
                copy(*style, &current_text_state);
            }
            else
            {
                printf("************************************ COMMAND: '%s', REMAINDER: '%s'\n",
                       temp_c_string(command), temp_c_string(remainder));
            }
        }
        else
        {
            if (!check_slide_property(current_slide, &handler)) continue;

            assert(line.count > 0);

            if (line[0] == '\\')
            {
                advance(&line, 1);
            }

            add_line_to_slide(result, current_slide, &current_text_state, line);
        }
    }

    if (current_text_state.font_name)
    {
        free_string(&current_text_state.font_name);
    }

    if (defining_style_name)
    {
        free_string(&defining_style_name);
    }

    deinit(&handler);

    return result;
}

void unload_slideshow(Slideshow *slideshow)
{
    // @Leaking
    // printf("[slideshow]: Freeing memory for old slideshow....\n");

    for (auto slide : slideshow->all_slides)
    {
        for (auto item : slide->items)
        {
            if (cmp_var_type_to_type(item->type, Text_Item))
            {
                auto text_item = CastSlideItem<Text_Item>(item);
                if (text_item->text) // Doing this because we have blank lines
                    free_string(&text_item->text);
                if (text_item->font_name)
                    free_string(&text_item->font_name);
            }

            if (cmp_var_type_to_type(item->type, Image_Item))
            {
                auto image = CastSlideItem<Image_Item>(item);
                free_string(&image->name);
            }

            my_free(item);
        }

        array_free(&slide->items);

        my_free(slide);
    }

    array_free(&slideshow->all_slides);
    array_free(&slideshow->visible_slides);

    for (auto pair : slideshow->style_table)
    {
        free_string(&pair.key);

        auto font_name = &(pair.value->font_name);
        if (*font_name)
        {
            free_string(font_name);
        }

        auto text = &(pair.value->font_name);
        if (*text)
        {
            free_string(text);
        }

        my_free(pair.value);
    }
    deinit(&slideshow->style_table);

    for (auto pair : slideshow->slide_name_table)
    {
        free_string(&pair.key);
    }
    deinit(&slideshow->slide_name_table);

    for (auto pair : slideshow->texture_name_table)
    {
        free_string(&pair.key);
        free_string(&pair.value);
    }
    deinit(&slideshow->texture_name_table);

    for (auto pair : slideshow->name_to_font_name)
    {
        free_string(&pair.key);
        free_string(&pair.value);
    }
    deinit(&slideshow->name_to_font_name);
    
    my_free(slideshow);
}
