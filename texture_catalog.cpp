#include "texture_catalog.h"

#include <stb_image.h>
#include "opengl.h"

void texture_register_loose_file(Catalog_Base *base, String short_name, String full_name);
void texture_perform_reload_or_creation(Catalog_Base *base, String short_name, String full_name, bool do_load_asset);

void init_bitmap(Bitmap *bitmap)
{
    bitmap->width  = 0;
    bitmap->height = 0;
    bitmap->data   = NULL;
    bitmap->num_mipmap_levels = 1;
    bitmap->length_in_bytes = 0;
}

void init_texture_map(Texture_Map *map)
{
    // init_string(&map->name);
    // init_string(&map->full_path);
    map->id     = 0; // 0xffffffff;
    map->fbo_id = 0; // 0xffffffff;
    map->width  = 0;
    map->height = 0;
    map->data   = NULL;
    map->dirty  = false;
    map->loaded = false;
}

void init_texture_catalog(Texture_Catalog *catalog)
{
    // @Temporary @Hack
    // Set uv flip for stb here
    stbi_set_flip_vertically_on_load(true);

    // @Fixme: Loading from the folder names 'my_name' for now
    catalog->base.my_name = String("textures");
    array_add(&catalog->base.extensions, String("jpg"));
    array_add(&catalog->base.extensions, String("png"));
    // array_add(&catalog->base.extensions, String("bmp"));
    // array_add(&catalog->base.extensions, String("tga"));

    catalog->base.proc_register_loose_file        = texture_register_loose_file;
    catalog->base.proc_perform_reload_or_creation = texture_perform_reload_or_creation;

    init(&catalog->table);
}

void deinit_texture_catalog(Texture_Catalog *catalog)
{
    array_free(&catalog->base.extensions);

    for (auto it : catalog->table)
    {
        my_free(it.key.data);

        auto map = it.value;

        my_free(map->name.data);
        my_free(map->full_path.data);

        stbi_image_free(map->data->data);
        my_free(map->data);
        my_free(map);
    }

    deinit(&catalog->table);
}

Texture_Map *make_placeholder(Texture_Catalog *catalog, String short_name, String full_path)
{
    Texture_Map *map = New<Texture_Map>(false);
    init_texture_map(map);
    map->name      = copy_string(short_name);
    map->full_path = copy_string(full_path);

    return map;
}

void reload_asset(Texture_Catalog *catalog, Texture_Map *map)
{
    if (!map->full_path)
    {
        fprintf(stderr, "Received a texture map that did not have a full path! (%s)\n",
                temp_c_string(map->name));
        return;
    }

    i32 width, height;
    i32 components;

    auto c_path = temp_c_string(map->full_path);

    u8 *data = stbi_load((char*)c_path, &width, &height, &components, 0);
    if (!data)
    {
        fprintf(stderr, "FAILED to load bitmap '%s'\n", temp_c_string(map->name));
        return;
    }

    /*
    // @Temporary
    if (map->data)
    {
        stbi_image_free(map->data->data);
        my_free(map->data);
        map->data = NULL;
    }
    */

    // We leave the Bitmap attached for now.
    // This is probably a temporary thing...
    auto result = New<Bitmap>(false);
    result->width  = width;
    result->height = height;
    result->data   = data;
    result->length_in_bytes = width * height * components;

    printf("Bitmap '%s' is %dx%d, %d components.\n", temp_c_string(map->name), width, height, components);

    // @Temporary @Hack @Fixme
    if (components == 3)
    {
        result->format = Texture_Format::RGB888;
    }
    else
    {
        result->format = Texture_Format::ARGB8888;
    }

    // @Cutnpaste from create_texture ??? @Fixme: we should make a create_texture
    map->width  = result->width;
    map->height = result->height;
    map->data   = result;

    update_texture(map);
}

void texture_register_loose_file(Catalog_Base *base, String short_name, String full_name)
{
    Texture_Catalog *tc = (Texture_Catalog*)(base);
    Texture_Map *new_map = make_placeholder(tc, short_name, full_name);

    reload_asset(tc, new_map);
    new_map->loaded = true;
    auto table_key = copy_string(new_map->name);
    table_add(&tc->table, table_key, new_map);
    printf("Added texture '%s'\n", temp_c_string(short_name));
}

void texture_perform_reload_or_creation(Catalog_Base *base, String short_name, String full_name, bool do_load_asset)
{
    assert(0);
}

bool is_supported(Texture_Format format)
{
    // @Incomplete: @Fixme:
    return true;
}
