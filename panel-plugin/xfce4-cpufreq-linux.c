/*  xfce4-cpu-freq-plugin - panel plugin for cpu informations
 *
 *  Copyright (c) 2006 Thomas Schreck <shrek@xfce.org>
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <dirent.h>
#include "xfce4-cpufreq-plugin.h"
#include "xfce4-cpufreq-linux.h"

#include <libxfcegui4/libxfcegui4.h>

#ifndef _
# include <libintl.h>
# define _(String) gettext (String)
#endif

static gboolean
cpufreq_cpu_parse_sysfs_init (gint cpu_number)
{
	CpuInfo *cpu;
	FILE    *file;
	gchar   *filePath, *fileContent, **tokens;

	cpu = g_new0 (CpuInfo, 1);

	/* read available cpu freqs */
	filePath = g_strdup_printf (
		"/sys/devices/system/cpu/cpu%i/cpufreq/scaling_available_frequencies",
		cpu_number);
	if (!g_file_test (filePath, G_FILE_TEST_EXISTS))
		goto file_error;
	file = fopen (filePath, "r");
	if (file)
	{
		gint i = 0;

		fileContent = g_new (gchar, 255);
		fgets (fileContent, 255, file);
		fclose (file);

		fileContent = g_strchomp (fileContent);
		tokens = g_strsplit (fileContent, " ", 0);
		g_free (fileContent);

		while (tokens[i] != NULL)
		{
			gint freq = atoi (tokens[i]);
			cpu->available_freqs = g_list_append (
				cpu->available_freqs, GINT_TO_POINTER(freq));
			i++;
		}
		g_strfreev (tokens);
	}
	g_free (filePath);

	/* read available cpu governors */
	filePath = g_strdup_printf (
		"/sys/devices/system/cpu/cpu%i/cpufreq/scaling_available_governors",
       		cpu_number);
	if (!g_file_test (filePath, G_FILE_TEST_EXISTS))
		goto file_error;
	file = fopen (filePath, "r");
	if (file)
	{
		gint i = 0;

		fileContent = g_new (gchar, 255);
		fgets (fileContent, 255, file);
		fclose (file);

		fileContent = g_strchomp (fileContent);
		tokens = g_strsplit (fileContent, " ", 0);
		g_free (fileContent);

		while (tokens[i] != NULL)
		{
			cpu->available_governors = g_list_append (
					cpu->available_governors,
					g_strdup (tokens[i]));
			i++;
		}
		g_strfreev (tokens);
	}
	g_free (filePath);

	/* read cpu driver */
	filePath = g_strdup_printf (
		"/sys/devices/system/cpu/cpu%i/cpufreq/scaling_driver",
		cpu_number);
	if (!g_file_test (filePath, G_FILE_TEST_EXISTS))
		goto file_error;
	file = fopen (filePath, "r");
	if (file)
	{
		cpu->scaling_driver = g_new (gchar, 15);
		fscanf (file, "%15s", cpu->scaling_driver);
		fclose (file);
	}
	g_free (filePath);

	/* read current cpu freq */
	filePath = g_strdup_printf (
		"/sys/devices/system/cpu/cpu%i/cpufreq/scaling_cur_freq",
		cpu_number);
	if (!g_file_test (filePath, G_FILE_TEST_EXISTS))
		goto file_error;
	file = fopen (filePath, "r");
	if (file)
	{
		fscanf (file, "%d", &cpu->cur_freq);
		fclose (file);
	}
	g_free (filePath);

	/* read current cpu governor */
	filePath = g_strdup_printf (
		"/sys/devices/system/cpu/cpu%i/cpufreq/scaling_governor",
		cpu_number);
	if (!g_file_test (filePath, G_FILE_TEST_EXISTS))
		goto file_error;
	file = fopen (filePath, "r");
	if (file)
	{
		cpu->cur_governor = g_new (gchar, 15);
		fscanf (file, "%15s", cpu->cur_governor);
		fclose (file);
	}
	g_free (filePath);

	/* read max cpu freq */
	filePath = g_strdup_printf (
		"/sys/devices/system/cpu/cpu%i/cpufreq/scaling_max_freq",
		cpu_number);
	if (!g_file_test (filePath, G_FILE_TEST_EXISTS))
		goto file_error;
	file = fopen (filePath, "r");
	if (file)
	{
		fscanf (file, "%d", &cpu->max_freq);
		fclose (file);
	}
	g_free (filePath);

	/* read min cpu freq */
	filePath = g_strdup_printf (
		"/sys/devices/system/cpu/cpu%i/cpufreq/scaling_max_freq",
		cpu_number);
	if (!g_file_test (filePath, G_FILE_TEST_EXISTS))
		goto file_error;
	file = fopen (filePath, "r");
	if (file)
	{
		fscanf (file, "%d", &cpu->min_freq);
		fclose (file);
	}
	g_free (filePath);

	g_ptr_array_add (cpuFreq->cpus, cpu);

	return TRUE;

