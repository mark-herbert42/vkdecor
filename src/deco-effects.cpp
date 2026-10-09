/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2024 Scott Moreau <oreaus@gmail.com>
 * - Ported weston-smoke to compute shader set
 * Copyright (c) 2024 Ilia Bozhinov <ammen99@gmail.com>
 * - Awesome optimizations
 * Copyright (c) 2024 Andrew Pliatsikas <futurebytestore@gmail.com>
 * - Ported effect shaders to compute
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

#include <wayfire/debug.hpp>
#include <wayfire/render.hpp>

#include "deco-effects.hpp"


namespace wf
{
	
	
namespace vkdecor
{



static const char *rounded_corner_overlay =
    R"(
#version 320 es

layout(binding = 0, rgba32f) writeonly uniform highp image2D out_tex;

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout(location = 1) uniform int title_height;
layout(location = 2) uniform int border_size;
layout(location = 5) uniform int width;
layout(location = 6) uniform int height;
layout(location = 7) uniform int corner_radius;
layout(location = 8) uniform int shadow_radius;
layout(location = 9) uniform vec4 shadow_color;
layout(location = 10) uniform vec4 border_color;

void main() {
    ivec2 pos = ivec2(gl_GlobalInvocationID.xy);

    // Guard against processing out-of-bounds invocations
    if (pos.x >= width || pos.y >= height) {
        return;
    }

    // Skip computing pixels residing inside the core window area
    if (pos.x >= border_size && pos.x <= (width - 1) - border_size && 
        pos.y >= title_height && pos.y <= (height - 1) - border_size)
    {
        return;
    }

    float d;
    vec4 e = shadow_color;
    vec4 c = border_color;
    vec4 m = vec4(0.0);
    vec4 s;
    float diffuse = 1.0 / float(shadow_radius == 0 ? 1 : shadow_radius);

    // Compute semantic boundary zones to clean up code repetition
    int left_edge_max   = shadow_radius * 2;
    int right_edge_min  = (width - 1) - (shadow_radius * 2);
    int top_edge_max    = shadow_radius * 2;
    int bottom_edge_min = (height - 1) - (shadow_radius * 2);

    int corner_left   = shadow_radius * 2 + corner_radius;
    int corner_right  = (width - 1) - (shadow_radius * 2 + corner_radius);
    int corner_top    = shadow_radius * 2 + corner_radius;
    int corner_bottom = (height - 1) - (shadow_radius * 2 + corner_radius);

    // --- CORNER ZONES (Evaluated first to claim overlapping areas) ---

    // Top-left corner
    if (pos.x < corner_left && pos.y < corner_top)
    {
        vec2 center = vec2(float(corner_left), float(corner_top));
        d = distance(center, vec2(pos)) - float(corner_radius);
        s = mix(e, m, 1.0 - exp(-pow(d * diffuse, 2.0)));
        d = distance(center, vec2(pos));
        imageStore(out_tex, pos, mix(c, s, clamp(d - float(corner_radius), 0.0, 1.0)));
    }
    // Bottom-left corner
    else if (pos.x < corner_left && pos.y > corner_bottom)
    {
        vec2 center = vec2(float(corner_left), float(corner_bottom));
        d = distance(center, vec2(pos)) - float(corner_radius);
        s = mix(e, m, 1.0 - exp(-pow(d * diffuse, 2.0)));
        d = distance(center, vec2(pos));
        imageStore(out_tex, pos, mix(c, s, clamp(d - float(corner_radius), 0.0, 1.0)));
    }
    // Top-right corner
    else if (pos.x > corner_right && pos.y < corner_top)
    {
        vec2 center = vec2(float(corner_right), float(corner_top));
        d = distance(center, vec2(pos)) - float(corner_radius);
        s = mix(e, m, 1.0 - exp(-pow(d * diffuse, 2.0)));
        d = distance(center, vec2(pos));
        imageStore(out_tex, pos, mix(c, s, clamp(d - float(corner_radius), 0.0, 1.0)));
    }
    // Bottom-right corner
    else if (pos.x > corner_right && pos.y > corner_bottom)
    {
        vec2 center = vec2(float(corner_right), float(corner_bottom));
        d = distance(center, vec2(pos)) - float(corner_radius);
        s = mix(e, m, 1.0 - exp(-pow(d * diffuse, 2.0)));
        d = distance(center, vec2(pos));
        imageStore(out_tex, pos, mix(c, s, clamp(d - float(corner_radius), 0.0, 1.0)));
    }

    // --- STRAIGHT EDGES ---

    // Left edge
    else if (pos.x < left_edge_max && pos.y >= corner_top && pos.y <= corner_bottom)
    {
        d = distance(vec2(float(left_edge_max), float(pos.y)), vec2(pos));
        imageStore(out_tex, pos, mix(e, m, 1.0 - exp(-pow(d * diffuse, 2.0))));
    }
    // Right edge
    else if (pos.x > right_edge_min && pos.y >= corner_top && pos.y <= corner_bottom)
    {
        d = distance(vec2(float(right_edge_min), float(pos.y)), vec2(pos));
        imageStore(out_tex, pos, mix(e, m, 1.0 - exp(-pow(d * diffuse, 2.0))));
    }
    // Top edge
    else if (pos.y < top_edge_max && pos.x >= corner_left && pos.x <= corner_right)
    {
        d = distance(vec2(float(pos.x), float(top_edge_max)), vec2(pos));
        imageStore(out_tex, pos, mix(e, m, 1.0 - exp(-pow(d * diffuse, 2.0))));
    }
    // Bottom edge
    else if (pos.y > bottom_edge_min && pos.x >= corner_left && pos.x <= corner_right)
    {
        d = distance(vec2(float(pos.x), float(bottom_edge_min)), vec2(pos));
        imageStore(out_tex, pos, mix(e, m, 1.0 - exp(-pow(d * diffuse, 2.0))));
    }
}

)";

#if WF_HAS_VULKANFX
    class vulkan_state_t : public wf::custom_data_t
    {
      public:
        std::shared_ptr<wf::vk::graphics_pipeline_t> pipeline;
    };



vulkan_state_t& ensure_vk(wf::vulkan_render_state_t& state)
{

    if (auto data = state.get_data<vulkan_state_t>())
    {
        return *data;
    }


auto vs = state.get_context()->load_shader_module(
    rounded_corner_vert_data, sizeof(rounded_corner_vert_data));
auto fs = state.get_context()->load_shader_module(
    rounded_corner_frag_data, sizeof(rounded_corner_frag_data));


    wf::vk::pipeline_params_t params{};

wf::vk::pipeline_shader_t vs_shader{};
vs_shader.stage  = VK_SHADER_STAGE_VERTEX_BIT;
vs_shader.shader = vs;
params.shaders.push_back(vs_shader);

wf::vk::pipeline_shader_t fs_shader{};
fs_shader.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
fs_shader.shader = fs;
params.shaders.push_back(fs_shader);


        params.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        params.vertex_input_description = {{
            .binding   = 0,
            .stride    = sizeof(float) * 4,   // pos.xy + uv.xy
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        }};
        params.vertex_attribute_description = {
            {.location = 0, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = 0},
            {.location = 1, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = sizeof(float) * 2},
        };

        // Use fragment shader instead of texture

        params.descriptor_set_layouts = {};
        params.push_constants = {{
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            .offset     = 0,
            .size       = sizeof(push_constants_t),
        }};

        auto data = std::make_unique<vulkan_state_t>();
        data->pipeline = std::make_shared<wf::vk::graphics_pipeline_t>(state.get_context(), params);
        auto ptr = data.get();
        state.store_data<vulkan_state_t>(std::move(data));
        return *ptr;
}

std::shared_ptr<wf::vk::gpu_buffer_t> smoke_t::find_buffer(
    std::shared_ptr<wf::vk::context_t> ctx, VkDeviceSize total_size)
{
    auto& buffer = vulkan_vertex_buffer;

    // Buffer reuse -  like wobbly.cpp
    if (buffer && (buffer->get_size() >= total_size) && (buffer.use_count() == 1))
    {
        return buffer;
    }

    buffer = ctx->create_buffer(total_size,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    return buffer;
}
#endif

void setup_shader(GLuint *program, std::string source)
{
    auto compute_shader  = OpenGL::compile_shader(source.c_str(), GL_COMPUTE_SHADER);
    auto compute_program = GL_CALL(glCreateProgram());
    GL_CALL(glAttachShader(compute_program, compute_shader));
    GL_CALL(glLinkProgram(compute_program));

    int s = GL_FALSE;
#define LENGTH 1024 * 128
    char log[LENGTH];
    GL_CALL(glGetProgramiv(compute_program, GL_LINK_STATUS, &s));
    GL_CALL(glGetProgramInfoLog(compute_program, LENGTH, NULL, log));

    if (s == GL_FALSE)
    {
        LOGE("Failed to link shader:\n", source,
            "\nLinker output:\n", log);
    }

    GL_CALL(glDeleteShader(compute_shader));
    *program = compute_program;
}

void smoke_t::destroy_programs()
{

    if (render_overlay_program != GLuint(-1))
    {
        GL_CALL(glDeleteProgram(render_overlay_program));
    }
                render_overlay_program = GLuint(-1);
}

void smoke_t::create_programs()
{
    destroy_programs();
    wf::gles::run_in_context_if_gles([&]
    {
            setup_shader(&render_overlay_program, rounded_corner_overlay);		

    });
}

smoke_t::smoke_t()
{
    render_overlay_program = GLuint(-1);

    texture = GLuint(-1);

    create_programs();
}

smoke_t::~smoke_t()
{
    destroy_programs();
    destroy_textures();
}

void smoke_t::create_textures()
{
    GL_CALL(glGenTextures(1, &texture));
}

void smoke_t::destroy_textures()
{
    if (texture != GLuint(-1))
    {
        GL_CALL(glDeleteTextures(1, &texture));
        texture = GLuint(-1);
    }

}

int round_up_div(int a, int b)
{
    return (a + b - 1) / b;
}

void smoke_t::recreate_textures(wf::geometry_t rectangle)
{
    if ((rectangle.width <= 0) || (rectangle.height <= 0))
    {
        return;
    }

    destroy_textures();
    create_textures();

    GL_CALL(glActiveTexture(GL_TEXTURE0 + 0));
    GL_CALL(glBindTexture(GL_TEXTURE_2D, texture));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
    GL_CALL(glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, rectangle.width, rectangle.height));

}

void smoke_t::step_effect(const wf::scene::render_instruction_t& data, wf::geometry_t rectangle,
    wf::pointf_t p, wf::color_t decor_color,
    int title_height, int border_size, int shadow_radius)
{

    if ((rectangle.width <= 0) || (rectangle.height <= 0))
    {
        return;
    }
            if ((rectangle.width == saved_width) && (rectangle.height == saved_height) && (decor_color == saved_color) )
    {
			return;
	}	
    
            saved_width  = rectangle.width;
            saved_height = rectangle.height;
            saved_color = decor_color;
            
    int radius = shadow_radius;
        const wf::geometry_t nonshadow_rect = wf::geometry_t{
            radius* 2,
            radius * 2,
            rectangle.width - 4 * radius,
            rectangle.height - 4 * radius
        };

        wf::geometry_t inner_part = {
            border_size + radius * 2,
            title_height + border_size + radius * 2,
            rectangle.width - border_size * 2 - radius * 4,
            rectangle.height - border_size * 2 - title_height - radius * 4,
        };

        wf::region_t border_region = wf::to_integer_box(nonshadow_rect);
        border_region ^= wf::to_integer_box(inner_part);
        border_region.expand_edges(1);
        border_region &= wf::to_integer_box(nonshadow_rect);      
 
#if WF_HAS_VULKANFX
    push_constants.shadow_color = glm::vec4(
        wf::color_t(shadow_color).r,
        wf::color_t(shadow_color).g,
        wf::color_t(shadow_color).b,
        wf::color_t(shadow_color).a);
    push_constants.border_color = glm::vec4(
        wf::color_t(decor_color).r,
        wf::color_t(decor_color).g,
        wf::color_t(decor_color).b,
        wf::color_t(decor_color).a);
    push_constants.title_height  = title_height + border_size + radius * 2;
    push_constants.border_size   = border_size + radius * 2;
    push_constants.width         = rectangle.width;
    push_constants.height        = rectangle.height;
    push_constants.corner_radius = rounded_corner_radius;
    push_constants.shadow_radius = radius;
#endif 
    wf::gles::run_in_context_if_gles([&]
    {
        wf::gles::bind_render_buffer(data.target);
   
            recreate_textures(rectangle);
            
        GL_CALL(glActiveTexture(GL_TEXTURE0 + 0));
        GL_CALL(glBindTexture(GL_TEXTURE_2D, texture));
        GL_CALL(glBindImageTexture(0, texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F));

		if (std::string(overlay_engine) != "none")
        {
            GLuint fb;
            GL_CALL(glGenFramebuffers(1, &fb));
            GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, fb));
            GL_CALL(glActiveTexture(GL_TEXTURE0 + 0));
            GL_CALL(glBindTexture(GL_TEXTURE_2D, texture));
            GL_CALL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                GL_TEXTURE_2D, texture, 0));
            OpenGL::clear(decor_color, GL_COLOR_BUFFER_BIT);
            GL_CALL(glDeleteFramebuffers(1, &fb));
            GL_CALL(glUseProgram(render_overlay_program));
            GL_CALL(glUniform1i(1, title_height + border_size + radius * 2));
            GL_CALL(glUniform1i(2, border_size + radius * 2));
            GL_CALL(glUniform1i(5, rectangle.width));
            GL_CALL(glUniform1i(6, rectangle.height));
            GL_CALL(glUniform1i(7, rounded_corner_radius));
            GLfloat shadow_color_f[4] =
                {GLfloat(wf::color_t(shadow_color).r), GLfloat(wf::color_t(shadow_color).g),
                    GLfloat(wf::color_t(shadow_color).b), GLfloat(wf::color_t(shadow_color).a)};
                GL_CALL(glUniform1i(8, radius));
                GL_CALL(glUniform4fv(9, 1, shadow_color_f));
            GLfloat border_color_f[4] =
                {GLfloat(wf::color_t(decor_color).r), GLfloat(wf::color_t(decor_color).g),
                    GLfloat(wf::color_t(decor_color).b), GLfloat(wf::color_t(decor_color).a)};
                GL_CALL(glUniform1i(8, radius));
                GL_CALL(glUniform4fv(9, 1, shadow_color_f));
                GL_CALL(glUniform4fv(10, 1, border_color_f));               
            GL_CALL(glDispatchCompute(round_up_div(rectangle.width, 16), round_up_div(rectangle.height, 16),
                1));
            GL_CALL(glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT));
        }

        GL_CALL(glUseProgram(0));
    });
}

