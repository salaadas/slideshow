// font rendering
//   ^~~~~~~~~~~~~~~~~~~~~~~~~~ current task
// hot loading

// mising the catalog (for argb_no_textures, argb_and_textures, shaders), perform_reloads for each catalog

// missing the immediate_begin, immediate_flush()

// image loading
// make Texture_Map struct (which is essentially Framebuffer with more properties) ??
//   ^~~~~~~ or is it an image??
// We also have the Bitmap structs which are images I think

// @Important: It is crucial to include "common.h" at the top in main.cpp
#include "common.h"

// OpenGL
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3native.h>

// GLM
// @Note: GLM uses a column major ordering for matrices
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
// Linux
#include <unistd.h>
#include <X11/Xlib.h>
#include <limits.h>

#include <stb_image.h>

#include "main.h"
#include "file_utils.h"
#include "path_utils.h"
#include "opengl.h"
#include "time_info.h"
#include "draw.h"
#include "font.h"
#include "cursor.h"
#include "events.h"
#include "user_control.h"
#include "hotloader.h"
#include "audio.h"
// #include "sound_catalog.h"
// #include "mesh_catalog.h"
// #include "sound.h"

// #define NON_RESIZABLE_MODE

const f32    DT_MAX = 0.15f;
const i32    DESIRED_WIDTH  = 1600;
const i32    DESIRED_HEIGHT = 900;
const String PROGRAM_NAME("show");
const String FONT_FOLDER("data/fonts/");
// @Fixme: Set this font size depending on the window's size
i32          BIG_FONT_SIZE = 32;

// True if set from command-line args
bool         window_dimension_set = false; // @Fixme: unhandled
bool         should_quit = false;
f32          windowed_aspect_ratio_h_over_w;
Display     *x_global_display = NULL;
String       dir_of_running_exe;

bool         was_resized    = true; // Set to true to resize on first frame
i32          resized_width  = DESIRED_WIDTH;
i32          resized_height = DESIRED_HEIGHT;
// Sound_Player *sound_player;
Slideshow   *the_slideshow;
i32          current_slide_index = 0;

RArr<Catalog_Base*> all_catalogs;
Shader_Catalog      shader_catalog;
Texture_Catalog     texture_catalog;
// Animation_Catalog       animation_catalog;
// Animation_Names_Catalog animation_names_catalog;
// Animation_Graph_Catalog animation_graph_catalog;

// Mesh_Catalog    mesh_catalog;
// Sound_Catalog   sound_catalog;

// @Hardcode:
bool is_fullscreen = false;
#include <X11/Xatom.h>
void toggle_fullscreen()
{
    assert((x_global_display != NULL));

    auto window = glfwGetX11Window(glfw_window);

    Atom wm_state      = XInternAtom(x_global_display, "_NET_WM_STATE", true);
    Atom wm_fullscreen = XInternAtom(x_global_display, "_NET_WM_STATE_FULLSCREEN", true);

    XEvent x_event;
    memset(&x_event, 0, sizeof(XEvent));

    x_event.type = ClientMessage;
    x_event.xclient.window = window;
    x_event.xclient.message_type = wm_state;
    x_event.xclient.format = 32;

    if (is_fullscreen)
        x_event.xclient.data.l[0] = false;
    else
        x_event.xclient.data.l[0] = true;

    x_event.xclient.data.l[1] = wm_fullscreen;
    x_event.xclient.data.l[2] = 0;

    XSendEvent(x_global_display, DefaultRootWindow(x_global_display), False, SubstructureRedirectMask | SubstructureNotifyMask, &x_event);

    is_fullscreen = !is_fullscreen;
}

// Error callback for GLFW
void glfw_error_callback(i32 error, const char *description);

void glfw_window_size_callback(GLFWwindow *window, i32 new_width, i32 new_height);

