project('include_directories', 'cpp')

lib = static_library('foo', 'foo.cpp', include_directories : include_directories('include'))

executable('main', 'main.cpp', include_directories : include_directories('include/subdir'), link_with : lib)
