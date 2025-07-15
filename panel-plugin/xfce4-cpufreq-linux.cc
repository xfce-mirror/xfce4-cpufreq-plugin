/*  xfce4-cpu-freq-plugin - panel plugin for cpu informations
 *
 *  Copyright (c) 2006 Thomas Schreck <shrek@xfce.org>
 *  Copyright (c) 2010,2011 Florian Rivoal <frivoal@xfce.org>
 *  Copyright (c) 2013 Harald Judt <h.judt@gmx.at>
 *  Copyright (c) 2022 Jan Ziak <0xe2.0x9a.0x9b@xfce.org>
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

#include <dirent.h>
#include <math.h>
#include <stdlib.h>

#include <libxfce4ui/libxfce4ui.h>

#include "xfce4-cpufreq-plugin.h"
#include "xfce4-cpufreq-linux.h"
#include "xfce4-cpufreq-linux-procfs.h"
#include "xfce4-cpufreq-linux-pstate.h"
#include "xfce4-cpufreq-linux-sysfs.h"



bool
cpufreq_linux_init ()
{
  if (cpufreq_sysfs_is_available ())
    return cpufreq_sysfs_read ();

  if (cpufreq_pstate_is_available ())
  {
    bool ret = cpufreq_pstate_read ();

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
      cpuFreq->options->show_warning = false;
    }

    return ret;
  }

  if (cpufreq_procfs_is_available ())
    return cpufreq_procfs_read ();

  if (cpuFreq->options->show_warning)
  {
    xfce_dialog_show_warning (NULL, NULL,
      _("Your system does not support cpufreq.\nThe plugin only shows the current cpu frequency"));
    cpuFreq->options->show_warning = false;
  }

  return cpufreq_procfs_read_cpuinfo ();
}



void
cpufreq_update_cpus ()
{
  if (G_UNLIKELY (cpuFreq == nullptr))
    return;

  if (cpufreq_sysfs_is_available ())
  {
    cpufreq_sysfs_read_current ();
  }
  else if (cpufreq_procfs_is_available ())
  {
    /* First we delete the cpus and then read the /proc/cpufreq file again */
    cpuFreq->cpus.clear();
    cpufreq_procfs_read ();
  }
  else
  {
    /* We do not need to update, because no scaling available */
    return;
  }

  for (const Ptr<CpuInfo> &cpu : cpuFreq->cpus)
  {
    guint cur_freq;
    {
      std::lock_guard<std::mutex> guard(cpu->mutex);
      cur_freq = cpu->shared.cur_freq;
    }

    cpu->max_freq_measured = MAX (cpu->max_freq_measured, cur_freq);

    gint bin = round ((cur_freq - FREQ_HIST_MIN) * ((gdouble) FREQ_HIST_BINS / (FREQ_HIST_MAX - FREQ_HIST_MIN)));
    if (G_UNLIKELY (bin < 0))
      bin = 0;
    if (G_UNLIKELY (bin >= FREQ_HIST_BINS))
      bin = FREQ_HIST_BINS - 1;

    if (G_UNLIKELY (cpuFreq->freq_hist[bin] == G_MAXUINT16))
    {
      // Divide all bin counts by 2
      for (gsize j = 0; j < G_N_ELEMENTS (cpuFreq->freq_hist); j++)
        cpuFreq->freq_hist[j] /= 2;
    }
    cpuFreq->freq_hist[bin]++;
  }

  cpufreq_update_plugin (false);
}
