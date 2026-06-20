# Source asset collection relative to this folder
sources = files(
  'main.cpp',
  'output_manager.cpp'
)

# Generate project build layout rules
executable('orientprompt',
  sources,
  dependencies: [
    gtkmm_dep,
    glibmm_dep,
    layer_shell_dep,
    wayland_client, # Keep this linked for standard wayland includes
    wf_protos       # Inject your custom compiled wayland protocols static library
  ],
  install: true
)
