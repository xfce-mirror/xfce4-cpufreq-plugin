/*  xfce4-cpu-freq-plugin - panel plugin for cpu informations
 *
 *  Copyright (c) 2018 Andre Miranda <andreldm@xfce.org>
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

#include "xfce4-cpufreq-plugin.h"
#include "xfce4-cpufreq-linux-pstate.h"
#include "xfce4-cpufreq-linux-sysfs.h"

#ifndef _
# include <libintl.h>
# define _(String) gettext (String)
#endif

#define PSTATE_BASE "/sys/devices/system/cpu/intel_pstate"

static gboolean read_params ();

gboolean
cpufreq_pstate_is_available ()
{
  return g_file_test (PSTATE_BASE, G_FILE_TEST_EXISTS);
}



gboolean
cpufreq_pstate_read (void)
{
  CpuInfo *cpu;
  gint i;

  /* gather intel pstate parameters */
  if (!read_params ())
    return FALSE;

  /* now read the number of cpus and the remaining cpufreq info
     for each of them from sysfs */
  if (!cpufreq_sysfs_read ())
    return FALSE;

  return TRUE;
}



static gboolean
read_params ()
{
  gchar *file, *contents;
  IntelPState *ips;

  ips = g_slice_new0(IntelPState);

  if (!g_file_test (PSTATE_BASE, G_FILE_TEST_EXISTS))
    return FALSE;

  file = g_strdup (PSTATE_BASE "/min_perf_pct");
  cpufreq_sysfs_read_int (file, contents, &ips->min_perf_pct);
  g_free (file);

  file = g_strdup (PSTATE_BASE "/max_perf_pct");
  cpufreq_sysfs_read_int (file, contents, &ips->max_perf_pct);
  g_free (file);

  file = g_strdup (PSTATE_BASE "/no_turbo");
  cpufreq_sysfs_read_int (file, contents, &ips->no_turbo);
  g_free (file);

  g_slice_free (IntelPState, cpuFreq->intel_pstate);
  cpuFreq->intel_pstate = ips;

  return TRUE;
}
