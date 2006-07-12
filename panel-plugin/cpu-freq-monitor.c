/*  xfce4-cpu-freq-plugin - panel plugin for cpu informations
 *
 *  Copyright (c) 2006 Thomas Schreck <thomas.schreck@gmail.com>
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

#include "cpu-freq-plugin.h"
#include "cpu-freq-monitor.h"

#include <dirent.h>

static gint
compare_str (gconstpointer a, gconstpointer b)
{
	return (g_ascii_strcasecmp (a, b));
}

gboolean
file_modified (gpointer data)
{
	gint i;
	gchar* filePath;
	FILE *file;
	for (i = 0; i < cpuFreq->cpus->len; i++)
	{
		CpuInfo *cpu = g_ptr_array_index (cpuFreq->cpus, i);

		filePath = g_strdup_printf (
			"/sys/devices/system/cpu/cpu%i/cpufreq/scaling_cur_freq",
			i);

		if (!g_file_test (filePath, G_FILE_TEST_EXISTS))
		{
			g_free (filePath);
			return;
		}
		
		file = fopen (filePath, "r");
		if (file)
		{
			fscanf (file, "%d", &cpu->cur_freq);
			fclose (file);
		}
		g_free (filePath);
		
		filePath = g_strdup_printf (
			"/sys/devices/system/cpu/cpu%i/cpufreq/scaling_governor",
			i);

		if (!g_file_test (filePath, G_FILE_TEST_EXISTS))
		{
			g_free (filePath);
			return;
		}

		file = fopen (filePath, "r");
		if (file)
		{
			fscanf (file, "%s", cpu->cur_governor);
			fclose (file);
		}
		g_free (filePath);
	}

	cpu_freq_update_plugin ();

	return TRUE;
}

static gboolean
cpu_add (gint cpu_number, gchar* path)
{
	gchar *files[] = {
		"scaling_available_frequencies",
		"scaling_available_governors",
		"scaling_cur_freq",
		"scaling_governor",
		"scaling_driver",
		NULL };
	CpuInfo *cpu;
	gint i;
	FILE *file;
	gchar *filePath = NULL;
	gchar *fileContent;
	gchar **tokens = NULL;
	cpu = g_new0 (CpuInfo, 1);

	filePath = g_strdup_printf ("%s%s", path, files[2]);

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

	filePath = g_strdup_printf ("%s%s", path, files[3]);

	if (!g_file_test (filePath, G_FILE_TEST_EXISTS))
	{
		g_free (filePath);
		return FALSE;
	}

	cpu->cur_governor = (gchar*)malloc (sizeof (gchar) * 15);
	file = fopen (filePath, "r");
	if (file)
	{
		fscanf (file, "%s", cpu->cur_governor );
		fclose (file);
	}
	g_free (filePath);

	filePath = g_strdup_printf ("%s%s", path, files[4]);

	if (!g_file_test (filePath, G_FILE_TEST_EXISTS))
	{
		g_free (filePath);
		return FALSE;
	}

	cpu->cur_driver = (gchar*)malloc (sizeof (gchar) * 15);
	file = fopen (filePath, "r");
	if (file)
	{
		fscanf (file, "%s", cpu->cur_driver );
		fclose (file);
	}
	g_free (filePath);
	
	filePath = g_strdup_printf ("%s%s", path, files[0]);
	fileContent = (gchar*)malloc (sizeof(gchar) * 255);
	if (!g_file_test (filePath, G_FILE_TEST_EXISTS))
	{
		g_free (filePath);
		return FALSE;
	}

	file = fopen (filePath, "r");
	if (file)
	{
		fgets (fileContent, 255, file);
		fclose (file);
		fileContent = g_strchomp (fileContent);
		tokens = g_strsplit (fileContent, " ", 0);

		i = 0;
		while (tokens[i] != NULL)
		{
			gint freq;
			freq = atoi (tokens[i]);
			cpu->available_freqs = g_list_append (
					cpu->available_freqs,
					freq);
			i++;
		}
		g_strfreev (tokens);
	}
	g_free (fileContent);
	g_free (filePath);
	
	tokens = NULL;
	filePath = g_strdup_printf ("%s%s", path, files[1]);
	fileContent = (gchar*)malloc (sizeof(gchar) * 255);
	if (!g_file_test (filePath, G_FILE_TEST_EXISTS))
	{
		g_free (filePath);
		return FALSE;
	}
	
	file = fopen (filePath, "r");
	if (file)
	{
		fgets (fileContent, 255, file);
		fclose (file);
		fileContent = g_strchomp (fileContent);
		tokens = g_strsplit (fileContent, " ", 0);
		
		i = 0;
		while (tokens[i] != NULL)
		{
			cpu->available_governors = g_list_append (
					cpu->available_governors, 
				 	g_strdup (tokens[i]));
			i++;
		}
		g_strfreev (tokens);
	}
	cpu->available_governors = g_list_sort (cpu->available_governors,
						compare_str);
	g_free (fileContent);
	g_free (filePath);
	g_ptr_array_add (cpuFreq->cpus, cpu);
	
	return TRUE;
}

static gboolean
cpu_init (void)
{
	gint cpuAmount = 0;
	int i;
	DIR *dir;
	struct dirent *dir_entry;

	if ((dir = opendir ("/sys/devices/system/cpu")) != NULL)
	{
		while ((dir_entry = readdir(dir)) != NULL)
		{
			cpuAmount++;
		}
	}
	else
		return FALSE;

	closedir (dir);

	cpuAmount-=2;
	for (i = 0; i < cpuAmount; i++)
	{
		gchar* cpuPath = g_strdup_printf
			("/sys/devices/system/cpu/cpu%i/cpufreq/", i);
		if (cpu_add (i,cpuPath) == FALSE)
		{	g_free (cpuPath);
			return FALSE;
		}
		g_free (cpuPath);
	}

	return TRUE;
}

void
cpu_freq_stop_monitor_timeout (void)
{
	GList *list;
	gint i;

	for (i = 0; i < cpuFreq->cpus->len; i++)
	{
		CpuInfo *cpu = g_ptr_array_index (cpuFreq->cpus, i);
		g_ptr_array_remove_fast (cpuFreq->cpus, cpu);
		g_free (cpu->cur_governor);
		g_free (cpu);
		g_list_free (cpu->available_freqs);
		g_list_free (cpu->available_governors);
	}
}

gboolean
cpu_freq_start_monitor_timeout (void)
{
	if( g_file_test("/sys/devices/system/cpu/cpu0/cpufreq",
	    G_FILE_TEST_EXISTS) )
	{
		if (cpu_init () == FALSE)
			return FALSE;
	}
	else
		return FALSE;

	return TRUE;
}
