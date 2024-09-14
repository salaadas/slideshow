#pragma once

#include "common.h"
#include "font.h"
#include "catalog.h"
#include "texture_catalog.h"
#include "shader_catalog.h"

extern Shader *shader_argb_no_texture;
extern Shader *shader_argb_and_texture;
extern Shader *shader_text;

void init_shaders();
void rendering_2d_right_handed();
void draw_slideshow();

void draw_prepared_text(Dynamic_Font *font, i64 x, i64 y, Vector4 color);
i64  draw_text(Dynamic_Font *font, i64 x, i64 y, String text, Vector4 color);
void draw_generated_quads(Dynamic_Font *font, Vector4 color);
void draw_letter_quad(Font_Quad q, Vector4 color);
void draw_centered(Dynamic_Font *font, f32 y_unit_scale, String text, Vector4 color);
