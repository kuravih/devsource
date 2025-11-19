libusb10_dep = dependency('libusb-1.0')

conf = configure_file(
    input: 'flisource_path_def.h.in',
    output: 'flisource_path_def.h',
    configuration: {
        'SRC_ROOT': meson.project_source_root()
    }
)

libfliusb_path = '../lib/libfliusb'
libfliusb_a = 'libfliusb.a' 
libfliusb_a_path = libfliusb_path + '/' + libfliusb_a
libfliusb_build = custom_target(
    output : libfliusb_a,
    command : ['make', '-C', libfliusb_path],
    build_by_default : true,
    build_always_stale : true
)

lib_include = include_directories('../lib')
executable(
    'flisource',
    ['flisource_main.cpp', conf],
    include_directories : [
        lib_include
    ],
    dependencies : [libusb10_dep],
    cpp_args : [
        '-Wall',
        '-Wextra',
        '-Wunused',
        '-Wformat',
        '-O3'
    ],
    link_args : [libfliusb_a_path, '-lpthread', '-lzmq'],
    install : false
)
