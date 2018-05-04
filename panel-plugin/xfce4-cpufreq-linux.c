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
#include "xfce4-cpufreq-plugin.h"
#include "xfce4-cpufreq-linux.h"

#include <libxfce4ui/libxfce4ui.h>

#ifndef _
# include <libintl.h>
# define _(String) gettext (String)
#endif

#define SYSFS_READ_STRING(file, contents, string)		\
	if (contents = read_sysfs_file_contents (file))	{	\
		g_free (string);								\
		string = contents;								\
	}

#define SYSFS_READ_STRING_LIST(file, contents, list)				\
	if (contents = read_sysfs_file_contents (file)) {				\
		gchar **tokens = NULL;										\
		gint i = 0;													\
		tokens = g_strsplit (contents, " ", 0);						\
		g_free (contents);											\
		g_list_free_full (list, g_free);							\
		while (tokens[i] != NULL) {									\
			list = g_list_append (list, strdup (tokens[i]));		\
			i++;													\
		}															\
		g_strfreev (tokens);										\
	}

#define SYSFS_READ_INT(file, contents, intval)			\
	if (contents = read_sysfs_file_contents (file)) {	\
		intval = atoi (contents);						\
		g_free (contents);								\
	}

#define SYSFS_READ_INT_LIST(file, contents, list)					\
	if (contents = read_sysfs_file_contents (file)) {				\
		gchar **tokens = NULL;										\
		gint i = 0;													\
		tokens = g_strsplit (contents, " ", 0);						\
		g_free (contents);											\
		g_list_free (list);											\
		while (tokens[i] != NULL) {									\
			gint value = atoi (tokens[i]);							\
			list = g_list_append (list, GINT_TO_POINTER (value));	\
			i++;													\
		}															\
		g_strfreev (tokens);										\
	}


static inline gchar *
read_sysfs_file_contents (const gchar *file)
{
	GError *error = NULL;
	gchar *contents = NULL;

	if (!g_file_test (file, G_FILE_TEST_EXISTS))
		return NULL;

	if (g_file_get_contents (file, &contents, NULL, &error)) {
		g_strstrip (contents);
		return contents;
	} else {
		g_debug ("Error reading %s: %s\n", file, error->message);
		g_error_free (error);
		return NULL;
	}
}

static void
cpufreq_cpu_parse_sysfs_init (gint cpu_number, CpuInfo *cpu)
{
	gchar   *file, *contents;
	gboolean add_cpu = FALSE;

	if (cpu == NULL) {
		cpu = g_new0 (CpuInfo, 1);
		add_cpu = TRUE;
	}

	/* read available cpu freqs */
	if (cpuFreq->intel_pstate == NULL) {
		file =
			g_strdup_printf ("/sys/devices/system/cpu/cpu%i/"
							 "cpufreq/scaling_available_frequencies",
							 cpu_number);
		SYSFS_READ_INT_LIST (file, contents, cpu->available_freqs);
		g_free (file);
	}

	/* read available cpu governors */
	file = g_strdup_printf (
		"/sys/devices/system/cpu/cpu%i/cpufreq/scaling_available_governors",
		cpu_number);
	SYSFS_READ_STRING_LIST (file, contents, cpu->available_governors);
	g_free (file);

	/* read cpu driver */
	file = g_strdup_printf (
		"/sys/devices/system/cpu/cpu%i/cpufreq/scaling_driver",
		cpu_number);
	SYSFS_READ_STRING (file, contents, cpu->scaling_driver);
	g_free (file);

	/* read current cpu freq */
        file = g_strdup_printf ("/sys/devices/system/cpu/cpu%i/"
                                "cpufreq/scaling_cur_freq",
                                cpu_number);
        SYSFS_READ_INT (file, contents, cpu->cur_freq);
        g_free (file);

	/* read current cpu governor */
	file = g_strdup_printf (
		"/sys/devices/system/cpu/cpu%i/cpufreq/scaling_governor",
		cpu_number);
	SYSFS_READ_STRING (file, contents, cpu->cur_governor);
	g_free (file);

	/* read max cpu freq */
	file = g_strdup_printf (
		"/sys/devices/system/cpu/cpu%i/cpufreq/scaling_max_freq",
		cpu_number);
	SYSFS_READ_INT (file, contents, cpu->max_freq);
	g_free (file);

	/* read min cpu freq */
	file = g_strdup_printf (
		"/sys/devices/system/cpu/cpu%i/cpufreq/scaling_min_freq",
		cpu_number);
	SYSFS_READ_INT (file, contents, cpu->min_freq);
	g_free (file);

	if (add_cpu)
		g_ptr_array_add (cpuFreq->cpus, cpu);
}

static void
cpufreq_cpu_read_sysfs_current (gint cpu_number)
{
	CpuInfo *cpu;
	gchar	*file, *contents;

	cpu = g_ptr_array_index (cpuFreq->cpus, cpu_number);

	/* read current cpu freq */
        file = g_strdup_printf ("/sys/devices/system/cpu/cpu%i/"
                                "cpufreq/scaling_cur_freq",
                                cpu_number);
        SYSFS_READ_INT (file, contents, cpu->cur_freq);
        g_free (file);

	/* read current cpu governor */
	file = g_strdup_printf ("/sys/devices/system/cpu/cpu%i/"
							"cpufreq/scaling_governor",
							cpu_number);
	SYSFS_READ_STRING (file, contents, cpu->cur_governor);
	g_free (file);
}

