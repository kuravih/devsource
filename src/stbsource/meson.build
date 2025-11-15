lib_include = include_directories('../lib')

executable(
    'stbsource',
    'stbsource_main.cpp',
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
