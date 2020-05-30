#pragma once

#include "libs/glad/include/glad/glad.h"
#include <GLFW/glfw3.h>

#include <benzene/benzene.hpp>

#include "../../core/format.hpp"

template<>
struct format::formatter<const GLubyte*> {
	template<typename OutputIt>
	static void format(format::format_output_it<OutputIt>& it, format::format_args args, const GLubyte* item){        
		formatter<const char*>::format(it, args, (const char*)item);
    }
};

namespace benzene::opengl {
    constexpr bool validation = true;
    constexpr bool debug = true;
    constexpr bool wireframe_rendering = true;

    class Backend;
    class Program;

    class IRenderer {
        public:
        virtual ~IRenderer() {}
        virtual void draw(std::unordered_map<benzene::ModelId, benzene::Batch*>& batches, benzene::FrameData& frame_data) = 0;
        virtual void framebuffer_resize_callback(size_t width, size_t height) = 0;
        glm::vec4 clear_colour;
    };
} // !benzene::opengl

namespace gl {
    template<typename T>
    struct type_to_enum { };

    template<>
    struct type_to_enum<unsigned int> { static constexpr GLenum value = GL_UNSIGNED_INT; };

    template<>
    struct type_to_enum<float> { static constexpr GLenum value = GL_FLOAT; };

    template<typename T>
    inline constexpr GLenum type_to_enum_v = type_to_enum<T>::value;

    template<typename... Options>
    void enable(Options&&... options){
        (glEnable(options), ...);
    }

    template<typename... Options>
    void disable(Options&&... options){
        (glDisable(options), ...);
    }
    
    struct DrawCommand {
        uint32_t instance_count;
        uint32_t base_instance;

        uint32_t first_index;
        uint32_t index_count;

        uint32_t base_vertex;
    };
    
    template<typename IndexType, GLenum draw_mode = GL_TRIANGLES>
    void draw(const DrawCommand& cmd){
        glDrawElementsInstancedBaseVertexBaseInstance(draw_mode, cmd.index_count, gl::type_to_enum_v<IndexType>, (const void*)(uintptr_t)cmd.first_index, cmd.instance_count, cmd.base_vertex, cmd.base_instance);
    }

    struct InstanceData {
        glm::mat4 model_matrix;
        glm::mat4 normal_matrix;
    };
}