#include "core.hpp"
#include "../../core/format.hpp"

using namespace benzene::opengl;

static void APIENTRY glDebugOutput(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userptr){
    (void)length;
    (void)userptr;

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

    print("--------------------------------\n");
}

Backend::Backend(const char* application_name, GLFWwindow* window){
    print("opengl: Starting OpenGL Backend\n");

    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        throw std::runtime_error("benzene/opengl: Failed to initialize GLAD");


    GLint flags;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if(flags & GL_CONTEXT_FLAG_DEBUG_BIT){
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

        glDebugMessageCallback(glDebugOutput, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
    }

    int width, height;
    glfwGetWindowSize(window, &width, &height);
    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);

    prog.add_shader(GL_VERTEX_SHADER, ""
        "#version 450 core\n"
        "uniform mat4 model;\n"
        "uniform mat4 view;\n"
        "uniform mat4 projection;\n"
        "\n"
        "layout (location = 0) in vec3 inPosition;\n"
        "layout (location = 1) in vec3 inColour;\n"
        "layout (location = 2) in vec2 inUv;\n"
        "\n"
        "layout (location = 0) out vec3 outColour;\n"
        "layout (location = 1) out vec2 outUv;\n"
        "void main() {\n"
        "   gl_Position = projection * view * model * vec4(inPosition.xyz, 1.0);\n"
        "   outColour = inColour;\n"
        "   outUv = inUv;\n"
        "}\n");

    prog.add_shader(GL_FRAGMENT_SHADER, ""
        "#version 450 core\n"
        "uniform sampler2D textureSampler;\n"
        "\n"
        "layout (location = 0) in vec3 inColour;\n"
        "layout (location = 1) in vec2 inUv;\n"
        "\n"
        "out vec4 fragColour;\n"
        "void main() {\n"
        "   fragColour = texture(textureSampler, inUv) * vec4(inColour, 1.0);\n"
        "}\n");

    prog.compile();
    prog.use();
    prog.set_uniform("view", glm::lookAt(glm::vec3{2.0f, 2.0f, 2.0f}, glm::vec3{0.0f, 0.0f, 0.0f}, glm::vec3{0.0f, 1.0f, 0.0f}));
    prog.set_uniform("projection", glm::perspective(glm::radians(45.0f), (float)width / height, 0.1f, 10.0f));

    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init();

    (void)application_name;
}

Backend::~Backend(){

}

void Backend::framebuffer_resize_callback(int width, int height){
    glViewport(0, 0, width, height);
    prog.use();
    prog.set_uniform("projection", glm::perspective(glm::radians(45.0f), (float)width / height, 0.1f, 10.0f));
}

void Backend::frame_update(std::unordered_map<benzene::ModelId, benzene::Model*>& models){
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // First things first, create state of models that the backend understands
    for(auto& [id, model] : models){
        auto it = internal_models.find(id);
        if(it != internal_models.end())
            continue; // Already exists

        std::vector<Vertex> internal_vertices{};
        for(auto& vertex : model->mesh.vertices){
            auto& item = internal_vertices.emplace_back();
            item.position = vertex.pos;
            item.colour = vertex.colour;
            item.uv = vertex.uv;
        }

        std::vector<uint32_t> internal_indicies{};
        for(const auto index : model->mesh.indices)
            internal_indicies.push_back(index);

        opengl::Model item{internal_vertices, internal_indicies, model->texture};
        item.model = model;

        internal_models[id] = std::move(item);
    }

    for(auto& [id, draw] : internal_models){
        glm::mat4 model = glm::translate(glm::mat4{1.0f}, draw.model->pos);
        model = glm::rotate(model, glm::radians(draw.model->rotation.x), glm::vec3{1.0f, 0.0f, 0.0f});
        model = glm::rotate(model, glm::radians(draw.model->rotation.y), glm::vec3{0.0f, 1.0f, 0.0f});
        model = glm::rotate(model, glm::radians(draw.model->rotation.z), glm::vec3{0.0f, 0.0f, 1.0f});
        model = glm::scale(model, draw.model->scale);
    
        prog.use();
        prog.set_uniform("model", model);
        prog.set_uniform("textureSampler", 0);
        draw.draw();
    }

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    
}

void Backend::imgui_update(){
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
}

void Backend::end_run(){

}