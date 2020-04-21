vulkan_deps = [dependency('vulkan'), engine_deps]
vulkan_sources = files('core.cpp', 
    'extra_api.cpp',
    'renderer.cpp')

vma_dep = static_library('vma', files('libs/vk_mem_alloc_impl.cpp'), cpp_args: ['-Wno-unused-variable', '-Wno-reorder', '-Wno-class-memaccess'])

vulkan_cpp_args = [engine_cpp_args, '-DBENZENE_VULKAN']
engine_lib_vulkan = shared_library('benzene-vulkan', engine_sources, vulkan_sources, cpp_args: vulkan_cpp_args, include_directories: engine_include, dependencies: vulkan_deps, link_with: [vma_dep, imgui_dep, stb_dep])
benzene_dep_vulkan = declare_dependency(link_with: engine_lib_vulkan, dependencies: vulkan_deps, include_directories: engine_include)