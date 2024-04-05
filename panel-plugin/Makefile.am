plugindir = $(libdir)/xfce4/panel/plugins
plugin_LTLIBRARIES = libcpufreq.la

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"

libcpufreq_la_CFLAGS = \
	@LIBXFCE4PANEL_CFLAGS@ \
	@LIBXFCE4UI_CFLAGS@ \
	@LIBXFCE4UTIL_CFLAGS@

libcpufreq_la_CXXFLAGS = $(libcpufreq_la_CFLAGS)

libcpufreq_la_LIBADD = \
	$(top_builddir)/xfce4++/util/libxfce4util_pp.la \
	@LIBXFCE4PANEL_LIBS@ \
	@LIBXFCE4UI_LIBS@ \
	@LIBXFCE4UTIL_LIBS@ \
	-lm

libcpufreq_la_SOURCES = \
	plugin.c \
	plugin.h \
	xfce4-cpufreq-configure.cc \
	xfce4-cpufreq-configure.h \
	xfce4-cpufreq-linux-procfs.cc \
	xfce4-cpufreq-linux-procfs.h \
	xfce4-cpufreq-linux-pstate.cc \
	xfce4-cpufreq-linux-pstate.h \
	xfce4-cpufreq-linux-sysfs.cc \
	xfce4-cpufreq-linux-sysfs.h \
	xfce4-cpufreq-linux.cc \
	xfce4-cpufreq-linux.h \
	xfce4-cpufreq-overview.cc \
	xfce4-cpufreq-overview.h \
	xfce4-cpufreq-plugin.cc \
	xfce4-cpufreq-plugin.h \
	xfce4-cpufreq-utils.cc \
	xfce4-cpufreq-utils.h

libcpufreq_la_LDFLAGS = \
	-avoid-version \
	-module \
	-no-undefined \
	-export-symbols-regex '^xfce_panel_module_(preinit|init|construct)' \
	$(PLATFORM_LDFLAGS)

SUFFIXES = .desktop .desktop.in

desktopdir = $(datadir)/xfce4/panel/plugins
desktop_in_files = cpufreq.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

.desktop.in.desktop:
	$(AM_V_GEN)$(MSGFMT) --desktop --template $< -d $(top_srcdir)/po -o $@

EXTRA_DIST = $(desktop_in_files)

DISTCLEANFILES = $(desktop_DATA)
