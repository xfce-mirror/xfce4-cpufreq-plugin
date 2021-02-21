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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <dirent.h>

#include <libxfce4ui/libxfce4ui.h>

#include "xfce4-cpufreq-plugin.h"
#include "xfce4-cpufreq-linux.h"
#include "xfce4-cpufreq-linux-procfs.h"
#include "xfce4-cpufreq-linux-pstate.h"
#include "xfce4-cpufreq-linux-sysfs.h"



gboolean
cpufreq_linux_init (void)
{
  if (cpuFreq->cpus == NULL)
    return FALSE;

  if (cpufreq_sysfs_is_available ())
    return cpufreq_sysfs_read ();

  if (cpufreq_pstate_is_available ())
  {
    gboolean ret = cpufreq_pstate_read ();

    /* Tools like i7z show the current real frequency using the
       current maximum performance. Assuming this is the proper
       way to do it, let's choose the maximum per default. Most
       CPUs nowadays have more than one core anyway, so there will
       not be much use in showing a single core's performance
       value. Besides, it's not very likely the user wants to
       follow values for 4 or 8 cores per second. */
    if (ret && cpuFreq->options->show_warning)
    {
      cpuFreq->options->show_cpu = CPU_DEFAULT;
      cpuFreq->options->show_warning = FALSE;
    }

    return ret;
  }

  if (cpufreq_procfs_is_available ())
    return cpufreq_procfs_read ();

  if (cpuFreq->options->show_warning)
  {
    xfce_dialog_show_warning (NULL, NULL,
      _("Your system does not support cpufreq.\nThe plugin only shows the current cpu frequency"));
    cpuFreq->options->show_warning = FALSE;
  }

  return cpufreq_procfs_read_cpuinfo ();
}



gboolean
cpufreq_update_cpus (gpointer data)
{
  guint  i;

  if (G_UNLIKELY (cpuFreq == NULL))
    return FALSE;

  if (cpufreq_sysfs_is_available ())
  {
    for (guint i = 0; i < cpuFreq->cpus->len; i++)
      cpufreq_sysfs_read_current (i);
  }
  else if (cpufreq_procfs_is_available ())
  {
    /* First we delete the cpus and then read the /proc/cpufreq file again */
    while (cpuFreq->cpus->len != 0)
    {
      CpuInfo *cpu = g_ptr_array_index (cpuFreq->cpus, 0);
      g_ptr_array_remove_index_fast (cpuFreq->cpus, 0);
      cpuinfo_free (cpu);
    }
    cpufreq_procfs_read ();
  }
  else
  {
    /* We do not need to update, because no scaling available */
    return FALSE;
  }

  for (i = 0; i < cpuFreq->cpus->len; i++)
    {
      CpuInfo *cpu = g_ptr_array_index (cpuFreq->cpus, i);
      cpu->max_freq_measured = MAX (cpu->max_freq_measured, cpu->cur_freq);
    }

  return cpufreq_update_plugin (FALSE);
}
