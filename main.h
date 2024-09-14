#pragma once

#include "common.h"

#include "catalog.h"
#include "shader_catalog.h"
#include "texture_catalog.h"
#include "slides.h"

extern String              dir_of_running_exe;
extern RArr<Catalog_Base*> all_catalogs;
extern Shader_Catalog      shader_catalog;
extern Texture_Catalog     texture_catalog;
extern bool                should_quit;

// extern const String SOUND_FOLDER;
// extern const String MUSIC_FOLDER;
extern const String FONT_FOLDER;
extern          i32 BIG_FONT_SIZE;

extern Slideshow *the_slideshow;
extern i32        current_slide_index;
