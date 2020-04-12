engine_include = include_directories('api/')

engine_deps = [
    dependency('vulkan'), 
    subproject('glfw').get_variable('glfw_dep'),
    subproject('glm').get_variable('glm_dep')
]

engine_sources = [
    'core/main.cpp', 
    'core/utils.cpp', 
    'backends/vulkan/core.cpp', 
    'backends/vulkan/extra_api.cpp',
    'backends/vulkan/renderer.cpp'
]

vma_dep = static_library('vma', 'backends/vulkan/libs/vk_mem_alloc_impl.cpp', dependencies : engine_deps)

imgui_dep = static_library('imgui', ['libs/imgui/imgui_demo.cpp', 'libs/imgui/imgui_draw.cpp', 'libs/imgui/imgui_widgets.cpp', 'libs/imgui/imgui.cpp'])

engine_lib = shared_library('benzene', engine_sources, 
    include_directories: engine_include, 
    dependencies: engine_deps, 
    link_with: [vma_dep, imgui_dep]
)

benzene_dep = declare_dependency(
    link_with: engine_lib, 
    dependencies: engine_deps, 
    include_directories: engine_include
)