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

layout(binding = 0, rgba32f) readonly uniform highp image2D in_tex;  // Use binding point 0
layout(binding = 0, rgba32f) writeonly uniform highp image2D out_tex;  // Use binding point 0

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout(location = 1) uniform int title_height;
layout(location = 2) uniform int border_size;
layout(location = 5) uniform int width;
layout(location = 6) uniform int height;
layout(location = 7) uniform int corner_radius;
layout(location = 8) uniform int shadow_radius;
layout(location = 9) uniform vec4 shadow_color;

void main() {
    ivec2 pos = ivec2(gl_GlobalInvocationID.xy);

    // Check if the pixel should be drawn
    if (pos.x >= border_size && pos.x <= (width - 1) - border_size && pos.y >= title_height && pos.y <= (height - 1) - border_size)
    {
        return;
    }
    float d;
    vec4 c = shadow_color;
    vec4 m = vec4(0.0);
    vec4 s;
    float diffuse = 1.0 / float(shadow_radius == 0 ? 1 : shadow_radius);
    // left
    if (pos.x < shadow_radius * 2 && pos.y >= shadow_radius * 2 && pos.y <= height - shadow_radius * 2)
    {
        d = distance(vec2(float(shadow_radius * 2), float(pos.y)), vec2(pos));
        imageStore(out_tex, pos, mix(c, m, 1.0 - exp(-pow(d * diffuse, 2.0))));
    }
    // top left corner
    if (pos.x < shadow_radius * 2 + corner_radius && pos.y < shadow_radius * 2 + corner_radius)
    {
        d = distance(vec2(float(shadow_radius * 2 + corner_radius)), vec2(pos)) - float(corner_radius);
        s = mix(c, m, 1.0 - exp(-pow(d * diffuse, 2.0)));
        c = imageLoad(in_tex, pos);
        d = distance(vec2(float(shadow_radius * 2 + corner_radius)), vec2(pos));
        imageStore(out_tex, pos, mix(c, s, clamp(d - float(corner_radius), 0.0, 1.0)));
    }
    // bottom left corner
    if (pos.x < (shadow_radius * 2 + corner_radius) && pos.y > height - (shadow_radius * 2 + corner_radius))
    {
        d = distance(vec2(float((shadow_radius * 2 + corner_radius)), float((height - 1) - (shadow_radius * 2 + corner_radius))), vec2(pos)) - float(corner_radius);
        s = mix(c, m, 1.0 - exp(-pow(d * diffuse, 2.0)));
        c = imageLoad(in_tex, pos);
        d = distance(vec2(float((shadow_radius * 2 + corner_radius)), float((height - 1) - (shadow_radius * 2 + corner_radius))), vec2(pos));
        imageStore(out_tex, pos, mix(c, s, clamp(d - float(corner_radius), 0.0, 1.0)));
    }
    // top
    if (pos.x >= shadow_radius * 2 + corner_radius && pos.x <= width - shadow_radius * 2 + corner_radius && pos.y < shadow_radius * 2)
    {
        d = distance(vec2(float(pos.x), float(shadow_radius * 2)), vec2(pos));
        imageStore(out_tex, pos, mix(c, m, 1.0 - exp(-pow(d * diffuse, 2.0))));
    }
    // right
    if (pos.x > (width - 1) - shadow_radius * 2 && pos.y >= shadow_radius * 2 && pos.y <= (height - 1) - shadow_radius * 2)
    {
        d = distance(vec2(float((width - 1) - shadow_radius * 2), float(pos.y)), vec2(pos));
        imageStore(out_tex, pos, mix(c, m, 1.0 - exp(-pow(d * diffuse, 2.0))));
    }
    // top right corner
    if (pos.x > width - (shadow_radius * 2 + corner_radius) && pos.y < (shadow_radius * 2 + corner_radius))
    {
        d = distance(vec2(float((width - 1) - (shadow_radius * 2 + corner_radius)), float((shadow_radius * 2 + corner_radius))), vec2(pos)) - float(corner_radius);
        s = mix(c, m, 1.0 - exp(-pow(d * diffuse, 2.0)));
        c = imageLoad(in_tex, pos);
        d = distance(vec2(float((width - 1) - (shadow_radius * 2 + corner_radius)), float((shadow_radius * 2 + corner_radius))), vec2(pos));
        imageStore(out_tex, pos, mix(c, s, clamp(d - float(corner_radius), 0.0, 1.0)));
    }
    // bottom right corner
    if (pos.x > (width - 1) - (shadow_radius * 2 + corner_radius) && pos.y > (height - 1) - (shadow_radius * 2 + corner_radius))
    {
        d = distance(vec2(float((width - 1) - (shadow_radius * 2 + corner_radius)), float((height - 1) - (shadow_radius * 2 + corner_radius))), vec2(pos)) - float(corner_radius);
        s = mix(c, m, 1.0 - exp(-pow(d * diffuse, 2.0)));
        c = imageLoad(in_tex, pos);
        d = distance(vec2(float((width - 1) - (shadow_radius * 2 + corner_radius)), float((height - 1) - (shadow_radius * 2 + corner_radius))), vec2(pos));
        imageStore(out_tex, pos, mix(c, s, clamp(d - float(corner_radius), 0.0, 1.0)));
    }
    // bottom
    if (pos.x >= (shadow_radius * 2 + corner_radius) && pos.x <= (width - 1) - (shadow_radius * 2 + corner_radius) && pos.y > (height - 1) - shadow_radius * 2)
    {
        d = distance(vec2(float(pos.x), float((height - 1) - shadow_radius * 2)), vec2(pos));
        imageStore(out_tex, pos, mix(c, m, 1.0 - exp(-pow(d * diffuse, 2.0))));
    }
}

)";



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

