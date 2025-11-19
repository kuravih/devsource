lib_include = include_directories('../lib')

conf = configure_file(
    input: 'stbsource_path_def.h.in',
    output: 'stbsource_path_def.h',
    configuration: {
        'SRC_ROOT': meson.project_source_root()
    }
)

executable(
    'stbsource',
    ['stbsource_main.cpp', conf],
    include_directories : [
        lib_include
    ],
    cpp_args : [
        '-Wall',
        '-Wextra',
        '-Wunused',
        '-Wformat',
        '-O3'
    ],
    link_args : ['-lpthread', '-lzmq'],
    install : false
)
