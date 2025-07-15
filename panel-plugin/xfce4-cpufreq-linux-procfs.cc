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

#include "xfce4-cpufreq-plugin.h"
#include "xfce4-cpufreq-linux-procfs.h"

#define PROCFS_BASE "/proc/cpufreq"



bool
cpufreq_procfs_is_available ()
{
  return g_file_test (PROCFS_BASE, G_FILE_TEST_EXISTS);
}



bool
cpufreq_procfs_read_cpuinfo ()
{
  const char *const filePath = "/proc/cpuinfo";

  if (!g_file_test (filePath, G_FILE_TEST_EXISTS))
    return false;

  FILE *file = fopen (filePath, "r");

  if (file)
  {
    gchar line[256];
    guint i = 0;
    while (fgets (line, sizeof(line), file) != NULL)
    {
      if (g_ascii_strncasecmp (line, "cpu MHz", 7) == 0)
      {
        Ptr0<CpuInfo> cpu;
        bool add_cpu = false;

        if (i < cpuFreq->cpus.size())
          cpu = cpuFreq->cpus[i];

        if (cpu == nullptr)
        {
          cpu = xfce4::make<CpuInfo>();
          {
            std::lock_guard<std::mutex> guard(cpu->mutex);
            cpu->shared.online = true;
          }
          add_cpu = true;
        }

        gchar *freq = g_strrstr (line, ":");
        if (freq == NULL)
          break;

        {
            std::lock_guard<std::mutex> guard(cpu->mutex);
            sscanf (++freq, "%d.", &cpu->shared.cur_freq);
            cpu->shared.cur_freq *= 1000;
        }

        if (add_cpu)
          cpuFreq->cpus.push_back(cpu.toPtr());

        ++i;
      }
    }

    fclose (file);
  }

  return true;
}



bool
cpufreq_procfs_read ()
{
  std::string filePath = PROCFS_BASE;

  if (!g_file_test (filePath.c_str(), G_FILE_TEST_EXISTS))
    return false;

  FILE *file = fopen (filePath.c_str(), "r");

  if (file)
  {
    gchar line[256];
    while (fgets (line, sizeof(line), file) != NULL)
    {
      if (g_ascii_strncasecmp (line, "CPU", 3) == 0)
      {
        auto cpu = xfce4::make<CpuInfo>();

        char gov[21];
        sscanf (line,
                "CPU %*d %d kHz (%*d %%) - %d kHz (%*d %%) - %20s",
                &cpu->min_freq,
                &cpu->max_freq_nominal,
                gov);
        cpu->min_freq *= 1000;
        cpu->max_freq_nominal *= 1000;
        gov[G_N_ELEMENTS(gov)-1] = '\0';

        {
            std::lock_guard<std::mutex> guard(cpu->mutex);
            cpu->shared.online = true;
            cpu->shared.cur_governor = gov;
        }

        cpuFreq->cpus.push_back(cpu);
      }
    }

    fclose (file);
  }

  for (size_t i = 0; i < cpuFreq->cpus.size(); i++)
  {
    const Ptr<CpuInfo> &cpu = cpuFreq->cpus[i];
    filePath = xfce4::sprintf ("/proc/sys/cpu/%zu/speed", i);

    if (!g_file_test (filePath.c_str(), G_FILE_TEST_EXISTS))
      return false;

    file = fopen (filePath.c_str(), "r");

    if (file)
    {
      guint cur_freq;
      if (fscanf (file, "%d", &cur_freq) != 1)
        cur_freq = 0;
      fclose (file);

      {
        std::lock_guard<std::mutex> guard(cpu->mutex);
        cpu->shared.cur_freq = cur_freq;
      }
    }
  }

  return true;
}
