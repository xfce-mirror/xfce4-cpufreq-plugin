plugindir = $(libexecdir)/xfce4/panel-plugins
plugin_PROGRAMS = xfce4-cpu-freq-plugin

xfce4_cpu_freq_plugin_CFLAGS =					\
	-I$(top_srcdir)						\
	@LIBXFCE4PANEL_CFLAGS@					\
	@LIBXFCE4UTIL_CFLAGS@					\
	@LIBXFCEGUI4_CFLAGS@					\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"

xfce4_cpu_freq_plugin_LDFLAGS =					\
	@LIBXFCE4PANEL_LIBS@					\
	@LIBXFCE4UTIL_LIBS@					\
	@LIBXFCEGUI4_LIBS@					

xfce4_cpu_freq_plugin_SOURCES =					\
	cpu-freq-plugin.h					\
	cpu-freq-plugin.c					\
	cpu-freq-dialogs.h					\
	cpu-freq-dialogs.c					\
	cpu-freq-monitor.h					\
	cpu-freq-monitor.c					\
	cpu-freq-overview.h					\
	cpu-freq-overview.c
	
desktop_in_in_files = xfce4-cpu-freq-plugin.desktop.in.in
desktop_in_files = $(desktop_in_in_files:.desktop.in.in=.desktop.in)
	
desktopdir = $(datadir)/xfce4/panel-plugins
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = 								\
	$(desktop_in_in_files)

DISTCLEANFILES =							\
	$(desktop_DATA) $(desktop_in_files)
	
# get full path into .desktop file
%.desktop.in: %.desktop.in.in
	sed -e "s^@PLUGIN_PATH@^$(libexecdir)/xfce4/panel-plugins^"	\
		$< > $@

