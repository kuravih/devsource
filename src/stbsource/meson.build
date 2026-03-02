threads_dep = dependency('threads')
zmq_dep = dependency('libzmq', required: true)

lib_path = '../lib'

lib_inc = include_directories(lib_path)

conf = configure_file(
    input: 'stbsource_conf_def.h.in',
    output: 'stbsource_conf_def.h',
    configuration: {
        'VERSION': meson.project_version(),
        'LIB_KATO_DIR': join_paths(meson.project_source_root(), 'lib', 'kato')
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
