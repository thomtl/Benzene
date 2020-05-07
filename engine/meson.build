engine_include = include_directories('api/')
engine_sources = files('core/main.cpp', 
    'core/model.cpp',
    'core/utils.cpp')
engine_cpp_args = ['-Wall', '-Wextra', '-Wdeprecated-copy-dtor', '-Werror', '-Wno-unknown-pragmas', '-std=c++2a']
engine_deps = [dependency('glfw3')]

imgui_dep = static_library('imgui', files('libs/imgui/imgui_demo.cpp', 'libs/imgui/imgui_draw.cpp', 'libs/imgui/imgui_widgets.cpp', 'libs/imgui/imgui.cpp'))
stb_dep = static_library('stb', files('libs/stb/stb_image.cpp'))
tinyobjloader_dep = static_library('tinyobjloader', files('libs/tinyobjloader/tinyobjloader.cpp'))

#subdir('backends/vulkan')
subdir('backends/opengl')