project('Benzene', 'cpp')

if false
    add_global_arguments('-fsanitize=undefined', language: 'cpp')
    add_global_link_arguments('-fsanitize=undefined', language: 'cpp')
endif

if false
    add_global_arguments('-fsanitize=address', language: 'cpp')
    add_global_link_arguments('-fsanitize=address', language: 'cpp')
endif

subdir('engine/')
subdir('test/')