dnl configure.ac
dnl
dnl xfce4-cpu-freq-plugin - Plugin shows Infos about Frequency and Governour of
dnl                         CPUs
dnl

m4_define([xfce4_cpu_freq_plugin_version], [0.0.1])

AC_INIT([xfce4-cpu-freq-plugin], [xfce4_cpu_freq_plugin_version()],
	[thomas.schreck@gmail.com])

CPU_FREQ_PLUGIN_VERSION=xfce4_cpu_freq_plugin_version()
AM_INIT_AUTOMAKE([xfce4-cpu-freq-plugin], [$CPU_FREQ_PLUGIN_VERSION])
AM_CONFIG_HEADER([config.h])

AM_MAINTAINER_MODE

dnl Check for basic programs
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_PROG_INTLTOOL()

dnl Check for standard header files
AC_HEADER_STDC

dnl configure the panel plugin
XDT_CHECK_PACKAGE([GTHREAD], [gthread-2.0], [2.6.0])
XDT_CHECK_PACKAGE([GTK], [gtk+-2.0], [2.6.0])
XDT_CHECK_PACKAGE([LIBXFCE4UTIL], [libxfce4util-1.0], [4.3.90])
XDT_CHECK_PACKAGE([LIBXFCEGUI4], [libxfcegui4-1.0], [4.3.90])
XDT_CHECK_PACKAGE([LIBXFCE4PANEL], [libxfce4panel-1.0], [4.3.90])

dnl Translations
XDT_I18N([])

dnl Check for debugging support
XDT_FEATURE_DEBUG()

AC_OUTPUT([
Makefile
panel-plugin/Makefile
icons/Makefile
icons/16x16/Makefile
icons/22x22/Makefile
icons/48x48/Makefile
icons/scalable/Makefile
po/Makefile.in
])
