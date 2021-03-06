#include "forward.hpp"

using namespace benzene::opengl;

ForwardRenderer::ForwardRenderer(int width, int height): main_program{} {
    main_program.add_shader(GL_VERTEX_SHADER, R"(#version 420 core
		#extension GL_ARB_shader_storage_buffer_object : require

		uniform mat4 viewMatrix;
		uniform mat4 projectionMatrix;

		struct Light {
			vec3 position;
			vec3 ambient;
			vec3 diffuse;
			vec3 specular;
		};

		uniform Light light;
		uniform vec3 cameraPos;

		struct InstanceData {
			mat4 modelMatrix;
			mat4 normalMatrix;
		};

		layout (std140, binding = 0) buffer PerInstanceData {
			InstanceData data[];
		} instanceData;
		
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
		   	gl_Position = projectionMatrix * viewMatrix * instanceData.data[gl_InstanceID].modelMatrix * vec4(inPosition.xyz, 1.0);
		
			vec3 T = normalize(mat3(instanceData.data[gl_InstanceID].normalMatrix) * inTangent);
		   	vec3 N = normalize(mat3(instanceData.data[gl_InstanceID].normalMatrix) * inNormal);
			
			T = normalize(T - dot(T, N) * N);

			vec3 B = cross(N, T);

			mat3 TBN = transpose(mat3(T, B, N));


			vs_out.uv = inUv;
			vs_out.fragPos = vec3(instanceData.data[gl_InstanceID].modelMatrix * vec4(inPosition, 1.0));
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

	main_program.set_uniform("light.position", glm::vec3{-300.0f, 200.0f, 0.0f});
	main_program.set_uniform("light.ambient", glm::vec3{0.2f, 0.2f, 0.2f});
	main_program.set_uniform("light.diffuse", glm::vec3{0.5f, 0.5f, 0.5f});
	main_program.set_uniform("light.specular", glm::vec3{1.0f, 1.0f, 1.0f});

	main_program.set_uniform("projectionMatrix", glm::perspective(glm::radians(45.0f), (float)width / height, 0.1f, 10000.0f));
}

ForwardRenderer::~ForwardRenderer(){
    for(auto& [id, model] : internal_batches)
		model.clean();

	main_program.clean();
}

void ForwardRenderer::framebuffer_resize_callback(size_t width, size_t height){
	main_program.set_uniform("projectionMatrix", glm::perspective(glm::radians(45.0f), (float)width / height, 0.1f, 10000.0f));
}

void ForwardRenderer::draw(std::unordered_map<benzene::ModelId, benzene::Batch*>& batches, benzene::FrameData& frame_data){
	camera.process_input(frame_data.delta_time);
    // First things first, create state of batches that the backend understands
    for(auto& [id, batch] : batches){
		if(internal_batches.count(id) == 0 || batch->is_updated()){
		    internal_batches[id].clean();
			internal_batches[id] = Batch{*batch, main_program};
		}
    }

    glClearColor(this->clear_colour.r, this->clear_colour.g, this->clear_colour.b, this->clear_colour.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	main_program.set_uniform("viewMatrix", camera.get_view_matrix());
	main_program.set_uniform("cameraPos", camera.get_position());

    for(const auto& [id, object] : internal_batches)
        object.draw();
};