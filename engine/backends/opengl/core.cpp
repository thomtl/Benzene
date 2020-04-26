#include "core.hpp"
#include "../../core/format.hpp"
#include <mutex>
#include <thread>

using namespace benzene::opengl;

static void APIENTRY glDebugOutput(GLenum source, GLenum type, GLuint id, GLenum severity, [[maybe_unused]] GLsizei length, const GLchar* message, [[maybe_unused]] const void* userptr){
	static std::mutex debug_lock{};
	std::lock_guard guard{debug_lock};

	if(id == 1 || id == 2)
		return;

	print("--------------------------------\n");
	print("Debug message ({:d}): {:s}\n", (uint64_t)id, (const char*)message);

	switch (source)
	{
		case GL_DEBUG_SOURCE_API: print("Source: API\n"); break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM: print("Source: Window System\n"); break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER: print("Source: Shader Compiler\n"); break;
		case GL_DEBUG_SOURCE_THIRD_PARTY: print("Source: Third Party\n"); break;
		case GL_DEBUG_SOURCE_APPLICATION: print("Source: Application\n"); break;
		case GL_DEBUG_SOURCE_OTHER: print("Source: Other\n"); break;
	}

	switch (type)
	{
		case GL_DEBUG_TYPE_ERROR: print("Type: Error\n"); break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: print("Type: Deprecated behaviour\n"); break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: print("Type: UB\n"); break;
		case GL_DEBUG_TYPE_PORTABILITY: print("Type: Portability\n"); break;
		case GL_DEBUG_TYPE_PERFORMANCE: print("Type: Performance\n"); break;
		case GL_DEBUG_TYPE_MARKER: print("Type: Marker\n"); break;
		case GL_DEBUG_TYPE_PUSH_GROUP: print("Type: Push Group\n"); break;
		case GL_DEBUG_TYPE_POP_GROUP: print("Type: Pop Group\n"); break;
		case GL_DEBUG_TYPE_OTHER: print("Type: Other\n"); break;
	}

	switch (severity)
	{
		case GL_DEBUG_SEVERITY_HIGH: print("Severity: High\n"); break;
		case GL_DEBUG_SEVERITY_MEDIUM: print("Severity: Medium\n"); break;
		case GL_DEBUG_SEVERITY_LOW: print("Severity: Low\n"); break;
		case GL_DEBUG_SEVERITY_NOTIFICATION: print("Severity: Notification\n"); break;
	}
}

