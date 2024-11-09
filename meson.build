project('vulkan', 'c', default_options : ['c_std=c11'])

is_windows = false

if host_machine.system() == 'windows'
    is_windows = true
endif

cc = meson.get_compiler('c')

config_dep = dependency('iniparser')
sdl2_dep = dependency('sdl2')
sdl2_main_dep = dependency('sdl2main', required:is_windows)
vulkan_dep = dependency('vulkan')

m_dep = cc.find_library('m', required : false)

raylib_path = include_directories('third-party-libs/Raylib')
qoi_path    = include_directories('third-party-libs/qoi')
cgltf_path  = include_directories('third-party-libs/cgltf')

executable('vulkan-test', ['src/main.c', 'src/u_collision.c', 'src/u_config.c', 'src/u_maze.c', 'src/u_read.c', 'src/u_vector.c', 'src/v_buffer.c', 'src/v_init.c', 'src/v_model.c', 'src/v_raymath.c', 'src/v_render.c'], dependencies: [config_dep, sdl2_dep, sdl2_main_dep, m_dep, vulkan_dep], include_directories: [raylib_path, qoi_path, cgltf_path])
