threads_dep = dependency('threads')
zmq_dep = dependency('libzmq', required: true)
libusb10_dep = dependency('libusb-1.0')

conf = configure_file(
    input: 'flisource_path_def.h.in',
    output: 'flisource_path_def.h',
    configuration: {
        'SRC_ROOT': meson.project_source_root()
    }
)

lib_path = '../lib'

libfliusb_path = lib_path + '/libfliusb'
libfliusb_a = 'libfliusb.a' 
libfliusb_a_path = libfliusb_path + '/' + libfliusb_a
libfliusb_build = custom_target(
    output : libfliusb_a,
    command : ['make', '-C', libfliusb_path],
    build_by_default : true,
    build_always_stale : true
)

lib_inc = include_directories(lib_path)
executable(
    'flisource',
    ['flisource_main.cpp', conf],
    include_directories : lib_inc,
    dependencies : [
        libusb10_dep,
        threads_dep,
        zmq_dep
    ],
    link_args : libfliusb_a_path,
    install : false
)
