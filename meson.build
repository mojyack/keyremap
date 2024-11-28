project('keyremap', 'cpp', version: '1.0.0', default_options : ['warning_level=3', 'werror=false', 'cpp_std=c++20'])
add_project_arguments('-Wno-missing-field-initializers', language: 'cpp')

src = files(
  'src/main.cpp',
  'src/config.cpp',
  'src/uinput.cpp',
)

viewer_src = files(
  'src/input-viewer.cpp',
  'src/uinput.cpp',
)

executable('keyremap', src, install : true)
executable('input-viewer', viewer_src, install : true)
