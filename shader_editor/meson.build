imnodes_dep = static_library('imnodes', files('libs/imnodes.cpp'))
cpp_args = ['-Wall', '-Wextra', '-Werror', '-Wno-unknown-pragmas', '-std=c++2a']
executable('benzene-shader_editor', 'main.cpp', 'editor.cpp', 'compiler.cpp', cpp_args: cpp_args, dependencies: benzene_dep_opengl, link_with: imnodes_dep)