Backend::Backend([[maybe_unused]] const char* application_name, GLFWwindow* window): is_wireframe{false} {
	frame_time = 0.0f;
	max_frame_time = 0.0f;
	min_frame_time = 9999.0f;
	last_frame_times = {};
	frame_counter = 0;
	fps = 0;
	fps_cap_enabled = false;
	glfwSwapInterval(0); // Remove 60 FPS Cap
	print("opengl: Starting OpenGL Backend\n");

	if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		throw std::runtime_error("benzene/opengl: Failed to initialize GLAD");

	if constexpr (validation) {
		GLint flags;
		glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
		if(flags & GL_CONTEXT_FLAG_DEBUG_BIT){
			glEnable(GL_DEBUG_OUTPUT);
			glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

			glDebugMessageCallback(glDebugOutput, nullptr);
			glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
		}
	}

	if constexpr (debug) {
		print("opengl: Vendor: {:s}\n", glGetString(GL_VENDOR));
		print("        Renderer: {:s}\n", glGetString(GL_RENDERER));
		print("        GL Version: {:s}\n", glGetString(GL_VERSION));
		print("        GLSL Version: {:s}\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
		GLint msaa_buffers, msaa_samples;
		glGetIntegerv(GL_SAMPLE_BUFFERS, &msaa_buffers);
		glGetIntegerv(GL_SAMPLES, &msaa_samples);
		print("        MSAA: Buffers: {:d}, samples: {:d}\n", msaa_buffers, msaa_samples);

		/*GLint n_extensions;
		glGetIntegerv(GL_NUM_EXTENSIONS, &n_extensions);
		print("opengl: Supported extensions\n");
		for(int i = 0; i < n_extensions; i++)
			print("\t- {:s}\n", glGetStringi(GL_EXTENSIONS, i));*/
	}

	int width, height;
	glfwGetWindowSize(window, &width, &height);
	glViewport(0, 0, width, height);

	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);

	glEnable(GL_CULL_FACE);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_MULTISAMPLE);

	//glLineWidth(2.0f);

	prog.add_shader(GL_VERTEX_SHADER, R"(#version 420 core
		uniform mat4 modelMatrix;
		uniform mat4 viewMatrix;
		uniform mat4 projectionMatrix;
		uniform mat3 normalMatrix;
		
		layout (location = 0) in vec3 inPosition;
		layout (location = 1) in vec3 inColour;
		layout (location = 2) in vec3 inNormal;
		layout (location = 3) in vec2 inUv;
		
		layout (location = 0) out vec3 outWorldPosition;
		layout (location = 1) out vec3 outColour;
		layout (location = 2) out vec3 outNormal;
		layout (location = 3) out vec2 outUv;
		void main() {
		   gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(inPosition.xyz, 1.0);
		   outWorldPosition = vec3(modelMatrix * vec4(inPosition.xyz, 1.0));
		   outColour = inColour;
		   outUv = inUv;
		   outNormal = normalMatrix * inNormal.xyz;
		})");

	prog.add_shader(GL_FRAGMENT_SHADER, R"(#version 420 core
		struct Material {
			sampler2D diffuse;
		};
		uniform Material material;
		uniform vec3 cameraPosition;
		
		layout (location = 0) in vec3 inWorldPosition;
		layout (location = 1) in vec3 inColour;
		layout (location = 2) in vec3 inNormal;
		layout (location = 3) in vec2 inUv;
		
		const vec3 lightColour = vec3(1.0, 1.0, 1);
		const vec3 lightPosition = vec3(500.0, 200.0, 300.0);//1.2, 1.0, 2.0);
		const float ambientStrength = 0.1;
		const float specularStrength = 0.5;
		const bool blinn = true;

		
		out vec4 fragColour;
		void main() {
		   	vec3 normal = normalize(inNormal);
		   	vec3 lightDir = normalize(lightPosition - inWorldPosition);
		   	vec3 cameraDir = normalize(cameraPosition - inWorldPosition);
		   
		   	vec3 ambient = lightColour * ambientStrength;
		   
		   	float diffuseIntensity = max(dot(normal, lightDir), 0.0);
		   	vec3 diffuse = lightColour * diffuseIntensity;

			float specularIntensity;
			if (blinn){
				vec3 halfwayDir = normalize(lightDir + cameraDir);
				specularIntensity = pow(max(dot(normal, halfwayDir), 0.0), 32);
			} else {
				vec3 reflectDir = reflect(-lightDir, normal);
				specularIntensity = pow(max(dot(cameraDir, reflectDir), 0.0), 32);
			}
			vec3 specular = specularStrength * specularIntensity * lightColour;
		   
		   	vec3 result = (ambient + diffuse + specular) * (inColour.xyz * texture(material.diffuse, inUv).xyz);
		   	fragColour = vec4(result, 1.0);
		})");

	prog.compile();
	prog.use();

	auto cameraPosition = glm::vec3{3.0f, 3.0f, 3.0f};
	prog.set_uniform("viewMatrix", glm::lookAt(cameraPosition, glm::vec3{0, 0, 0}, glm::vec3{0.0f, 1.0f, 0.0f}));
	prog.set_uniform("projectionMatrix", glm::perspective(glm::radians(45.0f), (float)width / height, 0.1f, 100.0f));
	prog.set_uniform("cameraPosition", cameraPosition);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init();
}

Backend::~Backend(){
	for(auto& [id, model] : internal_models)
		model.clean();

	prog.clean();

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
}

void Backend::framebuffer_resize_callback(int width, int height){
	glViewport(0, 0, width, height);
	prog.use();
	prog.set_uniform("projectionMatrix", glm::perspective(glm::radians(45.0f), (float)width / height, 0.1f, 10.0f));
}

void Backend::frame_update(std::unordered_map<benzene::ModelId, benzene::Model*>& models){
	auto time_begin = std::chrono::high_resolution_clock::now();

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// First things first, create state of models that the backend understands
	for(auto& [id, model] : models){
		if(internal_models.count(id) == 0 || model->is_updated()){
			internal_models[id].clean();
			internal_models[id] = Model{*model, prog};
		}
	}

	for(const auto& [id, model] : internal_models)
		model.draw();

	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	this->frame_counter++;

	auto time_end = std::chrono::high_resolution_clock::now();
	frame_time = (float)(std::chrono::duration<double, std::milli>(time_end - time_begin).count());

	auto fps_timer = (float)std::chrono::duration<double, std::milli>(time_end - last_frame_timestamp).count();
	if(fps_timer > 1000.0f){
		fps = ((float)frame_counter * (1000.0f / fps_timer));
		frame_counter = 0;
		last_frame_timestamp = time_end;
	}
	
	if(fps_cap_enabled)
		std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(std::chrono::duration<double, std::milli>(1000 / this->fps_cap) - (time_end - time_begin)));
}

void Backend::imgui_update(){
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
}

void Backend::end_run(){
	glFinish();
}

void Backend::draw_debug_window(){
	ImGui::Begin("Benzene");

	if constexpr (wireframe_rendering){
		ImGui::Checkbox("Wireframe rendering", &this->is_wireframe);

		if(this->is_wireframe)
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		else
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	std::rotate(last_frame_times.begin(), last_frame_times.begin() + 1, last_frame_times.end());
	last_frame_times.back() = this->frame_time;

	if(frame_time < min_frame_time)
		min_frame_time = frame_time;

	if(frame_time > max_frame_time)
		max_frame_time = frame_time;

	ImGui::PlotLines("Frame times (ms)", last_frame_times.data(), last_frame_times.size(), 0, "", min_frame_time, max_frame_time, ImVec2{0, 80});
	ImGui::Text("FPS: %f\n", this->fps);
	ImGui::End();
}