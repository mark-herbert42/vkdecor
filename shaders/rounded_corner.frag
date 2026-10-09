#version 450
#extension GL_ARB_shading_language_include : require

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

layout(location = 0) in vec2 frag_uv;
layout(location = 0) out vec4 out_color;

#include "color-transform.frag"

void main()
{
    vec4 out_c;
    // Coords within window rectancle (uv 0..1 )
    ivec2 pos = ivec2(frag_uv * vec2(float(pc.width), float(pc.height)));

    // Clear internal window area
    if (pos.x >= pc.border_size && pos.x <= (pc.width - 1) - pc.border_size &&
        pos.y >= pc.title_height && pos.y <= (pc.height - 1) - pc.border_size)
    {
   //     out_c = pc.border_color;
   //out_color = vec4(transform_color(pc.border_color.rgb), pc.border_color.a);
        out_color = vec4(0.0);
        return;
    }

    float d;
    vec4 e = pc.shadow_color;
    vec4 c = pc.border_color;
    vec4 m = vec4(0.0);
    vec4 s;
    float diffuse = 1.0 / float(pc.shadow_radius == 0 ? 1 : pc.shadow_radius);

    int left_edge_max   = pc.shadow_radius * 2;
    int right_edge_min  = (pc.width - 1) - (pc.shadow_radius * 2);
    int top_edge_max    = pc.shadow_radius * 2;
    int bottom_edge_min = (pc.height - 1) - (pc.shadow_radius * 2);

    int corner_left   = pc.shadow_radius * 2 + pc.corner_radius;
    int corner_right  = (pc.width - 1) - (pc.shadow_radius * 2 + pc.corner_radius);
    int corner_top    = pc.shadow_radius * 2 + pc.corner_radius;
    int corner_bottom = (pc.height - 1) - (pc.shadow_radius * 2 + pc.corner_radius);

    // --- Corners ---
    if (pos.x < corner_left && pos.y < corner_top)
    {
        vec2 center = vec2(float(corner_left), float(corner_top));
        d = distance(center, vec2(pos)) - float(pc.corner_radius);
        s = mix(e, m, 1.0 - exp(-pow(d * diffuse, 2.0)));
        d = distance(center, vec2(pos));
        out_c = mix(c, s, clamp(d - float(pc.corner_radius), 0.0, 1.0));
    }
    else if (pos.x < corner_left && pos.y > corner_bottom)
    {
        vec2 center = vec2(float(corner_left), float(corner_bottom));
        d = distance(center, vec2(pos)) - float(pc.corner_radius);
        s = mix(e, m, 1.0 - exp(-pow(d * diffuse, 2.0)));
        d = distance(center, vec2(pos));
        out_c = mix(c, s, clamp(d - float(pc.corner_radius), 0.0, 1.0));
    }
    else if (pos.x > corner_right && pos.y < corner_top)
    {
        vec2 center = vec2(float(corner_right), float(corner_top));
        d = distance(center, vec2(pos)) - float(pc.corner_radius);
        s = mix(e, m, 1.0 - exp(-pow(d * diffuse, 2.0)));
        d = distance(center, vec2(pos));
        out_c = mix(c, s, clamp(d - float(pc.corner_radius), 0.0, 1.0));
    }
    else if (pos.x > corner_right && pos.y > corner_bottom)
    {
        vec2 center = vec2(float(corner_right), float(corner_bottom));
        d = distance(center, vec2(pos)) - float(pc.corner_radius);
        s = mix(e, m, 1.0 - exp(-pow(d * diffuse, 2.0)));
        d = distance(center, vec2(pos));
        out_c = mix(c, s, clamp(d - float(pc.corner_radius), 0.0, 1.0));
    }
    // --- Edges ---
    else if (pos.x < left_edge_max && pos.y >= corner_top && pos.y <= corner_bottom)
    {
        d = distance(vec2(float(left_edge_max), float(pos.y)), vec2(pos));
        out_c = mix(e, m, 1.0 - exp(-pow(d * diffuse, 2.0)));
    }
    else if (pos.x > right_edge_min && pos.y >= corner_top && pos.y <= corner_bottom)
    {
        d = distance(vec2(float(right_edge_min), float(pos.y)), vec2(pos));
        out_c = mix(e, m, 1.0 - exp(-pow(d * diffuse, 2.0)));
    }
    else if (pos.y < top_edge_max && pos.x >= corner_left && pos.x <= corner_right)
    {
        d = distance(vec2(float(pos.x), float(top_edge_max)), vec2(pos));
        out_c = mix(e, m, 1.0 - exp(-pow(d * diffuse, 2.0)));
    }
    else if (pos.y > bottom_edge_min && pos.x >= corner_left && pos.x <= corner_right)
    {
        d = distance(vec2(float(pos.x), float(bottom_edge_min)), vec2(pos));
        out_c = mix(e, m, 1.0 - exp(-pow(d * diffuse, 2.0)));
    }
    else
    {
        // Decoration area between shadow and window area -  decor_color
        out_c = pc.border_color;
    }

out_color = vec4(transform_color(out_c.rgb), out_c.a);
}
