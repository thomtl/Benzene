#include "core.hpp"
#include "../../core/format.hpp"
#include <mutex>
#include <thread>
#include <regex>
#include "renderer/forward.hpp"


using namespace benzene::opengl;

static void APIENTRY glDebugOutput(GLenum source, GLenum type, GLuint id, GLenum severity, [[maybe_unused]] GLsizei length, const GLchar* message, [[maybe_unused]] const void* userptr){
	static std::mutex debug_lock{};
	std::lock_guard guard{debug_lock};

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
	extension_window_is_showing = false;
	driver_info_window_is_showing = false;
	glfwSwapInterval(0); // Disable Vsync

	if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		throw std::runtime_error("benzene/opengl: Failed to initialize GLAD");

	if constexpr (validation) {
		GLint flags;
		glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
		if(flags & GL_CONTEXT_FLAG_DEBUG_BIT){
			glEnable(GL_DEBUG_OUTPUT);
			glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

			glDebugMessageCallback(glDebugOutput, nullptr);
			glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
		}
	}

	if(!GLAD_GL_ARB_direct_state_access){
		print("opengl: Need GL_ARB_direct_state_access, which the current driver does not support\n");
		return;
	}

	int width, height;
	glfwGetWindowSize(window, &width, &height);
	glViewport(0, 0, width, height);

	gl::enable(GL_CULL_FACE, GL_DEPTH_TEST, GL_MULTISAMPLE, GL_FRAMEBUFFER_SRGB); // Enable Face-Culling, Depth-Testing, MSAA and Gamma correction

	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);

	this->renderer = new ForwardRenderer{};
	
	renderer->program().set_uniform("projectionMatrix", glm::perspective(glm::radians(45.0f), (float)width / height, 0.1f, 100.0f));

	renderer->program().set_uniform("light.position", glm::vec3{-3.0f, 2.0f, 0.0f});
	renderer->program().set_uniform("light.ambient", glm::vec3{0.2f, 0.2f, 0.2f});
	renderer->program().set_uniform("light.diffuse", glm::vec3{0.5f, 0.5f, 0.5f});
	renderer->program().set_uniform("light.specular", glm::vec3{1.0f, 1.0f, 1.0f});

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init();

	print("opengl: Started OpenGL Backend\n");
}

Backend::~Backend(){
	delete this->renderer;

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
}

void Backend::framebuffer_resize_callback(int width, int height){
	glViewport(0, 0, width, height);

	renderer->program().set_uniform("projectionMatrix", glm::perspective(glm::radians(45.0f), (float)width / height, 0.1f, 100.0f));
}

void Backend::frame_update(std::unordered_map<benzene::ModelId, benzene::Model*>& models){
	auto time_begin = std::chrono::high_resolution_clock::now();

	static double S = 0;
	auto cameraPosition = glm::vec3{sin(S) * 20, 0.0f, cos(S) * 20};
	S += 0.001;
	renderer->program().set_uniform("viewMatrix", glm::lookAt(cameraPosition, glm::vec3{0, 0, 0}, glm::vec3{0.0f, 1.0f, 0.0f}));
	renderer->program().set_uniform("cameraPos", cameraPosition);

	renderer->draw(models);
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
	ImGui::Begin("Benzene", NULL, ImGuiWindowFlags_MenuBar);

	if(ImGui::BeginMenuBar()){
		if(ImGui::MenuItem("Extensions"))
			extension_window_is_showing = !extension_window_is_showing;
		if(ImGui::MenuItem("Driver"))
			driver_info_window_is_showing = !driver_info_window_is_showing;
		ImGui::EndMenuBar();
	}

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

	if(extension_window_is_showing)
		this->show_extension_window(extension_window_is_showing);
		
	if(driver_info_window_is_showing)
		this->show_driver_info_window(driver_info_window_is_showing);
}

