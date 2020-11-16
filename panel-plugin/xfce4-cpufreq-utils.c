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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libxfce4ui/libxfce4ui.h>
#include <stdlib.h>
#include "xfce4-cpufreq-plugin.h"
#include "xfce4-cpufreq-utils.h"



gchar*
cpufreq_get_human_readable_freq (guint freq)
{
  guint div;
  const gchar *freq_unit;
  gchar *readable_freq;

  if (freq > 999999)
  {
    div = 1000 * 1000;
    freq_unit = "GHz";
  }
  else
  {
    div = 1000;
    freq_unit = "MHz";
  }
  
  if (div == 1000)
  {
    guint rounded_freq = (freq + div/2) / div;
    readable_freq = g_strdup_printf ("%u %s", rounded_freq, freq_unit);
  }
  else
    readable_freq = g_strdup_printf ("%3.2f %s", (gfloat) freq / div, freq_unit);

  return readable_freq;
}



guint
cpufreq_get_normal_freq (const gchar *freq)
{
  guint result;
  gchar **tokens;

  tokens = g_strsplit (freq, " ", 0);

  if (g_ascii_strcasecmp (tokens[1], "GHz") == 0)
    result = (guint) (atof (tokens[0]) * 1000 * 1000);
  else
    result = (guint) (atof (tokens[0]) * 1000);

  g_strfreev (tokens);

  return result;
}



void
cpufreq_warn_reset ()
{
  xfce_dialog_show_warning (NULL, NULL,
    _("The CPU displayed by the XFCE cpufreq plugin has been reset to a default value"));
}