void init_window()
{
    String exe = get_executable_path();

    i32 last_slash = find_index_from_right(exe, '/');
    exe.count = last_slash; // Upto but not including the last slash

    dir_of_running_exe = copy_string(exe);
    setcwd(dir_of_running_exe);

    // Reset temporary storage here because we use a lot of memory in get_executable_path
    // This is due to allocating PATH_MAX amount for the buffer storing the exe path
    reset_temporary_storage();
}

void init_gl(i32 render_target_width, i32 render_target_height, bool vsync = true, bool windowed = true)
{
    // Set the error callback first before doing anything
    glfwSetErrorCallback(glfw_error_callback);

    // Handle error
    assert(glfwInit() == GLFW_TRUE);

    // Hints the about-to-created window's properties using:
    // glfwWindowHint(i32 hint, i32 value);
    // to reset all the hints to their defaults:
    // glfwDefaultWindowHints();
    // ^ good idea to call this BEFORE setting any hints BEFORE creating any window

#ifdef NON_RESIZABLE_MODE
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
#endif

    {
        i32 width, height;
        width  = render_target_width;
        height = render_target_height;
        if (height < 1) height = 1;
        windowed_aspect_ratio_h_over_w = height / (f32)width;

        if (!window_dimension_set)
        {
            i32 limit_w, limit_h;

            // @Note: X11 way to get the dimension of the screen
            {
                x_global_display = glfwGetX11Display();
                assert((x_global_display != NULL));

                auto display = x_global_display;
                auto snum    = DefaultScreen(display);

                i32 desktop_height = DisplayHeight(display, snum);
                i32 desktop_width  = DisplayWidth(display, snum);

                // @Fixme: The screen query here is actually wrong because it merges both monitors into one.
                printf("              -----> Desktop width %d, height %d\n", desktop_width, desktop_height);

                limit_h = (i32)desktop_height;
                limit_w = (i32)desktop_width;
            }

            i32 other_limit_h = (i32)(limit_w * windowed_aspect_ratio_h_over_w);
            i32 limit = limit_h < other_limit_h ? limit_h : other_limit_h; // std::min(limit_h, other_limit_h);

            if (height > limit)
            {
                f32 ratio = limit / (f32)height;
                height    = (i32)(height * ratio);
                width     = (i32)(width  * ratio);
            }

            // @Fixme: Should we set the render_target_height and render_target_width like this?
            render_target_height = height;
            render_target_width  = width;
        }
    }

    // Creates both the window and context with which to render into
    if (windowed) glfw_window = glfwCreateWindow(render_target_width, render_target_height, (char*)PROGRAM_NAME.data, NULL, NULL);
    else          glfw_window = glfwCreateWindow(render_target_width, render_target_height, (char*)PROGRAM_NAME.data,
                                                 glfwGetPrimaryMonitor(), NULL);

    // Before we can use the context, we need to make it current
    glfwMakeContextCurrent(glfw_window);

    if (!vsync) glfwSwapInterval(0);

    // Hide the OS cursor
    glfwSetInputMode(glfw_window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

    hook_our_input_event_system_to_glfw(glfw_window);

    // Properties that can be controlled after creating the window:
    // - glfwSetWindowSize(GLFWwindow *window, i32 width, i32 height);
    // - glfwSetWindowPos(GLFWwindow *window, i32 x_pos, i32 y_pos);
    // similarly, we can:
    // - glfwGetWindowSize(GLFWwindow *window, i32 *width, i32 *height);
    // - glfwGetWindowPos(GLFWwindow *window, i32 *x_pos, i32 *y_pos);
    // or if you want to set a callback to the size and position of the window when it is changed, do:
    // - glfwSetWindowSizeCallback(...);
    // - glfwSetWindowPosCallback(...);
    glfwSetWindowSizeCallback(glfw_window, glfw_window_size_callback);

    // glfwGetWindowSize() returns the size of the window in pixels, which is skewed if the window system
    // uses scaling.
    // To retrieve the actual size of the framebuffer, use
    // glfwGetFrambuffersize(GLFWwindow *window, i32 *width, i32 *height);
    // you can also do
    // glfwSetFramebuffersizeCallback(...);


    // GLFW provides a mean to associate your own data with a window:
    // void *glfwGetWindowUserPointer(GLFWwindow *window);
    // glfwSetWindowUserPointer(GLFWwindow *window, void *pointer);


    // @Important: NOW COMES THE OPENGL GLUE THAT ALLOWS THE USE OF OPENGL FUNCTIONS
    // this is where we use the "glad.h" lib
    // we must set this up before using any OpenGL functions
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    // check for DSA support
    if (!GLAD_GL_ARB_direct_state_access)
    {
        fprintf(stderr, "GLAD: DSA is not supported\n");
        exit(1);
    }

    // @Fixme @Leak: not freeing the framebuffers at then end
    {
        auto result = create_texture_rendertarget(the_offscreen_buffer_width, the_offscreen_buffer_height,
                                                  false, false);
        the_offscreen_buffer = result.first;

        // @Fixme: So many similar variable names for sizes
        auto back_buffer_width  = render_target_width;
        auto back_buffer_height = render_target_height;
        
        the_back_buffer = New<Texture_Map>(false);
        init_texture_map(the_back_buffer);
        the_back_buffer->width  = back_buffer_width;
        the_back_buffer->height = back_buffer_height;
    }

    object_to_world_matrix = Matrix4(1.0);
    world_to_view_matrix   = Matrix4(1.0);
    view_to_proj_matrix    = Matrix4(1.0);
    object_to_proj_matrix  = Matrix4(1.0);

    num_immediate_vertices = 0;
    should_vsync           = vsync;
    in_windowed_mode       = windowed;

    glGenVertexArrays(1, &opengl_is_stupid_vao);
    glGenBuffers(1, &immediate_vbo);
    glGenBuffers(1, &immediate_vbo_indices);
}

void init_window_and_gl(i32 width, i32 height)
{
    render_target_width  = width;
    render_target_height = height;

    init_window(); // This modifies the render_target_width and render_target_height accordingly
    init_gl(render_target_width, render_target_height);
}

void init_context()
{
    global_context.allocator.proc    = __default_allocator;
    global_context.allocator.data    = NULL; // since regular malloc doesn't have a pointer to memory

    global_context.temporary_storage = &__default_temporary_storage;
}

void resize_offscreen_buffer_size(i32 new_width, i32 new_height)
{
    the_back_buffer->width  = new_width;
    the_back_buffer->height = new_height;

    f32  back_buffer_width  = new_width;
    f32  back_buffer_height = new_height;
    f32  back_buffer_denom  = std::max(back_buffer_height, 1.0f);
    auto window_aspect      = back_buffer_width / back_buffer_denom;

    auto desired_aspect_x = the_slideshow->aspect_x;
    auto desired_aspect_y = the_slideshow->aspect_y;
    auto desired_denom    = std::max((f32)desired_aspect_y, 1.0f);
    auto desired_aspect   = desired_aspect_x / desired_denom;

    f32 w, h;
    if (window_aspect > desired_aspect)
    {
        // Using the height of the back buffer for the offscreen buffer
        h = back_buffer_height;
        w = h * desired_aspect;
    }
    else
    {
        // Using the width of the back buffer for the offscreen buffer
        w = back_buffer_width;
        h = w / desired_aspect;
    }

    if (w < 1) w = 1;
    if (h < 1) h = 1;

    the_offscreen_buffer->width  = (i32)(floorf(w));
    the_offscreen_buffer->height = (i32)(floorf(h));
    size_color_target(the_offscreen_buffer, false);
}

void update_audio()
{
}

i64 highest_water = -1;
i64 frame_index = 0;

void do_one_frame()
{
    // frame_index += 1;
    // i64 hw = std::max(highest_water, global_context.temporary_storage->high_water_mark);
    // printf("[frame no.%ld] Highest water mark is %ld\n", frame_index, hw);

    // if (hw > highest_water)
    // {
    //     highest_water = hw;
    //     log_ts_usage();
    // }

    reset_temporary_storage();

    update_time(DT_MAX);

    update_linux_events();
    glfwPollEvents();

    // This must happend after the glfwPollEvents
    // @Note: was_resized is set in glfw_window_size_callback function
    if (was_resized)
    {
        resize_offscreen_buffer_size(resized_width, resized_height);
        // Because it they depends on the height of the render target
        deinit_all_font_stuff_on_resize();

        was_resized = false;
    }

    read_input();

    update_cursor();

    // @Temporary:
    {
        // simulate_game();

        // auto manager = get_entity_manager();
        // update_transition(manager);
    }

    update_audio();

    // @Temporary:
    {
        // auto manager = get_entity_manager();
        // --- hide cursor if focus application
        // update_game_camera(manager);
        // update_signs()manager;
        // draw_game_view_3d(manager);

        // set_depth_target(0, NULL);
        // glDisable(GL_DEPTH_TEST);

        // resolve_to_ldr(manager);

        // glEnable(GL_BLEND);
        // draw_hud(hud_i0, hud_j0, hud_width, hud_height);

        // draw_transition();
    }

    draw_slideshow();

    glfwSwapBuffers(glfw_window);

    while (hotloader_process_change()) {}
    // @Incomplete:
    // for (auto it : all_catalogs) perform_reloads(it);
}

// @ForwardDeclare
void my_hotloader_callback(Asset_Change *change, bool handled);


int main()
{
    init_context();

    // @Note: Init all the catalogs
    init_shader_catalog(&shader_catalog);
    init_texture_catalog(&texture_catalog);

    // @Note: Then, add catalogs into the catalog table
    array_add(&all_catalogs, &shader_catalog.base);
    array_add(&all_catalogs, &texture_catalog.base);

    init_window_and_gl(DESIRED_WIDTH, DESIRED_HEIGHT);

    catalog_loose_files(String("data"), &all_catalogs);

    newline();

    reset_temporary_storage();

    // @Important: Cannot init these after before is initted!!!
    // example:
    // white_texture    = catalog_find(&texture_catalog, "white");
    // the_missing_mesh = catalog_find(&texture_catalog, "missing_asset");

    // We init the shaders (after the catalog_loose_files)
    init_shaders();

    the_slideshow = load_slideshow(String("data/shows/my.show"));
    assert((the_slideshow));

    // @Incomplete
    // for (auto it : all_catalogs) hotloader_register_catalog(it);
    hotloader_init(); 
    hotloader_register_callback(my_hotloader_callback);
    
    while (!glfwWindowShouldClose(glfw_window))
    {
        if (should_quit) break;

        do_one_frame();
    }

    newline();
    printf("Exiting..\n");

    glfwDestroyWindow(glfw_window);
    glfwTerminate();

    hotloader_shutdown();

    newline();
    printf("Giving the OS all the allocated memory....\n");

    return(0);
}

// Error callback for GLFW
void glfw_error_callback(i32 error, const char *description)
{
    fprintf(stderr, "GLFW ERROR [%d]: %s", error, description);
    exit(1);
}

void glfw_window_size_callback(GLFWwindow *window, i32 new_width, i32 new_height)
{
    glViewport(0, 0, new_width, new_height);

    resized_width  = new_width;
    resized_height = new_height;
    was_resized = true;
}

void my_hotloader_callback(Asset_Change *change, bool handled)
{
    if (handled) return;

    auto full_name = change->full_name;

    // logprint("hotloader_callback", "Non-catalog asset change: %s\n", temp_c_string(full_name));

    if (change->extension == String("show"))
    {
        // @Incomplete: Only reload this if it is the actual slideshow we are currently viewing!!
        unload_slideshow(the_slideshow);
        the_slideshow = load_slideshow(full_name);
    }
}
