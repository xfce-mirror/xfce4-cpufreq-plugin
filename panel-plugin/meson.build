plugin_sources = [
  'plugin.c',
  'plugin.h',
  'xfce4-cpufreq-configure.cc',
  'xfce4-cpufreq-configure.h',
  'xfce4-cpufreq-linux-procfs.cc',
  'xfce4-cpufreq-linux-procfs.h',
  'xfce4-cpufreq-linux-pstate.cc',
  'xfce4-cpufreq-linux-pstate.h',
  'xfce4-cpufreq-linux-sysfs.cc',
  'xfce4-cpufreq-linux-sysfs.h',
  'xfce4-cpufreq-linux.cc',
  'xfce4-cpufreq-linux.h',
  'xfce4-cpufreq-overview.cc',
  'xfce4-cpufreq-overview.h',
  'xfce4-cpufreq-plugin.cc',
  'xfce4-cpufreq-plugin.h',
  'xfce4-cpufreq-utils.cc',
  'xfce4-cpufreq-utils.h',
  xfce_revision_h,
]

plugin_install_subdir = 'xfce4' / 'panel' / 'plugins'

plugin_lib = shared_library(
  'cpufreq',
  plugin_sources,
  gnu_symbol_visibility: 'hidden',
  c_args: [
    '-DG_LOG_DOMAIN="@0@"'.format('xfce4-cpufreq-plugin'),
  ],
  include_directories: [
    include_directories('..'),
  ],
  dependencies: [
    glib,
    gtk,
    libxfce4panel,
    libxfce4ui,
    libxfce4util,
  ],
  link_with: [
    libxfce4util_pp,
  ],
  install: true,
  install_dir: get_option('prefix') / get_option('libdir') / plugin_install_subdir,
)

i18n.merge_file(
  input: 'cpufreq.desktop.in',
  output: 'cpufreq.desktop',
  po_dir: '..' / 'po',
  type: 'desktop',
  install: true,
  install_dir: get_option('prefix') / get_option('datadir') / plugin_install_subdir,
)
