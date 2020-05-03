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
} // !benzene::opengl