project('keyremap', 'cpp', version: '1.0.0', default_options : ['warning_level=3', 'werror=false', 'cpp_std=c++20'])

src = files(
  'src/main.cpp',
  'src/config.cpp',
)

viewer_src = files(
  'src/input-viewer.cpp',
)

executable('keyremap', src, install : true)
executable('input-viewer', viewer_src, install : true)
