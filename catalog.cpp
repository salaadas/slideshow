#include "catalog.h"

#include "file_utils.h"

void perform_reloads(Catalog_Base *base)
{
    assert(base->short_names_to_reload.count == base->full_names_to_reload.count);

    i64 index = 0;
    for (auto it : base->short_names_to_reload)
    {
        auto short_name = copy_string(it);
        auto full_name  = copy_string(base->full_names_to_reload[index]);

        base->proc_perform_reload_or_creation(base, short_name, full_name, true);

        index += 1;

        my_free(short_name.data);
        my_free(full_name.data);
    }

    array_reset(&base->short_names_to_reload);
    array_reset(&base->full_names_to_reload);
}

String get_extension(String filename)
{
    auto index = find_index_from_right(filename, '.');

    if (index == -1)
    {
        return String("");
    }

    index += 1;

    String result = copy_string(filename, {global_context.temporary_storage, __temporary_allocator});
    result.data += index;
    result.count -= index;

    return result;
}

void add_relevent_files_and_parse(String short_name, String full_name, void *data)
{
    auto catalog = (Catalog_Base*)data;
    auto extensions_list = (RArr<String>*)(&catalog->extensions);
    String ext = get_extension(short_name);

    if (ext.count)
    {
        short_name.count -= 1;
        short_name.count -= ext.count;
    }

    if (!ext || !array_find(extensions_list, ext)) return;

    // printf("Registering %s, ext: %s\n", full_name.data, temp_c_string(ext));
    // Be sure to copy_string the names in your proc because short_name is allocated in temp storage
    catalog->proc_register_loose_file(catalog, short_name, full_name);
}

// Loop through the catalog base
// Check out the folder with the name in my_name
// Add all files that have the extension(s) supported
// Each type of catalog will have its own way of registering loose file
//   or performing reload. @Important: THESE PROCEDURES MUST BE DEFINE BY YOU
// Because of this, when we load them, parse the data and such.
void catalog_loose_files(String folder, RArr<Catalog_Base*> *catalogs)
{
    printf("Entering '%s' to do work...\n", folder.data);
    printf("Count of catalogs: %ld\n", catalogs->count);

    for (auto it : *catalogs)
    {
        // @Note: May change how we deal with folders later
        auto path = tprint(String("%s/%s"), folder.data, it->my_name.data);

        bool success = visit_files(path, it, add_relevent_files_and_parse);
        if (!success)
        {
            fprintf(stderr, "Could not visit %s...\n", path.data);
            assert(0);
        }
    }
}
