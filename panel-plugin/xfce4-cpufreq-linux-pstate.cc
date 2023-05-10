/*  xfce4-cpu-freq-plugin - panel plugin for cpu informations
 *
 *  Copyright (c) 2018 Andre Miranda <andreldm@xfce.org>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "xfce4-cpufreq-plugin.h"
#include "xfce4-cpufreq-linux-pstate.h"
#include "xfce4-cpufreq-linux-sysfs.h"

#define PSTATE_BASE "/sys/devices/system/cpu/intel_pstate"

static bool
read_params ()
{
  if (g_file_test (PSTATE_BASE, G_FILE_TEST_EXISTS))
  {
    auto ips = xfce4::make<IntelPState>();

    /* These attributes will not be exposed if the
     * intel_pstate=per_cpu_perf_limits argument
     * is present in the kernel command line.
     */
    if (g_file_test (PSTATE_BASE "/min_perf_pct", G_FILE_TEST_EXISTS))
    {
      cpufreq_sysfs_read_uint (PSTATE_BASE "/min_perf_pct", &ips->min_perf_pct);
      cpufreq_sysfs_read_uint (PSTATE_BASE "/max_perf_pct", &ips->max_perf_pct);
    }
    cpufreq_sysfs_read_uint (PSTATE_BASE "/hwp_dynamic_boost", &ips->hwp_dynamic_boost);
    cpufreq_sysfs_read_uint (PSTATE_BASE "/no_turbo", &ips->no_turbo);
    cpufreq_sysfs_read_string (PSTATE_BASE "/status", ips->status);

    cpuFreq->intel_pstate = ips;
    return true;
  }
  else
  {
    cpuFreq->intel_pstate = nullptr;
    return false;
  }
}



bool
cpufreq_pstate_is_available ()
{
  return g_file_test (PSTATE_BASE, G_FILE_TEST_EXISTS);
}



bool
cpufreq_pstate_read ()
{
  /* gather intel pstate parameters */
  if (!read_params ())
    return false;

  /* now read the number of cpus and the remaining cpufreq info
     for each of them from sysfs */
  if (!cpufreq_sysfs_read ())
    return false;

  return true;
}



void
cpufreq_pstate_read_current ()
{
  /* gather intel pstate parameters */
  read_params ();

  /* now read remaining cpufreq info
     for each of them from sysfs */
  cpufreq_sysfs_read_current ();
}
