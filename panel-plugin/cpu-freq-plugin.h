/*  xfce4-cpu-freq-plugin - panel plugin for cpu informations
 *
 *  Copyright (c) 2006 Thomas Schreck <thomas.schreck@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _CPU_FREQ_H
#define _CPU_FREQ_H

#include <gtk/gtk.h>

#include <libxfce4panel/xfce-panel-plugin.h>

typedef struct
{
	gint cur_freq;
	gchar* cur_governor;
	gchar* cur_driver;

	GList* available_freqs;
	GList* available_governors;
}
CpuInfo;

typedef struct
{
	gint cpu_number;
	guint monitor_timeout;
	gboolean show_icon;
	gboolean show_label_governor;
	gboolean show_label_freq;
}
CpuFreqPluginOptions;

typedef struct
{
	XfcePanelPlugin *plugin;

	/* Array with all CPUs */
	GPtrArray *cpus;

	/* Widgets */
	GtkWidget *ebox, *icon, *label;
	GtkTooltips *tooltip;

	CpuFreqPluginOptions *options;
	
	gint timeoutHandle;
}
CpuFreqPlugin;

CpuFreqPlugin *cpuFreq;

G_BEGIN_DECLS

void
cpu_freq_widgets (void);

void
cpu_freq_update_plugin (void);

void
cpu_freq_write_config (XfcePanelPlugin *plugin);

void
cpu_freq_restart_monitor_timeout_handle (void);

G_END_DECLS

#endif /* _CPU_FREQ_H */
