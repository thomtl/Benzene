engine_include = include_directories('api/')
engine_deps = [dependency('vulkan'), dependency('glfw3')]
engine_lib = shared_library('benzene', files('core/main.cpp', 'core/utils.cpp', 'backends/vulkan/core.cpp', 'backends/vulkan/extra_api.cpp'), include_directories: engine_include, dependencies: engine_deps)
benzene_dep = declare_dependency(link_with: engine_lib, dependencies: engine_deps, include_directories: engine_include)