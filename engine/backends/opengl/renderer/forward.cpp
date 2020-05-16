#include "forward.hpp"

using namespace benzene::opengl;

ForwardRenderer::ForwardRenderer(): main_program{} {
    main_program.add_shader(GL_VERTEX_SHADER, R"(#version 420 core
		uniform mat4 modelMatrix;
		uniform mat4 viewMatrix;
		uniform mat4 projectionMatrix;
		uniform mat3 normalMatrix;

		struct Light {
			vec3 position;
			vec3 ambient;
			vec3 diffuse;
			vec3 specular;
		};

		uniform Light light;
		uniform vec3 cameraPos;
		
		layout (location = 0) in vec3 inPosition;
		layout (location = 1) in vec3 inNormal;
		layout (location = 2) in vec3 inTangent;
		layout (location = 3) in vec2 inUv;
		
		out VS_OUT {
			vec3 fragPos;
			vec3 tangentLightPos;
			vec3 tangentCameraPos;
			vec3 tangentFragPos;
			vec2 uv;
		} vs_out;

		void main() {
		   	gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(inPosition.xyz, 1.0);
		
			vec3 T = normalize(normalMatrix * inTangent);
		   	vec3 N = normalize(normalMatrix * inNormal);
			
			T = normalize(T - dot(T, N) * N);

			vec3 B = cross(N, T);

			mat3 TBN = transpose(mat3(T, B, N));


			vs_out.uv = inUv;
			vs_out.fragPos = vec3(modelMatrix * vec4(inPosition, 1.0));
			vs_out.tangentLightPos = TBN * light.position;
			vs_out.tangentCameraPos = TBN * cameraPos;
			vs_out.tangentFragPos = TBN * vs_out.fragPos;
		})");

	main_program.add_shader(GL_FRAGMENT_SHADER, R"(#version 420 core
		struct Material {
			sampler2D diffuse;
			sampler2D specular;
			sampler2D normal;
			float shininess;
		};

		struct Light {
			vec3 position;
			vec3 ambient;
			vec3 diffuse;
			vec3 specular;
		};

		uniform Light light;
		uniform Material material;
		
		
		in VS_OUT {
			vec3 fragPos;
			vec3 tangentLightPos;
			vec3 tangentCameraPos;
			vec3 tangentFragPos;
			vec2 uv;
		} fs_in;
		
		const bool blinn = true;

		
		out vec4 fragColour;
		void main() {
		   	vec3 normal = normalize(texture(material.normal, fs_in.uv).rgb * 2.0 - 1.0);
		   	vec3 lightDir = normalize(light.position - fs_in.tangentFragPos);
		   	vec3 cameraDir = normalize(fs_in.tangentCameraPos - fs_in.tangentFragPos);
		   
		   	vec3 ambient = light.ambient * texture(material.diffuse, fs_in.uv).rgb;
		   
		   	float diffuseIntensity = max(dot(normal, lightDir), 0.0);
		   	vec3 diffuse = light.diffuse * diffuseIntensity * texture(material.diffuse, fs_in.uv).rgb;

			float specularIntensity;
			if (blinn){
				vec3 halfwayDir = normalize(lightDir + cameraDir);
				specularIntensity = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);
			} else {
				vec3 reflectDir = reflect(-lightDir, normal);
				specularIntensity = pow(max(dot(cameraDir, reflectDir), 0.0), material.shininess);
			}
			vec3 specular = light.specular * specularIntensity * texture(material.specular, fs_in.uv).rgb;
		   
		   	vec3 result = ambient + diffuse + specular;
		   	fragColour = vec4(result, 1.0);
		})");

	main_program.compile();
}

ForwardRenderer::~ForwardRenderer(){
    for(auto& [id, model] : internal_models)
		model.clean();

	main_program.clean();
}

void ForwardRenderer::draw(std::unordered_map<benzene::ModelId, benzene::Model*>& models){
    // First things first, create state of models that the backend understands
    for(auto& [id, model] : models){
		if(internal_models.count(id) == 0 || model->is_updated()){
		    internal_models[id].clean();
			internal_models[id] = Model{*model, main_program};
		}
    }

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for(const auto& [id, object] : internal_models)
        object.draw();
};