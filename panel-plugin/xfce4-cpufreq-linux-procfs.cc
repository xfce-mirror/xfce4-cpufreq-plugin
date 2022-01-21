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
#include "xfce4-cpufreq-linux-procfs.h"

#define PROCFS_BASE "/proc/cpufreq"



gboolean
cpufreq_procfs_is_available ()
{
  return g_file_test (PROCFS_BASE, G_FILE_TEST_EXISTS);
}



gboolean
cpufreq_procfs_read_cpuinfo ()
{
  const char *const filePath = "/proc/cpuinfo";
  FILE *file;

  if (!g_file_test (filePath, G_FILE_TEST_EXISTS))
    return FALSE;

  file = fopen (filePath, "r");

  if (file)
  {
    gchar line[256];
    guint i = 0;
    while (fgets (line, sizeof(line), file) != NULL)
    {
      if (g_ascii_strncasecmp (line, "cpu MHz", 7) == 0)
      {
        CpuInfo *cpu = NULL;
        gboolean add_cpu = FALSE;
        gchar *freq;

        if (cpuFreq->cpus && cpuFreq->cpus->len > i)
          cpu = (CpuInfo*) g_ptr_array_index (cpuFreq->cpus, i);

        if (cpu == NULL)
        {
          cpu = g_new0 (CpuInfo, 1);
          cpu->online = TRUE;
          add_cpu = TRUE;
        }

        freq = g_strrstr (line, ":");

        if (freq == NULL)
        {
          if (add_cpu)
            cpuinfo_free (cpu);
          break;
        }

        sscanf (++freq, "%d.", &cpu->cur_freq);
        cpu->cur_freq *= 1000;

        if (add_cpu)
          g_ptr_array_add (cpuFreq->cpus, cpu);

        ++i;
      }
    }

    fclose (file);
  }

  return TRUE;
}



gboolean
cpufreq_procfs_read ()
{
  FILE *file;
  gchar *filePath;

  filePath = g_strdup (PROCFS_BASE);
  if (!g_file_test (filePath, G_FILE_TEST_EXISTS))
  {
    g_free (filePath);
    return FALSE;
  }

  file = fopen (filePath, "r");

  if (file)
  {
    gchar line[256];
    while (fgets (line, sizeof(line), file) != NULL)
    {
      if (g_ascii_strncasecmp (line, "CPU", 3) == 0)
      {
        CpuInfo *cpu = g_new0 (CpuInfo, 1);
        cpu->cur_governor = g_new (gchar, 20);
        cpu->online = TRUE;

        sscanf (line,
                "CPU %*d %d kHz (%*d %%) - %d kHz (%*d %%) - %20s",
                &cpu->min_freq,
                &cpu->max_freq_nominal,
                cpu->cur_governor);
        cpu->min_freq *= 1000;
        cpu->max_freq_nominal *= 1000;

        g_ptr_array_add (cpuFreq->cpus, cpu);
      }
    }

    fclose (file);
  }

  g_free (filePath);

  for (guint i = 0; i < cpuFreq->cpus->len; i++)
  {
    auto cpu = (CpuInfo*) g_ptr_array_index (cpuFreq->cpus, i);
    filePath = g_strdup_printf ("/proc/sys/cpu/%d/speed", i);

    if (!g_file_test (filePath, G_FILE_TEST_EXISTS))
    {
      g_free (filePath);
      return FALSE;
    }

    file = fopen (filePath, "r");

    if (file)
    {
      if (fscanf (file, "%d", &cpu->cur_freq) != 1)
        cpu->cur_freq = 0;
      fclose (file);
    }

    g_free (filePath);
  }

  return TRUE;
}
