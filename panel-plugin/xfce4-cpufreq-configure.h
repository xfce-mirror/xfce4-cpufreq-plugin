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

#ifndef XFCE4_CPUFREQ_CONFIGURE_H
#define XFCE4_CPUFREQ_CONFIGURE_H

typedef struct
{
  GtkWidget *display_cpu;
  GtkWidget *display_icon;
  GtkWidget *display_freq;
  GtkWidget *display_governor;
  GtkWidget *icon_color_freq;
  GtkWidget *monitor_timeout;
  GtkWidget *combo_cpu;
  GtkWidget *combo_unit;
  GtkWidget *spinner_timeout;
  GtkWidget *keep_compact;
  GtkWidget *one_line;
  GtkWidget *fontcolor, *fontcolor_hbox;
  GtkWidget *fontname, *fontname_hbox;
} CpuFreqPluginConfigure;

G_BEGIN_DECLS

void
cpufreq_configure (XfcePanelPlugin *plugin);

#define TIMEOUT_MIN	1
#define TIMEOUT_MAX	10
#define TIMEOUT_STEP	1

G_END_DECLS

#endif /* XFCE4_CPUFREQ_CONFIGURE_H */
