#version 450
#extension GL_ARB_separate_shader_objects : enable
 
layout (set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec3 cameraPosition;
} ubo;

layout (push_constant) uniform PushConstants {
    mat4 model;
    mat3 normal;
} pushConstants;

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inColour;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec2 inUv;

layout (location = 0) out VS_OUT {
    vec3 worldPosition;
    vec3 colour;
    vec3 normal;
    vec2 uv;
} vertex_out;

void main(){
    vertex_out.worldPosition = vec3(pushConstants.model * vec4(inPosition, 1.0));
    vertex_out.normal = pushConstants.normal * inNormal;
    vertex_out.colour = inColour;
    vertex_out.uv = inUv;

    gl_Position = ubo.proj * ubo.view * vec4(vertex_out.worldPosition, 1.0);
}