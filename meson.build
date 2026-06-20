# Fix: Added 'c' to enable the C compiler for wayland-scanner outputs
project('orientprompt', 'c', 'cpp',
  version: '1.0.0',
  default_options: [
    'c_std=c11',
    'cpp_std=c++20',
    'warning_level=3',
    'werror=false'
  ]
)
gtkmm_dep       = dependency('gtkmm-4.0', version: '>= 4.0.0')
glibmm_dep      = dependency('glibmm-2.68', version: '>= 2.68.0')
layer_shell_dep = dependency('gtk4-layer-shell-0', version: '>= 1.0.0')
wayland_client  = dependency('wayland-client')
wayland_protos  = dependency('wayland-protocols', version: '>= 1.14')

subdir('protocol')
subdir('src')
