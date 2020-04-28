project('Benzene', 'cpp', 'c')

if true
    add_global_arguments('-fsanitize=undefined', language: 'cpp')
    add_global_link_arguments('-fsanitize=undefined', language: 'cpp')
endif

if true
    add_global_arguments('-fsanitize=address', language: 'cpp')
    add_global_link_arguments('-fsanitize=address', language: 'cpp')
endif

subdir('engine/')
subdir('shader_editor/')
subdir('test/')