static gboolean
cpufreq_cpu_read_procfs_cpuinfo ()
{
	CpuInfo	*cpu;
	FILE	*file;
	gchar	*freq, *filePath, *fileContent;
	gint     i = 0;
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
				{
					cpu = g_ptr_array_index (cpuFreq->cpus, i);
				}

				if (cpu == NULL)
				{
					cpu = g_new0 (CpuInfo, 1);
					cpu->max_freq = 0;
					cpu->min_freq = 0;
					cpu->cur_governor = NULL;
					cpu->available_freqs = NULL;
					cpu->available_governors = NULL;
					add_cpu = TRUE;
				}

				freq = g_strrstr (fileContent, ":");
				if (freq != NULL)
				{
					sscanf (++freq, "%d.", &cpu->cur_freq);
					cpu->cur_freq *= 1000;
				}
				else {
					if (add_cpu)
						cpuinfo_free (cpu);
					break;
				}

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

static gboolean
cpufreq_cpu_read_procfs ()
{
	CpuInfo *cpu;
	FILE	*file;
	gint	i;
	gchar	*filePath, *fileContent;

	filePath = g_strdup ("/proc/cpufreq");
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

	for (i = 0; i < cpuFreq->cpus->len; i++)
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

static inline gboolean
cpufreq_cpu_exists (gint num)
{
	const gchar *base = "/sys/devices/system/cpu";
	gchar *file;
	gboolean ret;

	file = g_strdup_printf ("%s/cpu%d", base, num);
	ret = g_file_test (file, G_FILE_TEST_EXISTS);
	g_free (file);
	return ret;
}

static gboolean
cpufreq_cpu_read_sysfs (void)
{
	gchar *file;
	gint count = 0, i = 0;

	while (cpufreq_cpu_exists (count))
		count++;

	if (count == 0)
		return FALSE;

	while (i < count)
		cpufreq_cpu_parse_sysfs_init (i++, NULL);

	return TRUE;
}

gboolean
cpufreq_intel_pstate_params (void)
{
	gchar   *file, *contents;
	IntelPState *ips;

	ips = g_slice_new0(IntelPState);

	if (!g_file_test ("/sys/devices/system/cpu/intel_pstate",
					  G_FILE_TEST_EXISTS))
		return FALSE;

	file =
		g_strdup ("/sys/devices/system/cpu/intel_pstate/min_perf_pct");
	SYSFS_READ_INT (file, contents, ips->min_perf_pct);
	g_free (file);

	file =
		g_strdup ("/sys/devices/system/cpu/intel_pstate/max_perf_pct");
	SYSFS_READ_INT (file, contents, ips->max_perf_pct);
	g_free (file);

	file =
		g_strdup ("/sys/devices/system/cpu/intel_pstate/no_turbo");
	SYSFS_READ_INT (file, contents, ips->no_turbo);
	g_free (file);

	g_slice_free (IntelPState, cpuFreq->intel_pstate);
	cpuFreq->intel_pstate = ips;
	return TRUE;
}

static gboolean
cpufreq_cpu_intel_pstate_read ()
{
	CpuInfo *cpu;
	gint i;

	/* gather intel pstate parameters */
	if (!cpufreq_intel_pstate_params ())
		return FALSE;
        /* now read the number of cpus and the remaining cpufreq info
           for each of them from sysfs */
        if (!cpufreq_cpu_read_sysfs ())
          {
            return FALSE;
          }
	return TRUE;
}

gboolean
cpufreq_update_cpus (gpointer data)
{
	gint i;

	if (g_file_test ("/sys/devices/system/cpu/cpu0/cpufreq",
                         G_FILE_TEST_EXISTS))
	{
		for (i = 0; i < cpuFreq->cpus->len; i++)
			cpufreq_cpu_read_sysfs_current (i);
	}
	else if (g_file_test ("/proc/cpufreq", G_FILE_TEST_EXISTS))
	{
		/* First we delete the cpus and then read the /proc/cpufreq file again */
		for (i = 0; i < cpuFreq->cpus->len; i++)
		{
			CpuInfo *cpu = g_ptr_array_index (cpuFreq->cpus, i);
			g_ptr_array_remove_fast (cpuFreq->cpus, cpu);
			cpuinfo_free (cpu);
		}
		cpufreq_cpu_read_procfs ();
	}
	else
	{
		/* We do not need to update, because no scaling available */
		return FALSE;
	}

	return cpufreq_update_plugin (FALSE);
}

gboolean
cpufreq_linux_init (void)
{
	if (cpuFreq->cpus == NULL)
		return FALSE;

	if (g_file_test ("/sys/devices/system/cpu/cpu0/cpufreq", G_FILE_TEST_EXISTS))
		return cpufreq_cpu_read_sysfs ();
	else if (g_file_test ("/sys/devices/system/cpu/intel_pstate", G_FILE_TEST_EXISTS))
	{
		gboolean ret = cpufreq_cpu_intel_pstate_read ();

		/* Tools like i7z show the current real frequency using the
		   current maximum performance. Assuming this is the proper
		   way to do it, let's choose the maximum per default. Most
		   CPUs nowadays have more than one core anyway, so there will
		   not be much use in showing a single core's performance
		   value. Besides, it's not very likely the user wants to
		   follow values for 4 or 8 cores per second. */
		if (ret && cpuFreq->options->show_warning) {
			cpuFreq->options->show_cpu = CPU_MAX;
			cpuFreq->options->show_warning = FALSE;
		}
		return ret;
	}
	else if (g_file_test ("/proc/cpufreq", G_FILE_TEST_EXISTS))
		return cpufreq_cpu_read_procfs ();
	else
	{
		if (cpuFreq->options->show_warning)
		{
			xfce_dialog_show_warning (NULL, NULL, _("Your system does not support cpufreq.\nThe applet only shows the current cpu frequency"));
			cpuFreq->options->show_warning = FALSE;
		}

		return cpufreq_cpu_read_procfs_cpuinfo ();
	}
}