file_error:
	g_free (filePath);
	return FALSE;
}

static gboolean
cpufreq_cpu_read_sysfs_current (gint cpu_number)
{
	CpuInfo *cpu;
	FILE	*file;
	gchar	*filePath;

	cpu = g_ptr_array_index (cpuFreq->cpus, cpu_number);

	/* read current cpu freq */
	filePath = g_strdup_printf (
		"/sys/devices/system/cpu/cpu%i/cpufreq/scaling_cur_freq",
		cpu_number);
	if (!g_file_test (filePath, G_FILE_TEST_EXISTS))
		goto file_error;
	file = fopen (filePath, "r");
	if (file)
	{
		fscanf (file, "%d", &cpu->cur_freq);
		fclose (file);
	}
	g_free (filePath);

	/* read current cpu governor */
	filePath = g_strdup_printf (
		"/sys/devices/system/cpu/cpu%i/cpufreq/scaling_governor",
		cpu_number);
	if (!g_file_test (filePath, G_FILE_TEST_EXISTS))
		goto file_error;
	file = fopen (filePath, "r");
	if (file)
	{
		fscanf (file, "%15s", cpu->cur_governor);
		fclose (file);
	}
	g_free (filePath);

	return TRUE;

file_error:
	g_free (filePath);
	return FALSE;
}

static gboolean
cpufreq_cpu_read_procfs_cpuinfo ()
{
	CpuInfo	*cpu;
	FILE	*file;
	gchar	*freq, *filePath, *fileContent;

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
				cpu = g_new0 (CpuInfo, 1);
				cpu->max_freq = 0;
				cpu->min_freq = 0;
				cpu->cur_governor = NULL;
				cpu->available_freqs = NULL;
				cpu->available_governors = NULL;

				freq = g_strrstr (fileContent, ":");
				if (freq != NULL)
				{
					sscanf (++freq, "%d.", &cpu->cur_freq);
					cpu->cur_freq *= 1000;
				}
				else
					break;

				g_ptr_array_add (cpuFreq->cpus, cpu);
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
					"CPU %d %d kHz (%d %%) - %d kHz (%d %%) - %20s",
					NULL, &cpu->min_freq,
					NULL, &cpu->max_freq,
					NULL, cpu->cur_governor);
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

static gboolean
cpufreq_cpu_read_sysfs ()
{
	gint j, i = -2;
	DIR *dir;
	struct dirent *dir_entry;

	if ((dir = opendir ("/sys/devices/system/cpu")) != NULL)
	{
		while ((dir_entry = readdir (dir)) != NULL)
			i++;
	}
	else
		return FALSE;
	closedir (dir);

	for (j = 0; j < i; j++)
	{
		cpufreq_cpu_parse_sysfs_init (j);
	}

	return TRUE;
}

gboolean
cpufreq_update_cpus (gpointer data)
{
	gint i;

	if (g_file_test ("/sys/devices/system/cpu/cpu0/cpufreq", G_FILE_TEST_EXISTS))
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
			g_free (cpu->cur_governor);
			g_free (cpu);
		}
		cpufreq_cpu_read_procfs ();
	}
	else
	{
		/* We do not need to update, because no scaling available */
		return FALSE;
	}

	return cpufreq_update_plugin ();
}

gboolean
cpufreq_linux_init (void)
{
	if (cpuFreq->cpus == NULL)
		return FALSE;

	if (g_file_test ("/sys/devices/system/cpu/cpu0/cpufreq", G_FILE_TEST_EXISTS))
		return cpufreq_cpu_read_sysfs ();
	else if (g_file_test ("/proc/cpufreq", G_FILE_TEST_EXISTS))
		return cpufreq_cpu_read_procfs ();
	else
	{	xfce_warn (_("Your system does not support cpufreq.\nThe applet only shows the current cpu frequency"));
		return cpufreq_cpu_read_procfs_cpuinfo ();
	}
}
