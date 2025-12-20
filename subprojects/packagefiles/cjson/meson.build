project('cjson', 'c',
  version : '1.7.18',
  license : 'MIT',
)

cjson_inc = include_directories('.')

cjson_lib = static_library('cjson',
  'cJSON.c',
  include_directories : cjson_inc,
)

cjson_dep = declare_dependency(
  include_directories : cjson_inc,
  link_with : cjson_lib,
)
