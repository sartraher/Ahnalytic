project(
  'zlib',
  'c',
  version : '1.3',
  default_options : [
    'warning_level=0',
    'c_std=c99'
  ]
)

cc = meson.get_compiler('c')
is_windows = host_machine.system() == 'windows'

# -------------------------------------------------
# Sources (matches your vcxproj exactly)
# -------------------------------------------------
zlib_sources = files(
  'adler32.c',
  'compress.c',
  'crc32.c',
  'deflate.c',
  'gzclose.c',
  'gzlib.c',
  'gzread.c',
  'gzwrite.c',
  'inflate.c',
  'infback.c',
  'inftrees.c',
  'inffast.c',
  'trees.c',
  'uncompr.c',
  'zutil.c'
)

# -------------------------------------------------
# Include directories
# -------------------------------------------------
zlib_inc = include_directories(
  '.',
  'build'   # for zconf.h
)

# -------------------------------------------------
# Preprocessor definitions
# -------------------------------------------------
zlib_defs = [
  'NO_FSEEKO',
  '_CRT_SECURE_NO_DEPRECATE',
  '_CRT_NONSTDC_NO_DEPRECATE'
]

if is_windows
  zlib_defs += ['WIN32', '_WINDOWS']
endif

# -------------------------------------------------
# Compiler arguments
# -------------------------------------------------
zlib_c_args = []

if cc.get_id() == 'msvc'
  # MSVC matches vcxproj behavior
  zlib_c_args += ['/utf-8']
else
  # Unix-like platforms
  zlib_c_args += ['-fPIC']
endif

# -------------------------------------------------
# Static library
# -------------------------------------------------
zlib_lib = static_library(
  'zlibstatic',
  zlib_sources,
  include_directories : zlib_inc,
  c_args : zlib_c_args,
  c_defines : zlib_defs,
  install : false
)

# -------------------------------------------------
# Export dependency for parent projects
# -------------------------------------------------
zlib_dep = declare_dependency(
  link_with : zlib_lib,
  include_directories : zlib_inc,
  compile_args : zlib_c_args,
  compile_definitions : zlib_defs
)
