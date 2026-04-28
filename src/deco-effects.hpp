#pragma once
#include <wayfire/option-wrapper.hpp>
#include <wayfire/core.hpp>
#include <wayfire/opengl.hpp>
#include <map>
#include <GLES3/gl32.h>
#include <wayfire/scene-render.hpp>

namespace wf
{
namespace vkdecor
{
class smoke_t
{
    /** background effects */
    GLuint render_overlay_program,
        texture;

    int saved_width = -1, saved_height = -1;

    wf::option_wrapper_t<std::string> overlay_engine{"vkdecor/overlay_engine"};
    wf::option_wrapper_t<int> rounded_corner_radius{"vkdecor/rounded_corner_radius"};
    wf::option_wrapper_t<wf::color_t> shadow_color{"vkdecor/shadow_color"};

  public:
    smoke_t();
    ~smoke_t();

    void run_shader(GLuint program, int width, int height, int title_height, int border_size, int radius);
    void run_shader_region(GLuint program, const wf::region_t & region, const wf::dimensions_t & size);
    void dispatch_region(const wf::region_t& region);

    void step_effect(const wf::scene::render_instruction_t& data, wf::geometry_t rectangle,
        bool ink, wf::pointf_t p, wf::color_t decor_color,
        int title_height, int border_size, int shadow_radius);
    void render_effect(const wf::scene::render_instruction_t& data, wf::geometry_t rectangle);
    void recreate_textures(wf::geometry_t rectangle);
    void create_programs();
    void destroy_programs();
    void create_textures();
    void destroy_textures();
    void effect_updated();
};
}
}
