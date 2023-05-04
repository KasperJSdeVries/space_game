#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_colour;

layout(location = 0) out vec3 out_colour;

layout(set = 0, binding = 0) uniform global_uniform_object {
    mat4 projection;
    mat4 view;
} global_ubo;

void main() {
    gl_Position = global_ubo.projection * global_ubo.view * vec4(in_position, 1.0);
    out_colour = in_colour;
}
