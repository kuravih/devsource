threads_dep = dependency('threads')
zmq_dep = dependency('libzmq', required: true)

lib_path = '../lib'

lib_inc = include_directories(lib_path)

cc = meson.get_compiler('cpp')
vmbcpp_dep = cc.find_library('VmbCPP', required: true)

conf = configure_file(
    input: 'vmbsource_path_def.h.in',
    output: 'vmbsource_path_def.h',
    configuration: {
        'SRC_ROOT': meson.project_source_root()
    }
)

executable(
    'vmbsource',
    ['vmbsource_main.cpp', conf],
    include_directories : lib_inc,
    dependencies : [
        threads_dep,
        zmq_dep,
        vmbcpp_dep
    ],
    install : false
)
