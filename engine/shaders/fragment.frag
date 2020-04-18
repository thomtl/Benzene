#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 1, binding = 0) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec3 fragColour;
layout(location = 1) in vec2 fragUv;


void main(){
    vec3 textureColour = texture(texSampler, fragUv).rgb;
    outColor = vec4(fragColour * textureColour, 1.0);
}