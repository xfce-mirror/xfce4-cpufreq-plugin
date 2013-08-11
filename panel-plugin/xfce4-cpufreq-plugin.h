/*  xfce4-cpu-freq-plugin - panel plugin for cpu informations
 *
 *  Copyright (c) 2006 Thomas Schreck <shrek@xfce.org>
 *  Copyright (c) 2010,2011 Florian Rivoal <frivoal@xfce.org>
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef XFCE4_CPUFREQ_H
#define XFCE4_CPUFREQ_H

#include <gtk/gtk.h>
#include <libxfce4panel/xfce-panel-plugin.h>

typedef struct
{
	guint  cur_freq;
	guint  max_freq;
	guint  min_freq;
	gchar  *cur_governor;
	gchar  *scaling_driver;

	GList* available_freqs;
	GList* available_governors;
} CpuInfo;

typedef struct
{
	gint     min_perf_pct;
	gint     max_perf_pct;
	gint     no_turbo;
} IntelPState;

typedef struct
{
	guint 	 timeout;       /* time between refresh */
	guint	 show_cpu;      /* cpu number in panel */
	gboolean show_icon;
	gboolean show_label_governor;
	gboolean show_label_freq;
	gboolean show_warning;
	gboolean keep_compact;
} CpuFreqPluginOptions;

typedef struct
{
	XfcePanelPlugin *plugin;
	XfcePanelPluginMode panel_mode;

	/* Array with all CPUs */
	GPtrArray *cpus;

	/* Intel P-State parameters */
	IntelPState *intel_pstate;

	/* Widgets */
	GtkWidget *button, *box, *icon, *label;

	gint icon_size;

	CpuFreqPluginOptions  *options;
	gint 		      timeoutHandle;
} CpuFreqPlugin;

CpuFreqPlugin *cpuFreq;

G_BEGIN_DECLS

void
cpuinfo_free (CpuInfo *cpu);

gboolean
cpufreq_update_plugin (void);

void
cpufreq_restart_timeout (void);

void
cpufreq_write_config (XfcePanelPlugin *plugin);

G_END_DECLS

#endif /* XFCE4_CPU_FREQ_H */
