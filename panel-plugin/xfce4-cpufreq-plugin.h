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
#include <string>
#include <vector>
#include "xfce4++/util.h"

using xfce4::Ptr;
using xfce4::Ptr0;

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
  guint  cur_freq = 0;  /* frequency in kHz */
  guint  max_freq_measured = 0;
  guint  max_freq_nominal = 0;
  guint  min_freq = 0;

  std::string cur_governor;
  std::string scaling_driver;

  std::vector<guint> available_freqs;
  std::vector<std::string> available_governors;

  bool online = false;
};

struct IntelPState
{
  guint min_perf_pct = 0;
  guint max_perf_pct = 0;
  guint no_turbo = 0;
};

struct CpuFreqPluginOptions
{
  guint       timeout = 1;             /* time between refresh */
  gint        show_cpu = CPU_DEFAULT;  /* cpu number in panel, or CPU_MIN/AVG/MAX */
  bool        show_icon = true;
  bool        show_label_freq = true;
  bool        show_label_governor = true;
  bool        show_warning = true;
  bool        keep_compact = false;
  bool        one_line = false;
  bool        icon_color_freq = false;
  std::string fontname;
  std::string fontcolor;
  CpuFreqUnit unit = UNIT_DEFAULT;
};

struct CpuFreqPlugin
{
  XfcePanelPlugin *const plugin;
  XfcePanelPluginMode panel_mode = XFCE_PANEL_PLUGIN_MODE_HORIZONTAL;
  gint panel_size = 0;
  gint panel_rows = 0;

  /* Array with all CPUs */
  std::vector<Ptr<CpuInfo>> cpus;

  /* Calculated values */
  Ptr0<CpuInfo> cpu_min;
  Ptr0<CpuInfo> cpu_avg;
  Ptr0<CpuInfo> cpu_max;

  /* Intel P-State parameters */
  Ptr0<IntelPState> intel_pstate;

  /* Widgets */
  GtkWidget *button = nullptr;
  GtkWidget *box = nullptr;
  GtkWidget *icon = nullptr;
  struct {
    GtkWidget            *draw_area = nullptr;
    PangoFontDescription *font_desc = nullptr;
    bool                  reset_size = false;
    std::string           text;
  } label;
  bool layout_changed;

  GdkPixbuf *base_icon = nullptr;
  GdkPixbuf *current_icon_pixmap = nullptr;
  GdkPixbuf *icon_pixmaps[32] = {};  /* table with frequency color coded pixbufs */

  /* Histogram of measured frequencies:
   *  min: FREQ_HIST_MIN
   *  max: FREQ_HIST_MAX
   *  range: max - min
   *  resolution: range / FREQ_HIST_BINS = 62.5 MHz */
  guint16 freq_hist[FREQ_HIST_BINS] = {};

  const Ptr<CpuFreqPluginOptions> options = xfce4::make<CpuFreqPluginOptions>();

  gint timeoutHandle = 0;

  CpuFreqPlugin(XfcePanelPlugin *plugin);
  ~CpuFreqPlugin();

  void destroy_icons();
  void set_font(const std::string &fontname_orEmpty);
};

extern CpuFreqPlugin *cpuFreq;

void
cpufreq_prepare_label ();

void
cpufreq_restart_timeout ();

void
cpufreq_update_icon ();

bool
cpufreq_update_plugin (bool reset_label_size);

void
cpufreq_write_config (XfcePanelPlugin *plugin);

#endif /* XFCE4_CPU_FREQ_H */
