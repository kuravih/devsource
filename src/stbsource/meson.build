threads_dep = dependency('threads')
zmq_dep = dependency('libzmq', required: true)

lib_path = '../lib'

lib_inc = include_directories(lib_path)

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
    include_directories : lib_inc,
    dependencies : [
        threads_dep,
        zmq_dep
    ],
    install : false
)
