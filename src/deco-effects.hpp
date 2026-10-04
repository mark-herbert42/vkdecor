#pragma once
#include <wayfire/option-wrapper.hpp>
#include <wayfire/core.hpp>
#include <wayfire/opengl.hpp>
#include <map>
#include <GLES3/gl32.h>
#include <wayfire/scene-render.hpp>
#include <wayfire/vulkan.hpp>

#if WF_HAS_VULKANFX
#include "../shaders/rounded_corner.frag.h"
#include "../shaders/rounded_corner.vert.h"
#endif

namespace wf
{

namespace vkdecor
{

    // Порядок и выравнивание должны совпадать с блоком Params в шейдере:
    // vec4 идут первыми (выравнивание 16), затем int-ы (выравнивание 4).
struct push_constants_t
{
    glm::mat4 mvp;
    int   title_height;
    int   border_size;
    int   width;
    int   height;
    int   corner_radius;
    int   shadow_radius;
    float _pad[2];          // std430: vec4 требует выравнивание 16 байт
    glm::vec4 shadow_color;  // offset 32
    glm::vec4 border_color;  // offset 48
};
//static_assert(sizeof(push_constants_t) == 64, "push constants must be 64 bytes");
   // 64 байта

class smoke_t
{
    /** background effects */
    GLuint render_overlay_program,
        texture;

    int saved_width = -1, saved_height = -1; 
    wf::color_t saved_color; 

#if WF_HAS_VULKANFX
    std::shared_ptr<wf::vk::gpu_buffer_t> vulkan_vertex_buffer;

    // Push-константы для рендера скруглённых углов: заполняются в step_effect,
    // используются в render_effect (subpass), где параметров окна уже нет.
    push_constants_t push_constants{};

    std::shared_ptr<wf::vk::gpu_buffer_t> find_buffer(
        std::shared_ptr<wf::vk::context_t> ctx, VkDeviceSize total_size);
#endif


    wf::option_wrapper_t<std::string> overlay_engine{"vkdecor/overlay_engine"};
    wf::option_wrapper_t<int> rounded_corner_radius{"vkdecor/rounded_corner_radius"};
    wf::option_wrapper_t<wf::color_t> shadow_color{"vkdecor/shadow_color"};

  public:
    smoke_t();
    ~smoke_t();
    
    void step_effect(const wf::scene::render_instruction_t& data, wf::geometry_t rectangle,
        wf::pointf_t p, wf::color_t decor_color,
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
