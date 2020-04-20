#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (set = 1, binding = 0) uniform sampler2D texSampler;

layout (location = 0) in VS_OUT {
    vec3 colour;
    vec2 uv;
} vertex_out;

layout (location = 0) out vec4 outColor;

void main(){
    vec3 textureColour = texture(texSampler, vertex_out.uv).rgb;
    outColor = vec4(vertex_out.colour * textureColour, 1.0);
}