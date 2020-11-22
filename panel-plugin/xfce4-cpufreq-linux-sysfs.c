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
#include <stdlib.h>
#include <string.h>

#include "xfce4-cpufreq-plugin.h"
#include "xfce4-cpufreq-linux-sysfs.h"

#define SYSFS_BASE  "/sys/devices/system/cpu"

static void cpufreq_sysfs_read_int_list (gchar *file, GList **list);

static void cpufreq_sysfs_read_string (gchar *file, gchar **string);

static void cpufreq_sysfs_read_string_list (gchar *file, GList **list);

static void parse_sysfs_init (gint cpu_number, CpuInfo *cpu);

static inline gchar* read_file_contents (const gchar *file);

static inline gboolean cpufreq_cpu_exists (gint num);



gboolean
cpufreq_sysfs_is_available (void)
{
  return g_file_test (SYSFS_BASE"/cpu0/cpufreq", G_FILE_TEST_EXISTS);
}



void
cpufreq_sysfs_read_current (gint cpu_number)
{
  CpuInfo *cpu;
  gchar file[128];

  cpu = g_ptr_array_index (cpuFreq->cpus, cpu_number);

  /* read current cpu freq */
  g_snprintf (file, sizeof (file), SYSFS_BASE"/cpu%i/cpufreq/scaling_cur_freq", cpu_number);
  cpufreq_sysfs_read_int (file, &cpu->cur_freq);

  /* read current cpu governor */
  g_snprintf (file, sizeof (file), SYSFS_BASE"/cpu%i/cpufreq/scaling_governor", cpu_number);
  cpufreq_sysfs_read_string (file, &cpu->cur_governor);

  /* read whether the cpu is online, skip first */
  if (cpu_number != 0)
  {
    guint online;

    g_snprintf (file, sizeof (file), SYSFS_BASE"/cpu%i/online", cpu_number);
    cpufreq_sysfs_read_int (file, &online);

    cpu->online = online != 0;
  }
}



gboolean
cpufreq_sysfs_read (void)
{
  gint count = 0, i = 0;

  while (cpufreq_cpu_exists (count))
    count++;

  if (count == 0)
    return FALSE;

  while (i < count)
    parse_sysfs_init (i++, NULL);

  return TRUE;
}



void
cpufreq_sysfs_read_int (gchar *file, guint *intval)
{
  gchar *contents = read_file_contents (file);

  if (contents) {
    (*intval) = atoi (contents);
    g_free (contents);
  }
}



static void
cpufreq_sysfs_read_int_list (gchar *file, GList **list)
{
  gchar *contents = read_file_contents (file);

  if (contents) {
    gchar **tokens = NULL;
    gint i = 0;
    tokens = g_strsplit (contents, " ", 0);
    g_free (contents);
    g_list_free (*list);
    while (tokens[i] != NULL) {
      gint value = atoi (tokens[i]);
      *list = g_list_append (*list, GINT_TO_POINTER (value));
      i++;
    }
    g_strfreev (tokens);
  }
}


static void
cpufreq_sysfs_read_string (gchar *file, gchar **string)
{
  gchar *contents = read_file_contents (file);

  if (contents) {
    g_free (*string);
    *string = contents;
  }
}



static void
cpufreq_sysfs_read_string_list (gchar *file, GList **list)
{
  gchar *contents = read_file_contents (file);

  if (contents) {
    gchar **tokens = NULL;
    gint i = 0;
    tokens = g_strsplit (contents, " ", 0);
    g_free (contents);
    g_list_free_full (*list, g_free);
    while (tokens[i] != NULL) {
      *list = g_list_append (*list, strdup (tokens[i]));
      i++;
    }
    g_strfreev (tokens);
  }
}



static void
parse_sysfs_init (gint cpu_number, CpuInfo *cpu)
{
  gchar file[128];
  gboolean add_cpu = FALSE;

  if (cpu == NULL) {
    cpu = g_new0 (CpuInfo, 1);
    cpu->online = TRUE;
    add_cpu = TRUE;
  }

  /* read available cpu freqs */
  if (cpuFreq->intel_pstate == NULL) {
    g_snprintf (file, sizeof (file), SYSFS_BASE"/cpu%i/cpufreq/scaling_available_frequencies", cpu_number);
    cpufreq_sysfs_read_int_list (file, &cpu->available_freqs);
  }

  /* read available cpu governors */
  g_snprintf (file, sizeof (file), SYSFS_BASE"/cpu%i/cpufreq/scaling_available_governors", cpu_number);
  cpufreq_sysfs_read_string_list (file, &cpu->available_governors);

  /* read cpu driver */
  g_snprintf (file, sizeof (file), SYSFS_BASE"/cpu%i/cpufreq/scaling_driver", cpu_number);
  cpufreq_sysfs_read_string (file, &cpu->scaling_driver);

  /* read current cpu freq */
  g_snprintf (file, sizeof (file), SYSFS_BASE"/cpu%i/cpufreq/scaling_cur_freq", cpu_number);
  cpufreq_sysfs_read_int (file, &cpu->cur_freq);

  /* read current cpu governor */
  g_snprintf (file, sizeof (file), SYSFS_BASE"/cpu%i/cpufreq/scaling_governor", cpu_number);
  cpufreq_sysfs_read_string (file, &cpu->cur_governor);

  /* read max cpu freq */
  g_snprintf (file, sizeof (file), SYSFS_BASE"/cpu%i/cpufreq/scaling_max_freq", cpu_number);
  cpufreq_sysfs_read_int (file, &cpu->max_freq);

  /* read min cpu freq */
  g_snprintf (file, sizeof (file), SYSFS_BASE"/cpu%i/cpufreq/scaling_min_freq", cpu_number);
  cpufreq_sysfs_read_int (file, &cpu->min_freq);

  if (add_cpu)
    g_ptr_array_add (cpuFreq->cpus, cpu);
}



static inline gchar*
read_file_contents (const gchar *file)
{
  GError *error = NULL;
  gchar *contents = NULL;

  if (!g_file_test (file, G_FILE_TEST_EXISTS))
    return NULL;

  if (g_file_get_contents (file, &contents, NULL, &error)) {
    g_strstrip (contents);
    return contents;
  }

  g_debug ("Error reading %s: %s\n", file, error->message);
  g_error_free (error);
  return NULL;
}



static inline gboolean
cpufreq_cpu_exists (gint num)
{
  gchar file[128];

  g_snprintf (file, sizeof (file), SYSFS_BASE"/cpu%d", num);
  return g_file_test (file, G_FILE_TEST_EXISTS);
}
