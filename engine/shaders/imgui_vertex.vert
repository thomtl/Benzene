#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec2 inPos;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec4 inColour;

layout (push_constant) uniform PushConstants {
    vec2 scale;
    vec2 translate;
} pushConstants;

layout (location = 0) out VS_OUT {
    vec4 colour;
    vec2 uv;
} vertex_out;

void main(){
    vertex_out.colour = inColour;
    vertex_out.uv = inUV;
    gl_Position = vec4(inPos * pushConstants.scale + pushConstants.translate, 0.0, 1.0);
}