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
cpufreq_sysfs_read_current ()
{
  /*
   * The following code reads cpufreq data from sysfs asynchronously,
   * in a different thread that isn't interfering with the GUI thread.
   *
   * The reason for this is that reading all the '/sys/.../scaling_cur_freq' files can
   * take a long time, for example, 800 milliseconds on a Ryzen 3900X CPU with 24 threads.
   */

  /* Start a new sysfs-read only if the previous read has finished */
  xfce4::LaunchConfig config;
  config.start_if_busy = false;

  const std::vector<Ptr<CpuInfo>> cpus = cpuFreq->cpus;
  xfce4::singleThreadQueue->start(config, [cpus]() {
      for (size_t i = 0; i < cpus.size(); i++) {
        Ptr<CpuInfo> cpu = cpus[i];
        std::string file;

        /* read current cpu freq */
        guint cur_freq;
        file = xfce4::sprintf (SYSFS_BASE "/cpu%zu/cpufreq/scaling_cur_freq", i);
        cpufreq_sysfs_read_uint (file, &cur_freq);

        /* read current cpu governor */
        std::string cpu_governor;
        file = xfce4::sprintf (SYSFS_BASE "/cpu%zu/cpufreq/scaling_governor", i);
        cpufreq_sysfs_read_string (file, cpu_governor);

        /* read current cpu preference */
        std::string cpu_preference;
        file = xfce4::sprintf (SYSFS_BASE "/cpu%zu/cpufreq/energy_performance_preference", i);
        cpufreq_sysfs_read_string (file, cpu_preference);

        /* read whether the cpu is online, skip first */
        guint online = 1;
        if (i != 0)
        {
          file = xfce4::sprintf (SYSFS_BASE "/cpu%zu/online", i);
          cpufreq_sysfs_read_uint (file, &online);
        }

        {
            std::lock_guard<std::mutex> guard(cpu->mutex);
            cpu->shared.cur_freq = cur_freq;
            cpu->shared.cur_governor = cpu_governor;
            cpu->shared.cur_preference = cpu_preference;
            cpu->shared.online = (online != 0);
        }
      }
    });
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

  /* read available cpu preferences */
  file = xfce4::sprintf (SYSFS_BASE "/cpu%i/cpufreq/energy_performance_available_preferences", cpu_number);
  cpufreq_sysfs_read_list (file, cpu->available_preferences);

  /* read cpu driver */
  file = xfce4::sprintf (SYSFS_BASE "/cpu%i/cpufreq/scaling_driver", cpu_number);
  cpufreq_sysfs_read_string (file, cpu->scaling_driver);

  /* NOTE: Do NOT read the current CPU frequency here.
   *       Reading all '/sys/.../scaling_cur_freq' files
   *       can take hundreds of milliseconds (for example: 800 milliseconds).
   */

  /* read current cpu governor */
  std::string cur_governor;
  file = xfce4::sprintf (SYSFS_BASE "/cpu%i/cpufreq/scaling_governor", cpu_number);
  cpufreq_sysfs_read_string (file, cur_governor);

  /* read current cpu preference */
  std::string cur_preference;
  file = xfce4::sprintf (SYSFS_BASE "/cpu%i/cpufreq/energy_performance_preference", cpu_number);
  cpufreq_sysfs_read_string (file, cur_preference);

  /* read max cpu freq */
  file = xfce4::sprintf (SYSFS_BASE "/cpu%i/cpufreq/cpuinfo_max_freq", cpu_number);
  cpufreq_sysfs_read_uint (file, &cpu->max_freq_nominal);

  /* read min cpu freq */
  file = xfce4::sprintf (SYSFS_BASE "/cpu%i/cpufreq/cpuinfo_min_freq", cpu_number);
  cpufreq_sysfs_read_uint (file, &cpu->min_freq);

  {
    std::lock_guard<std::mutex> guard(cpu->mutex);
    cpu->shared.online = true;
    cpu->shared.cur_freq = 0;
    cpu->shared.cur_governor = cur_governor;
    cpu->shared.cur_preference = cur_preference;
  }

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