static void seed_random()
{
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    srandom(ts.tv_nsec);
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
        if (std::string(overlay_engine) == "rounded_corners")
            setup_shader(&render_overlay_program, rounded_corner_overlay);		

    });
}

smoke_t::smoke_t()
{
    render_overlay_program = GLuint(-1);

    texture = GLuint(-1);

    create_programs();
    seed_random();
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

void smoke_t::dispatch_region(const wf::region_t& region)
{
    std::vector<int> values;

    int max_x = 0;
    int max_y = 0;

    for (auto& box : region)
    {
        auto rect = wlr_box_from_pixman_box(box);
        values.push_back(rect.x);
        values.push_back(rect.y);
        values.push_back(rect.width);
        values.push_back(rect.height);

        if (rect.width > rect.height)
        {
            values.push_back(0); // xy will not be flipped in compute shader
            max_x = std::max(max_x, rect.width);
            max_y = std::max(max_y, rect.height);
        } else
        {
            values.push_back(1); // xy will be flipped in compute shader
            max_x = std::max(max_x, rect.height);
            max_y = std::max(max_y, rect.width);
        }
    }

    if (values.size() / 5 > 4)
    {
        LOGE("Error: too many regions");
        return;
    }

    GL_CALL(glUniform1iv(10, values.size(), values.data()));
    GL_CALL(glDispatchCompute(round_up_div(max_x, 16), round_up_div(max_y, 16), values.size() / 5));
    GL_CALL(glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT));
}

void smoke_t::run_shader_region(GLuint program, const wf::region_t & region, const wf::dimensions_t & size)
{
    GL_CALL(glUseProgram(program));
    GL_CALL(glUniform1i(5, size.width));
    GL_CALL(glUniform1i(6, size.height));
    dispatch_region(region);
}

void smoke_t::run_shader(GLuint program, int width, int height, int title_height, int border_size, int radius)
{
    GL_CALL(glUseProgram(program));
    GL_CALL(glUniform1i(1, title_height + border_size + radius * 2));
    GL_CALL(glUniform1i(2, border_size + radius * 2));
    GL_CALL(glUniform1i(5, width));
    GL_CALL(glUniform1i(6, height));
    GL_CALL(glUniform1i(9, radius * 2));
    GL_CALL(glDispatchCompute(width / 15, height / 15, 1));
    GL_CALL(glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT));
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
    bool ink, wf::pointf_t p, wf::color_t decor_color,
    int title_height, int border_size, int shadow_radius)
{

    if ((rectangle.width <= 0) || (rectangle.height <= 0))
    {
        return;
    }

    int radius = shadow_radius;

    wf::gles::run_in_context_if_gles([&]
    {
        wf::gles::bind_render_buffer(data.target);
        if ((rectangle.width != saved_width) || (rectangle.height != saved_height))
        {
            saved_width  = rectangle.width;
            saved_height = rectangle.height;

            recreate_textures(rectangle);
        }

        GL_CALL(glActiveTexture(GL_TEXTURE0 + 0));
        GL_CALL(glBindTexture(GL_TEXTURE_2D, texture));
        GL_CALL(glBindImageTexture(0, texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F));

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

        wf::region_t border_region = nonshadow_rect;
        border_region ^= inner_part;
        border_region.expand_edges(1);
        border_region &= nonshadow_rect;

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
        }

        if (std::string(overlay_engine) == "rounded_corners") 
        {
            GL_CALL(glUseProgram(render_overlay_program));
            GL_CALL(glUniform1i(1, title_height + border_size + radius * 2));
            GL_CALL(glUniform1i(2, border_size + radius * 2));
            GL_CALL(glUniform1i(5, rectangle.width));
            GL_CALL(glUniform1i(6, rectangle.height));
            GL_CALL(glUniform1i(7, rounded_corner_radius));
            if (std::string(overlay_engine) == "rounded_corners")
            {
                GLfloat shadow_color_f[4] =
                {GLfloat(wf::color_t(shadow_color).r), GLfloat(wf::color_t(shadow_color).g),
                    GLfloat(wf::color_t(shadow_color).b), GLfloat(wf::color_t(shadow_color).a)};
                GL_CALL(glUniform1i(8, radius));
                GL_CALL(glUniform4fv(9, 1, shadow_color_f));
            }

            GL_CALL(glDispatchCompute(round_up_div(rectangle.width, 16), round_up_div(rectangle.height, 16),
                1));
            GL_CALL(glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT));
        }

        GL_CALL(glUseProgram(0));
    });
}

void smoke_t::render_effect(const wf::scene::render_instruction_t& data, wf::geometry_t rectangle)
{
    OpenGL::render_transformed_texture(wf::gles_texture_t{texture}, rectangle,
        wf::gles::render_target_orthographic_projection(data.target), glm::vec4{1},
        OpenGL::TEXTURE_TRANSFORM_INVERT_Y | OpenGL::RENDER_FLAG_CACHED);

    data.pass->custom_gles_subpass(data.target, [&]
    {
        for (auto& box : data.damage)
        {
            wf::gles::render_target_logic_scissor(data.target, wlr_box_from_pixman_box(box));
            OpenGL::draw_cached();
        }
    });

    OpenGL::clear_cached();
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
