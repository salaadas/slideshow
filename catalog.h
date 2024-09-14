#pragma once

#include "common.h"

#include "table.h"

struct Catalog_Base;

typedef void(*ProcRegisterLooseFile)(Catalog_Base *base,
                                     String short_name, String full_name);
typedef void(*ProcPerformReloadOrCreation)(Catalog_Base *base,
                                           String short_name, String full_name, bool do_load_asset);

//
// We separate Catalog_Base from Catalog, so that we can put Catalog_Base pointers into arrays, 
// pass to procedures, etc, without needing to know the full type of the Catalog.
//
struct Catalog_Base
{
    // @Note: Not needed right now because we are using the folder with the same name as my_name
    // RArr<String>                folders;
    RArr<String>                extensions;

    RArr<String>                short_names_to_reload;
    RArr<String>                full_names_to_reload;

    String                      my_name; // Name for this catalog

    ProcRegisterLooseFile       proc_register_loose_file        = NULL;
    ProcPerformReloadOrCreation proc_perform_reload_or_creation = NULL;
};

template <typename V>
struct Catalog
{
    Catalog_Base      base;
    Table<String, V*> table;
};

void   perform_reloads(Catalog_Base *base);
String get_extension(String filename);
void   catalog_loose_files(String folder, RArr<Catalog_Base*> *catalogs);

template <typename V>
void reload_asset(Catalog<V> *catalog, V *value);

template <typename V>
V *catalog_find(Catalog<V> *catalog, String short_name)
{
    auto tf      = table_find(&catalog->table, short_name);
    auto value   = tf.first;
    auto success = tf.second;

    if (!success || !value)
    {
        fprintf(stderr, "Catalog %s was not able to find asset %s\n",
                catalog->base.my_name.data, short_name.data);
        return NULL;
    }

    if (!value->loaded)
    {
        reload_asset(catalog, value);
        value->loaded = true;
    }

    return value;
}

// @Important: For a catalog, ex Shader_Catalog, we must define the below two functions
// void XXX_register_loose_file(Catalog_Base *base, String short_name, String full_name);
// void XXX_perform_reload_or_creation(Catalog_Base *base, String short_name, String full_name, bool do_load_asset);

// template <typename V>
// void do_polymorphic_inits(Catalog<V> *catalog);
// {
//     catalog->base.proc_register_loose_file        = register_loose_file<V>;
//     catalog->base.proc_perform_reload_or_creation = perform_reload_or_creation<V>;
// }
