#version 450

layout(location = 0) in vec2 in_pos;
layout(location = 1) in vec2 in_uv;

layout(push_constant) uniform Params
{
    mat4 mvp;
    int  title_height;
    int  border_size;
    int  width;
    int  height;
    int  corner_radius;
    int  shadow_radius;
    vec4 shadow_color;
    vec4 border_color;
} pc;

layout(location = 0) out vec2 frag_uv;

void main()
{
    frag_uv = in_uv;
    gl_Position = pc.mvp * vec4(in_pos, 0.0, 1.0);
}
