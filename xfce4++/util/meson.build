util_sources = [
  'async.cc',
  'async.h',
  'gtk.cc',
  'gtk.h',
  'memory.cc',
  'memory.h',
  'optional.h',
  'rc.cc',
  'rc.h',
  'string-utils.cc',
  'string-utils.h',
]

libxfce4util_pp = static_library(
  'xfce4util_pp',
  util_sources,
  include_directories: [
    include_directories('..' / '..'),
  ],
  dependencies: [
    glib,
    gtk,
    libxfce4panel,
    libxfce4util,
  ],
  install: false,
)
