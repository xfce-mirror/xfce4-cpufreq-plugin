dnl
dnl Copyright (c) 2006-2022 The Xfce development team
dnl

dnl Version information
AC_CONFIG_MACRO_DIRS([m4])
XDT_VERSION_INIT([1.2.8], [git])

dnl Initialize autoconf
AC_COPYRIGHT([Copyright (c) 2006 Thomas Schreck <shrek@xfce.org>
              Copyright (c) 2010-2011 Florian Rivoal <frivoal@xfce.org>
              Copyright (c) 2013-2015 Harald Judt <h.judt@gmx.at>
              Copyright (c) 2016-2020 Andre Miranda <andreldm@xfce.org>
              Copyright (c) 2021-2024 The Xfce development team. All rights reserved.])
AC_INIT([xfce4-cpufreq-plugin],[xdt_version],[https://gitlab.xfce.org/panel-plugins/xfce4-cpufreq-plugin],[xfce4-cpufreq-plugin])
AC_REVISION([xdt_version_build])
AC_PREREQ([2.69])

dnl Initialize automake
AM_INIT_AUTOMAKE([1.11 dist-bzip2 tar-ustar no-dist-gzip foreign])
AC_CONFIG_HEADERS([config.h])
AM_MAINTAINER_MODE()
AM_SILENT_RULES([yes])

dnl Check for basic programs
AC_PROG_CC()
AC_PROG_CXX()
LT_PATH_LD([])
AC_PROG_INSTALL
AX_CXX_COMPILE_STDCXX_11()
LT_PREREQ([2.2.6])
LT_INIT([disable-static])

dnl configure the panel plugin
XDT_CHECK_PACKAGE([GLIB], [glib-2.0], [2.50.0])
XDT_CHECK_PACKAGE([GTHREAD], [gthread-2.0], [2.50.0])
XDT_CHECK_PACKAGE([GTK], [gtk+-3.0], [3.22.0])
XDT_CHECK_PACKAGE([LIBXFCE4PANEL], [libxfce4panel-2.0], [4.16.0])
XDT_CHECK_PACKAGE([LIBXFCE4UI], [libxfce4ui-2], [4.16.0])
XDT_CHECK_PACKAGE([LIBXFCE4UTIL], [libxfce4util-1.0], [4.17.2])

dnl Check for i18n support
GETTEXT_PACKAGE="$PACKAGE"
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"], [Name of default gettext domain])
AC_SUBST([GETTEXT_PACKAGE])
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.19.8])

dnl Check for debugging support
XDT_FEATURE_DEBUG([xdt_debug_default])

AC_CONFIG_FILES([
Makefile
panel-plugin/Makefile
xfce4++/Makefile
xfce4++/util/Makefile
icons/Makefile
icons/16x16/Makefile
icons/22x22/Makefile
icons/48x48/Makefile
po/Makefile.in
])
AC_OUTPUT
