project('vulkan', 'c', default_options : ['c_std=c11'])

is_windows = false

if host_machine.system() == 'windows'
    is_windows = true
endif

cc = meson.get_compiler('c')

sdl2_dep = dependency('sdl2')
sdl2_main_dep = dependency('sdl2main', required:is_windows)
vulkan_dep = dependency('vulkan')

m_dep = cc.find_library('m', required : false)

raylib_path = include_directories('included_subprojects/Raylib')
qoi_path = include_directories('included_subprojects/qoi')
cgltf_path = include_directories('included_subprojects/cgltf')

executable('vulkan-test', ['src/main.c', 'src/v_init.c', 'src/v_render.c', 'src/v_buffer.c', 'src/u_read.c', 'src/v_raymath.c'], dependencies: [sdl2_dep, sdl2_main_dep, vulkan_dep, m_dep], include_directories: [raylib_path, qoi_path, cgltf_path])
