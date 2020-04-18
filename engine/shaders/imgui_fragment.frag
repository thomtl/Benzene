#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (set = 0, binding = 0) uniform sampler2D fontSampler;

layout (location = 0) in VS_OUT {
    vec4 colour;
    vec2 uv;
} vertex_out;

layout (location = 0) out vec4 outColor;

void main(){
    outColor = vertex_out.colour * texture(fontSampler, vertex_out.uv);
}