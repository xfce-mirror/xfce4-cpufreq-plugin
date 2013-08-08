AM_CPPFLAGS= 								\
	-I$(top_srcdir)							\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"

plugindir = $(libdir)/xfce4/panel/plugins

plugin_LTLIBRARIES = libcpufreq.la

libcpufreq_la_CFLAGS =						\
	$(LIBXFCE4PANEL_CFLAGS)					\
	$(LIBXFCE4UTIL_CFLAGS)					\
	$(LIBXFCE4UI_CFLAGS)

libcpufreq_la_LIBADD =						\
	$(LIBXFCE4PANEL_LIBS)					\
	$(LIBXFCE4UTIL_LIBS)					\
	$(LIBXFCE4UI_LIBS)

libcpufreq_la_SOURCES =						\
	xfce4-cpufreq-plugin.h					\
	xfce4-cpufreq-plugin.c					\
	xfce4-cpufreq-linux.h					\
	xfce4-cpufreq-linux.c					\
	xfce4-cpufreq-configure.h				\
	xfce4-cpufreq-configure.c				\
	xfce4-cpufreq-overview.h				\
	xfce4-cpufreq-overview.c				\
	xfce4-cpufreq-utils.h					\
	xfce4-cpufreq-utils.c

libcpugraph_la_LDFLAGS = \
	-avoid-version \
	-module \
	-no-undefined \
	-export-symbols-regex '^xfce_panel_module_(preinit|init|construct)' \
	$(PLATFORM_LDFLAGS)

desktopdir = $(datadir)/xfce4/panel/plugins
desktop_in_files = cpufreq.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = 								\
	$(desktop_in_files)

DISTCLEANFILES =								\
	$(desktop_DATA)
