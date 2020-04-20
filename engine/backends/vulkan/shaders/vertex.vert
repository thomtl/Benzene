#version 450
#extension GL_ARB_separate_shader_objects : enable
 
layout (set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;

layout (push_constant) uniform PushConstants {
    mat4 model;
} pushConstants;

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inColour;
layout (location = 2) in vec2 inUv;

layout (location = 0) out VS_OUT {
    vec3 colour;
    vec2 uv;
} vertex_out;

void main(){
    gl_Position = ubo.proj * ubo.view * pushConstants.model * vec4(inPosition, 1.0);
    vertex_out.colour = inColour;
    vertex_out.uv = inUv;
}