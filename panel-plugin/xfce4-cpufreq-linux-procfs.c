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
#include "xfce4-cpufreq-linux-procfs.h"

#define PROCFS_BASE "/proc/cpufreq"



gboolean
cpufreq_procfs_is_available (void)
{
  return g_file_test (PROCFS_BASE, G_FILE_TEST_EXISTS);
}



gboolean
cpufreq_procfs_read_cpuinfo (void)
{
  CpuInfo *cpu;
  FILE *file;
  gchar *freq, *filePath, *fileContent;
  guint i = 0;
  gboolean add_cpu;

  filePath = g_strdup ("/proc/cpuinfo");
  if (!g_file_test (filePath, G_FILE_TEST_EXISTS))
  {
    g_free (filePath);
    return FALSE;
  }

  file = fopen (filePath, "r");

  if (file)
  {
    fileContent = g_new (gchar,255);
    while (fgets (fileContent, 255, file) != NULL)
    {
      if (g_ascii_strncasecmp (fileContent, "cpu MHz", 7) == 0)
      {
        cpu = NULL;
        add_cpu = FALSE;

        if (cpuFreq->cpus && cpuFreq->cpus->len > i)
          cpu = g_ptr_array_index (cpuFreq->cpus, i);

        if (cpu == NULL)
        {
          cpu = g_new0 (CpuInfo, 1);
          cpu->max_freq = 0;
          cpu->min_freq = 0;
          cpu->cur_governor = NULL;
          cpu->available_freqs = NULL;
          cpu->available_governors = NULL;
          cpu->online = TRUE;
          add_cpu = TRUE;
        }

        freq = g_strrstr (fileContent, ":");

        if (freq == NULL)
        {
          if (add_cpu)
            cpuinfo_free (cpu);
          break;
        }

        sscanf (++freq, "%d.", &cpu->cur_freq);
        cpu->cur_freq *= 1000;

        if (add_cpu && cpu != NULL)
          g_ptr_array_add (cpuFreq->cpus, cpu);

        ++i;
      }
    }

    fclose (file);
    g_free (fileContent);
  }

  g_free (filePath);

  return TRUE;
}



gboolean
cpufreq_procfs_read (void)
{
  CpuInfo *cpu;
  FILE *file;
  gchar *filePath, *fileContent;

  filePath = g_strdup (PROCFS_BASE);
  if (!g_file_test (filePath, G_FILE_TEST_EXISTS))
  {
    g_free (filePath);
    return FALSE;
  }

  file = fopen (filePath, "r");

  if (file)
  {
    fileContent = g_new (gchar, 255);
    while (fgets (fileContent, 255, file) != NULL)
    {
      if (g_ascii_strncasecmp (fileContent, "CPU", 3) == 0)
      {
        cpu = g_new0 (CpuInfo, 1);
        cpu->max_freq = 0;
        cpu->min_freq = 0;
        cpu->cur_governor = g_new (gchar, 20);
        cpu->available_freqs = NULL;
        cpu->available_governors = NULL;
        cpu->online = TRUE;

        sscanf (fileContent,
                "CPU %*d %d kHz (%*d %%) - %d kHz (%*d %%) - %20s",
                &cpu->min_freq,
                &cpu->max_freq,
                cpu->cur_governor);
        cpu->min_freq *= 1000;
        cpu->max_freq *= 1000;

        g_ptr_array_add (cpuFreq->cpus, cpu);
      }
    }

    fclose (file);
    g_free (fileContent);
  }

  g_free (filePath);

  for (guint i = 0; i < cpuFreq->cpus->len; i++)
  {
    cpu = g_ptr_array_index (cpuFreq->cpus, i);
    filePath = g_strdup_printf ("/proc/sys/cpu/%d/speed", i);

    if (!g_file_test (filePath, G_FILE_TEST_EXISTS))
    {
      g_free (filePath);
      return FALSE;
    }

    file = fopen (filePath, "r");

    if (file)
    {
      fscanf (file, "%d", &cpu->cur_freq);
      fclose (file);
    }

    g_free (filePath);
  }

  return TRUE;
}
