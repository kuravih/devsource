threads_dep = dependency('threads')
zmq_dep = dependency('libzmq', required: true)

lib_path = '../lib'

lib_inc = include_directories(lib_path)

cc = meson.get_compiler('cpp')
vmbcpp_dep = cc.find_library('VmbCPP', required: true)

conf = configure_file(
    input: 'vmbsource_conf_def.h.in',
    output: 'vmbsource_conf_def.h',
    configuration: {
        'VERSION': meson.project_version(),
        'LIB_KATO_DIR': join_paths(meson.project_source_root(), 'lib', 'kato')
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
