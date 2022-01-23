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

/* The fixes file has to be included before any other #include directives */
#include "xfce4++/util/fixes.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <string>
#include <string.h>
#include <vector>

#include "xfce4-cpufreq-plugin.h"
#include "xfce4-cpufreq-linux-sysfs.h"

#define SYSFS_BASE  "/sys/devices/system/cpu"

static void cpufreq_sysfs_read_list (const std::string &file, std::vector<guint> &list);

static void cpufreq_sysfs_read_string (const std::string &file, std::string &string);

static void cpufreq_sysfs_read_list (const std::string &file, std::vector<std::string> &list);

static void parse_sysfs_init (gint cpu_number, Ptr0<CpuInfo> cpu);

static gchar* read_file_contents (const std::string &file);

static bool cpufreq_cpu_exists (gint num);



bool
cpufreq_sysfs_is_available ()
{
  return g_file_test (SYSFS_BASE"/cpu0/cpufreq", G_FILE_TEST_EXISTS);
}



void
cpufreq_sysfs_read_current (gint cpu_number)
{
  Ptr<CpuInfo> cpu = cpuFreq->cpus[cpu_number];
  std::string file;

  /* read current cpu freq */
  file = xfce4::sprintf (SYSFS_BASE "/cpu%i/cpufreq/scaling_cur_freq", cpu_number);
  cpufreq_sysfs_read_uint (file, &cpu->cur_freq);

  /* read current cpu governor */
  file = xfce4::sprintf (SYSFS_BASE "/cpu%i/cpufreq/scaling_governor", cpu_number);
  cpufreq_sysfs_read_string (file, cpu->cur_governor);

  /* read whether the cpu is online, skip first */
  if (cpu_number != 0)
  {
    guint online;

    file = xfce4::sprintf (SYSFS_BASE "/cpu%i/online", cpu_number);
    cpufreq_sysfs_read_uint (file, &online);

    cpu->online = (online != 0);
  }
}



bool
cpufreq_sysfs_read ()
{
  gint count = 0;
  while (cpufreq_cpu_exists (count))
    count++;

  if (count == 0)
    return false;

  gint i = 0;
  while (i < count)
    parse_sysfs_init (i++, nullptr);

  return true;
}



void
cpufreq_sysfs_read_uint (const std::string &file, guint *intval)
{
  gchar *contents = read_file_contents (file);
  if (contents) {
    int i = atoi (contents);
    if (i >= 0)
      *intval = guint(i);
    g_free (contents);
  }
}



static void
cpufreq_sysfs_read_list (const std::string &file, std::vector<guint> &list)
{
  gchar *contents = read_file_contents (file);

  if (contents) {
    gchar **tokens = g_strsplit (contents, " ", 0);
    g_free (contents);
    list.clear();
    for(gint i = 0; tokens[i] != NULL; i++) {
      gint value = atoi (tokens[i]);
      if (value >= 0)
        list.push_back(guint(value));
    }
    g_strfreev (tokens);
  }
}


static void
cpufreq_sysfs_read_string (const std::string &file, std::string &string)
{
  gchar *contents = read_file_contents (file);
  if (contents) {
    string = contents;
    g_free (contents);
  }
}



static void
cpufreq_sysfs_read_list (const std::string &file, std::vector<std::string> &list)
{
  gchar *contents = read_file_contents (file);

  if (contents) {
    gchar **tokens = g_strsplit (contents, " ", 0);
    g_free (contents);
    list.clear();
    for(gint i = 0; tokens[i] != NULL; i++) {
      list.push_back(tokens[i]);
    }
    g_strfreev (tokens);
  }
}



static void
parse_sysfs_init (gint cpu_number, Ptr0<CpuInfo> cpu)
{
  std::string file;
  bool add_cpu = false;

  if (cpu == nullptr) {
    cpu = xfce4::make<CpuInfo>();
    cpu->online = true;
    add_cpu = true;
  }

  /* read available cpu freqs */
  if (cpuFreq->intel_pstate == nullptr) {
    file = xfce4::sprintf (SYSFS_BASE "/cpu%i/cpufreq/scaling_available_frequencies", cpu_number);
    cpufreq_sysfs_read_list (file, cpu->available_freqs);
  }

  /* read available cpu governors */
  file = xfce4::sprintf (SYSFS_BASE "/cpu%i/cpufreq/scaling_available_governors", cpu_number);
  cpufreq_sysfs_read_list (file, cpu->available_governors);

  /* read cpu driver */
  file = xfce4::sprintf (SYSFS_BASE "/cpu%i/cpufreq/scaling_driver", cpu_number);
  cpufreq_sysfs_read_string (file, cpu->scaling_driver);

  /* read current cpu freq */
  file = xfce4::sprintf (SYSFS_BASE "/cpu%i/cpufreq/scaling_cur_freq", cpu_number);
  cpufreq_sysfs_read_uint (file, &cpu->cur_freq);

  /* read current cpu governor */
  file = xfce4::sprintf (SYSFS_BASE "/cpu%i/cpufreq/scaling_governor", cpu_number);
  cpufreq_sysfs_read_string (file, cpu->cur_governor);

  /* read max cpu freq */
  file = xfce4::sprintf (SYSFS_BASE "/cpu%i/cpufreq/scaling_max_freq", cpu_number);
  cpufreq_sysfs_read_uint (file, &cpu->max_freq_nominal);

  /* read min cpu freq */
  file = xfce4::sprintf (SYSFS_BASE "/cpu%i/cpufreq/scaling_min_freq", cpu_number);
  cpufreq_sysfs_read_uint (file, &cpu->min_freq);

  if (add_cpu)
    cpuFreq->cpus.push_back(cpu.toPtr());
}



static gchar*
read_file_contents (const std::string &file)
{
  if (!g_file_test (file.c_str(), G_FILE_TEST_EXISTS))
    return NULL;

  GError *error = NULL;
  gchar *contents = NULL;
  if (g_file_get_contents (file.c_str(), &contents, NULL, &error)) {
    g_strstrip (contents);
    return contents;
  }

  g_debug ("Error reading %s: %s\n", file.c_str(), error->message);
  g_error_free (error);
  return NULL;
}



static bool
cpufreq_cpu_exists (gint num)
{
  gchar file[128];

  g_snprintf (file, sizeof (file), SYSFS_BASE "/cpu%d", num);
  return g_file_test (file, G_FILE_TEST_EXISTS);
}
