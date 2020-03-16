project('uv_http', ['c'], version: '1.0.7')

uv_http_dir = include_directories(['include', '../llhttp/include'])
uv_http_lib = static_library('uv_http',
  sources: [
    'src/uv_http_t.c',
    'src/uv_http_req_t.c',
    'src/methods.c',
    'src/req_methods.c',
    'src/utils.c',
    '../llhttp/src/api.c',
    '../llhttp/src/http.c'
  ],
  dependencies : [uv_dep],
  include_directories: [uv_http_dir, uv_link_dir])

e = executable('uv_http_example',
  sources: [
    'example/src/main.c'
  ],
  link_with: [uv_http_lib, uv_link_lib, minitest_lib],
  dependencies : [uv_dep],
  include_directories: [uv_http_dir, uv_link_dir])

t = executable('uv_http_test',
  sources: [
    'test/src/main.c',
    'test/src/test-req-no-headers.c',
    'test/src/test-req-with-headers.c',
    'test/src/test-req-respond.c',
    'test/src/test-req-write.c',
  ],
  link_with: [uv_http_lib, uv_link_lib, minitest_lib],
  dependencies : [uv_dep],
  include_directories: [uv_http_dir, uv_link_dir, minitest_dir])

test('uv_http_t test', t)

