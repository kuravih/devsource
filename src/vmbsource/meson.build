lib_include = include_directories('../lib')

executable(
    'vmbsource',
    'vmbsource_main.cpp',
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
    link_args : ['-lVmbCPP', '-lpthread', '-lzmq'],
    install : false
)
