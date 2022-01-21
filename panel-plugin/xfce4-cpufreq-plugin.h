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

#define FREQ_HIST_BINS 128           /* number of bins */
#define FREQ_HIST_MAX  (8*1000*1000) /* frequency in kHz */
#define FREQ_HIST_MIN  0             /* frequency in kHz */

enum CpuFreqUnit
{
  UNIT_AUTO,
  UNIT_GHZ,
  UNIT_MHZ,
};

#define UNIT_DEFAULT UNIT_GHZ

struct CpuInfo
{
  guint  cur_freq;  /* frequency in kHz */
  guint  max_freq_measured;
  guint  max_freq_nominal;
  guint  min_freq;
  gchar  *cur_governor;
  gchar  *scaling_driver;

  GList *available_freqs;
  GList *available_governors;

  gboolean online;
};

struct IntelPState
{
  guint min_perf_pct;
  guint max_perf_pct;
  guint no_turbo;
};

struct CpuFreqPluginOptions
{
  guint       timeout;       /* time between refresh */
  gint        show_cpu;      /* cpu number in panel, or CPU_MIN/AVG/MAX */
  gboolean    show_icon:1;
  gboolean    show_label_governor:1;
  gboolean    show_label_freq:1;
  gboolean    show_warning:1;
  gboolean    keep_compact:1;
  gboolean    one_line:1;
  gboolean    icon_color_freq:1;
  gchar      *fontname;
  gchar      *fontcolor;
  CpuFreqUnit unit;
};

struct CpuFreqPlugin
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
  GtkWidget *button, *box, *icon;
  struct {
    GtkWidget            *draw_area;
    PangoFontDescription *font_desc;
    gboolean              reset_size;
    gchar                *text;
  } label;
  gboolean layout_changed;

  GdkPixbuf *base_icon;
  GdkPixbuf *current_icon_pixmap;
  GdkPixbuf *icon_pixmaps[32];  /* table with frequency color coded pixbufs */

  /* Histogram of measured frequencies:
   *  min: FREQ_HIST_MIN
   *  max: FREQ_HIST_MAX
   *  range: max - min
   *  resolution: range / FREQ_HIST_BINS = 62.5 MHz */
  guint16 freq_hist[FREQ_HIST_BINS];

  CpuFreqPluginOptions *options;
  gint timeoutHandle;
};

extern CpuFreqPlugin *cpuFreq;

void
cpufreq_prepare_label ();

void
cpufreq_restart_timeout ();

void
cpufreq_set_font (const gchar *fontname_or_null);

void
cpufreq_update_icon ();

gboolean
cpufreq_update_plugin (gboolean reset_label_size);

void
cpufreq_write_config (XfcePanelPlugin *plugin);

void
cpuinfo_free (CpuInfo *cpu);

#endif /* XFCE4_CPU_FREQ_H */
