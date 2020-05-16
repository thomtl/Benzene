opengl_deps = [engine_deps]
opengl_sources = files('core.cpp', 'model/model.cpp', 'renderer/forward.cpp')

cc = meson.get_compiler('cpp')
dl_dep = cc.find_library('dl', required: false)
glad_dep = static_library('glad', files('libs/glad/source/glad.c'), include_directories: include_directories('libs/glad/include'), dependencies: [dl_dep])

imgui_renderer_dep = static_library('imgui-renderer', files('libs/imgui/imgui_impl_glfw.cpp', 'libs/imgui/imgui_impl_opengl3.cpp'), cpp_args: ['-DIMGUI_IMPL_OPENGL_LOADER_GLAD'])

opengl_cpp_args = [engine_cpp_args, '-DBENZENE_OPENGL', '-DIMGUI_IMPL_OPENGL_LOADER_GLAD']
engine_lib_opengl = shared_library('benzene-opengl', engine_sources, opengl_sources, cpp_args: opengl_cpp_args, include_directories: engine_include, dependencies: opengl_deps, link_with: [glad_dep, imgui_dep, imgui_renderer_dep, stb_dep, tinyobjloader_dep])
benzene_dep_opengl = declare_dependency(link_with: engine_lib_opengl, dependencies: opengl_deps, include_directories: engine_include)