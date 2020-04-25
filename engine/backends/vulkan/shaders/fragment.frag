#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec3 cameraPosition;
} ubo;

layout (set = 1, binding = 0) uniform sampler2D texSampler;

layout (location = 0) in VS_OUT {
    vec3 worldPosition;
    vec3 colour;
    vec3 normal;
    vec2 uv;
} vertex_out;

layout (location = 0) out vec4 outColour;

const vec3 lightColour = vec3(1.0, 1.0, 1);
const vec3 lightPosition = vec3(500.0, 200.0, 300.0);//1.2, 1.0, 2.0);
const float ambientStrength = 0.1;	
const float specularStrength = 0.5;

// Options
const bool blinn = true; // Use Blinn-Phong model or bare Phong

void main(){
    vec3 normal = normalize(vertex_out.normal);
    vec3 lightDir = normalize(lightPosition - vertex_out.worldPosition);
    vec3 cameraDir = normalize(ubo.cameraPosition - vertex_out.worldPosition);

    vec3 ambient = lightColour * ambientStrength;

    float diffuseIntensity = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = lightColour * diffuseIntensity;

    float specularIntensity;
    if(blinn){
        vec3 halfwayDir = normalize(lightDir + cameraDir);
        specularIntensity = pow(max(dot(normal, halfwayDir), 0.0), 32);
    } else {
        vec3 reflectDir = reflect(-lightDir, normal);
        specularIntensity = pow(max(dot(cameraDir, reflectDir), 0.0), 32);
    }
    vec3 specular = specularStrength * specularIntensity * lightColour;

    vec3 result = (ambient + diffuse + specular) * (vertex_out.colour * texture(texSampler, vertex_out.uv).rgb);
    outColour = vec4(result, 1.0);
}