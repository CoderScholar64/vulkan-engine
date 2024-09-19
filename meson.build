project('vulkan', 'c', default_options : ['c_std=c11'])

is_windows = false

if host_machine.system() == 'windows'
    is_windows = true
endif

sdl2_dep = dependency('sdl2')
sdl2_main_dep = dependency('sdl2main', required:is_windows)
vulkan_dep = dependency('vulkan')

executable('vulkan-test', ['src/main.c', 'src/v_init.c', 'src/u_read.c'], dependencies: [sdl2_dep, sdl2_main_dep, vulkan_dep])
