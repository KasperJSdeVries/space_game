#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec4 in_colour;

void main() {
    gl_Position = vec4(in_position, 1.0);
}
