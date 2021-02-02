/*  xfce4-cpu-freq-plugin - panel plugin for cpu informations
 *
 *  Copyright (c) 2006 Thomas Schreck <shrek@xfce.org>
 *  Copyright (c) 2010,2011 Florian Rivoal <frivoal@xfce.org>
 *  Copyright (c) 2013 Harald Judt <h.judt@gmx.at>
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
#include <libxfce4panel/libxfce4panel.h>

#define PLUGIN_WEBSITE ("https://docs.xfce.org/panel-plugins/xfce4-cpufreq-plugin")

#define CPU_MIN (-1)
#define CPU_AVG (-2)
#define CPU_MAX (-3)
#define CPU_DEFAULT CPU_MAX

typedef struct
{
  guint  cur_freq;  /* frequency in kHz */
  guint  max_freq;
  guint  min_freq;
  gchar  *cur_governor;
  gchar  *scaling_driver;

  GList* available_freqs;
  GList* available_governors;

  gboolean online;
} CpuInfo;

typedef struct
{
  guint min_perf_pct;
  guint max_perf_pct;
  guint no_turbo;
} IntelPState;

typedef struct
{
  guint    timeout;       /* time between refresh */
  gint     show_cpu;      /* cpu number in panel, or CPU_MIN/AVG/MAX */
  gboolean show_icon;
  gboolean show_label_governor;
  gboolean show_label_freq;
  gboolean show_warning;
  gboolean keep_compact;
  gboolean one_line;
  gchar   *fontname;
  gchar   *fontcolor;
} CpuFreqPluginOptions;

typedef struct
{
  XfcePanelPlugin *plugin;
  XfcePanelPluginMode panel_mode;
  gint panel_size;
  gint panel_rows;

  /* Array with all CPUs */
  GPtrArray *cpus;

  /* Calculated values */
  CpuInfo *cpu_min;
  CpuInfo *cpu_avg;
  CpuInfo *cpu_max;

  /* Intel P-State parameters */
  IntelPState *intel_pstate;

  /* Widgets */
  GtkWidget *button, *box, *icon, *label_orNull;
  GtkCssProvider *label_css_provider;
  gboolean layout_changed;
  gint label_max_width;

  CpuFreqPluginOptions *options;
  gint timeoutHandle;
} CpuFreqPlugin;

extern CpuFreqPlugin *cpuFreq;

G_BEGIN_DECLS

void
cpuinfo_free (CpuInfo *cpu);

gboolean
cpufreq_update_plugin (gboolean reset_label_size);

void
cpufreq_restart_timeout (void);

void
cpufreq_write_config (XfcePanelPlugin *plugin);

void
cpufreq_update_icon (CpuFreqPlugin *cpufreq);

G_END_DECLS

#endif /* XFCE4_CPU_FREQ_H */
