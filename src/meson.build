vkdecor_sources = ['decoration.cpp', 'deco-subsurface.cpp', 'deco-button.cpp', 'deco-layout.cpp', 'deco-theme.cpp', 'deco-effects.cpp']
vkdecor_sources += vulkan_shaders
vkdecor = shared_module('vkdecor',
        vkdecor_sources,
        dependencies: [wayfire],
        install: true,
        install_dir: join_paths(get_option('libdir'), 'wayfire'))