void Backend::show_extension_window(bool& opened){
	ImGui::Begin("Extension Query", &opened);

	char buf[128] = {};
	ImGui::Text("Query: ");
	ImGui::SameLine(0, 0);
	ImGui::InputText("", buf, 128);

	std::regex regex{std::string{buf}};
	
	ImGui::BeginChild("Scrolling", ImVec2{0, 0}, true);

	GLint n = 0;
	glGetIntegerv(GL_NUM_EXTENSIONS, &n);

	for(GLint i = 0; i < n; i++){
		auto str = std::string{(const char*)glGetStringi(GL_EXTENSIONS, i)};
		if(std::regex_search(str, regex))
			ImGui::Text("%s", str.c_str());
	}

	ImGui::EndChild();
	ImGui::End();
}

void Backend::show_driver_info_window(bool& opened){
    auto get_integer = [](GLenum v) -> GLint {
		GLint i{};
		glGetIntegerv(v, &i);
		return i;
	};

    auto show_state_info = [&get_integer](){
        ImGui::TextUnformatted(format_to_str("opengl: Context Version: {:d}.{:d}", get_integer(GL_MAJOR_VERSION), get_integer(GL_MINOR_VERSION)).c_str());
	    ImGui::TextUnformatted(format_to_str("        Vendor: {:s}", glGetString(GL_VENDOR)).c_str());
	    ImGui::TextUnformatted(format_to_str("        Renderer: {:s}", glGetString(GL_RENDERER)).c_str());
	    ImGui::TextUnformatted(format_to_str("        GL Version: {:s}", glGetString(GL_VERSION)).c_str());
    	ImGui::TextUnformatted(format_to_str("        GLSL Version: {:s}", glGetString(GL_SHADING_LANGUAGE_VERSION)).c_str());
	    ImGui::TextUnformatted(format_to_str("        MSAA: Buffers: {:d}, samples: {:d}", get_integer(GL_SAMPLE_BUFFERS), get_integer(GL_SAMPLES)).c_str());
    };

    auto show_capabilities_info = [&get_integer](){
        ImGui::TextUnformatted(format_to_str("opengl: Capabilities:\n").c_str());
	    ImGui::TextUnformatted(format_to_str("        Max Clip Distances: {:d}", get_integer(GL_MAX_CLIP_DISTANCES)).c_str());
	    ImGui::TextUnformatted(format_to_str("        Max Texture Units: {:d}", get_integer(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS)).c_str());
	    ImGui::TextUnformatted(format_to_str("        Max Uniform Blocks: {:d}", get_integer(GL_MAX_COMBINED_UNIFORM_BLOCKS)).c_str());
	    ImGui::TextUnformatted(format_to_str("        Max SSBO Bindings: {:d}", get_integer(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS)).c_str());
	    ImGui::TextUnformatted(format_to_str("        Max Vertex Attribs: {:d}", get_integer(GL_MAX_VERTEX_ATTRIBS)).c_str());
	    ImGui::TextUnformatted(format_to_str("        Max Fragment Outputs: {:d}", get_integer(GL_MAX_DRAW_BUFFERS)).c_str());
	    ImGui::TextUnformatted(format_to_str("        Max Framebuffer Dimensions: {:d}x{:d}", get_integer(GL_MAX_FRAMEBUFFER_WIDTH), get_integer(GL_MAX_FRAMEBUFFER_HEIGHT)).c_str());
	    ImGui::TextUnformatted(format_to_str("        Max MSAA Samples: {:d}", get_integer(GL_MAX_INTEGER_SAMPLES)).c_str());
        ImGui::TextUnformatted(format_to_str("        Max Tesselation Patch Vertices: {:d}", get_integer(GL_MAX_PATCH_VERTICES)).c_str());
    };

    ImGui::Begin("Driver Info", &opened);
    {
        if(ImGui::BeginTabBar("Tab bar")){
            if(ImGui::BeginTabItem("State")){
                show_state_info();
                ImGui::EndTabItem();
            }

            if(ImGui::BeginTabItem("Capabilities")){
                show_capabilities_info();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }   
    }
    ImGui::End();
}