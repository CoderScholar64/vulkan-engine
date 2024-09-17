project('vulkan', 'c')

sdl2_dep = dependency('sdl2')
sdl2_main_dep = dependency('sdl2main', required:false)

executable('vulkan-test', 'src/main.c', dependencies: [sdl2_dep, sdl2_main_dep])