void smoke_t::render_effect(const wf::scene::render_instruction_t& data, wf::geometry_t rectangle)
{
    if (wf::get_core().is_gles2())
	{	
    LOGE("VKDECOR render effect");

    OpenGL::render_transformed_texture(wf::gles_texture_t{texture}, rectangle,
        wf::gles::render_target_orthographic_projection(data.target), glm::vec4{1},
        OpenGL::TEXTURE_TRANSFORM_INVERT_Y | OpenGL::RENDER_FLAG_CACHED);
    data.pass->custom_gles_subpass(data.target, [&]
    {
        for (auto& box : data.damage)
        {
            wf::gles::render_target_logic_scissor(data.target, box);
            OpenGL::draw_cached();
        }
    });

    OpenGL::clear_cached();
	}
else {

#if WF_HAS_VULKANFX


        data.pass->custom_vulkan_subpass([&] (wf::vulkan_render_state_t& state,
                                              wf::vk::command_buffer_t& cmd_buf)
        {
            auto& our_state = ensure_vk(state);

            // Repeat the logic of wayfire core (render_pass_t::add_texture): geometry
            // is converted  to  dst_box in framebuffer pixels, which do
            // correctly process scale, wl_transform and subbuffers (expo).
            // MVP — just tramslating pixels into  NDC Vulkan
            // (Y inverted, equivalent to  non-aligned render_target_transform
            // for target withou  wl-transformation).
            const auto dst_box =
                data.target.framebuffer_texture_dst_box_from_geometry_box(rectangle);

            float x0 = (float)dst_box.x;
            float y0 = (float)dst_box.y;
            float x1 = (float)(dst_box.x + dst_box.width);
            float y1 = (float)(dst_box.y + dst_box.height);

            const auto fb_size = data.target.get_size();
            const float fb_w = (float)fb_size.width;
            const float fb_h = (float)fb_size.height;

            // Pixels -> NDC. Y is directed UPWARDS (GL-like convention of wlr-pass,
            // wlroots use inverted vieport), so pixel 
            // line  0 (top of the screen) -> NDC -1.  Usinhg pore
            // Vulkan-convention Y-downwards made vertical mirroring,
            // not affecting full target coverage , but breaking sub-areas.
            glm::mat4 mvp = glm::mat4(1.0);
            mvp[0][0] = 2.0f / fb_w;
            mvp[1][1] = 2.0f / fb_h;
            mvp[3][0] = -1.0f;
            mvp[3][1] = -1.0f;

            push_constants.mvp = mvp;

            std::vector<float> unified_buffer = {
                x0, y0, 0.0f, 0.0f,
                x1, y0, 1.0f, 0.0f,
                x0, y1, 0.0f, 1.0f,

                x1, y0, 1.0f, 0.0f,
                x1, y1, 1.0f, 1.0f,
                x0, y1, 0.0f, 1.0f,
            };

            VkDeviceSize total_size = unified_buffer.size() * sizeof(float);
            auto buffer = find_buffer(state.get_context(), total_size);
            buffer->write(unified_buffer.data(), total_size);

            auto [layout, _] = cmd_buf.bind_pipeline(our_state.pipeline, data.target,
                wf::vk::pipeline_specialization_t{});
            cmd_buf.set_full_viewport(data.target);

            vkCmdPushConstants(cmd_buf, layout,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0, sizeof(push_constants_t), &push_constants);

            cmd_buf.bind_buffer(buffer);
            VkDeviceSize offset = 0;
            vkCmdBindVertexBuffers(cmd_buf, 0, 1, &buffer->get_buffer(), &offset);

            cmd_buf.for_each_scissor_rect(data.target, (data.damage & data.target.geometry), [&]
            {
                vkCmdDraw(cmd_buf, 6, 1, 0, 0);
            });
        });
#endif

}
}

void smoke_t::effect_updated()
{
    create_programs();
    wf::gles::run_in_context_if_gles([&]
    {
        recreate_textures(wf::geometry_t{0, 0, saved_width, saved_height});
    });
}
} // namespace vkdecor
}
