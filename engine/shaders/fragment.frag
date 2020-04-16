#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec3 fragColour;
layout(location = 1) in vec2 fragTexCoord;

void main(){
    vec3 textureColour = texture(texSampler, fragTexCoord).rgb;
    outColor = vec4(fragColour * textureColour, 1.0);
}