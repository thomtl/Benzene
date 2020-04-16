engine_include = include_directories('api/')
engine_deps = [dependency('vulkan'), dependency('glfw3')]

engine_sources = files('core/main.cpp', 
    'core/utils.cpp', 
    'backends/vulkan/core.cpp', 
    'backends/vulkan/extra_api.cpp',
    'backends/vulkan/renderer.cpp')

vma_dep = static_library('vma', files('backends/vulkan/libs/vk_mem_alloc_impl.cpp'), cpp_args: ['-Wno-unused-variable', '-Wno-reorder', '-Wno-class-memaccess'])
imgui_dep = static_library('imgui', files('libs/imgui/imgui_demo.cpp', 'libs/imgui/imgui_draw.cpp', 'libs/imgui/imgui_widgets.cpp', 'libs/imgui/imgui.cpp'))
stb_dep = static_library('stb', files('libs/stb/stb_image.cpp'))

engine_cpp_args = ['-Wall', '-Wextra', '-Wdeprecated-copy-dtor', '-Werror', '-Wno-unknown-pragmas', '-std=c++2a']
engine_lib = shared_library('benzene', engine_sources, cpp_args: engine_cpp_args, include_directories: engine_include, dependencies: engine_deps, link_with: [vma_dep, imgui_dep, stb_dep])
benzene_dep = declare_dependency(link_with: engine_lib, dependencies: engine_deps, include_directories: engine_include)