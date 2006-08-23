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
#include <gksuui.h>

#include "xfce4-cpufreq-plugin.h"
#include "xfce4-cpufreq-linux.h"

gboolean
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

		fileContent = (gchar*)malloc (sizeof(gchar) * 255);
		fgets (fileContent, 255, file);
		fclose (file);

		fileContent = g_strchomp (fileContent);
		tokens = g_strsplit (fileContent, " ", 0);
		g_free (fileContent);

		while (tokens[i] != NULL)
		{
			gint freq = atoi (tokens[i]);
			cpu->available_freqs = g_list_append (
				cpu->available_freqs, freq);
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

		fileContent = (gchar*)malloc (sizeof(gchar) * 255);
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
		cpu->scaling_driver = (gchar*)malloc (sizeof (gchar) * 15);
		fscanf (file, "%s", cpu->scaling_driver);
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
		cpu->cur_governor = (gchar*)malloc (sizeof (gchar) * 15);
		fscanf (file, "%d", cpu->cur_governor);
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

gboolean
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
		fscanf (file, "%s", cpu->cur_governor);
		fclose (file);
	}
	g_free (filePath);

	return TRUE;

file_error:
	g_free (filePath);
	return FALSE;
}

gboolean
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
		return cpufreq_cpu_parse_sysfs_init (j);
	}

	return TRUE;
}

gboolean
cpufreq_cpu_set_freq (guint cpu_number, guint *freq)
{
	gchar     *cmd, *cpu_num, *cpu_freq;
	GtkWidget *gksuui_dialog;
	GError    *error = NULL;

	// TODO check if freq is different
	
	cpu_freq = (gchar*)malloc (sizeof (gchar) * 30);
	cpu_num  = (gchar*)malloc (sizeof (gchar) * 3);
	sprintf (cpu_num, "%d", cpu_number);
	sprintf (cpu_freq, "%d", freq);
	cmd = g_strconcat ("/bin/echo ",
			cpu_freq,
			" > /sys/devices/system/cpu/cpu",
			cpu_num,
			"/cpufreq/scaling_setspeed",
			NULL);

	if (gksu_context_try_need_password (cpuFreq->gksu_ctx))
	{
		gksuui_dialog = gksuui_dialog_new ();
		gksuui_dialog_set_message (GKSUUI_DIALOG (gksuui_dialog),
				_("Please enter root password to change CPU frequency !"));
		gtk_widget_show_all (gksuui_dialog);
		if (gtk_dialog_run (GTK_DIALOG (gksuui_dialog)))
			gksu_context_set_password (cpuFreq->gksu_ctx,
					gksuui_dialog_get_password (GKSUUI_DIALOG (gksuui_dialog)));

		gtk_widget_hide (gksuui_dialog);
	}

	gksu_context_set_command (cpuFreq->gksu_ctx, cmd);
	gksu_context_run (cpuFreq->gksu_ctx, &error);

	g_free (cpu_num);
	g_free (cpu_freq);
	g_free (cmd);
	
	if (error)
		return FALSE;
	return TRUE;
}

gboolean
cpufreq_cpu_set_governor (guint cpu_number, gchar *governor)
{
	gchar	  *cmd, *cpu_num;
	GtkWidget *gksuui_dialog;
	GError	  *error = NULL;

	if (governor == NULL) //TODO && governor in list
		return FALSE;

	cpu_num = (gchar*)malloc (sizeof (gchar) * 3);
	sprintf (cpu_num, "%d", cpu_number);
	cmd = g_strconcat ("/bin/echo ", 
			governor, 
			" > /sys/devices/system/cpu/cpu", 
			cpu_num, 
			"/cpufreq/scaling_governor", 
			NULL);

	if (gksu_context_try_need_password (cpuFreq->gksu_ctx))
	{
		gksuui_dialog = gksuui_dialog_new ();
		gksuui_dialog_set_message (GKSUUI_DIALOG (gksuui_dialog),
				_("Please enter root password to change CPU governor !"));
		gtk_widget_show_all (gksuui_dialog);
		if (gtk_dialog_run (GTK_DIALOG (gksuui_dialog)))
			gksu_context_set_password (cpuFreq->gksu_ctx,
				gksuui_dialog_get_password (GKSUUI_DIALOG (gksuui_dialog)));
		
		gtk_widget_hide (gksuui_dialog);
	}

	gksu_context_set_command (cpuFreq->gksu_ctx, cmd);
	gksu_context_run (cpuFreq->gksu_ctx, &error);

	g_free (cpu_num);
	g_free (cmd);

	if (error)
		return FALSE;
	return TRUE;
}

gboolean
cpufreq_update_cpus (gpointer data)
{
	gint i;

	for (i = 0; i < cpuFreq->cpus->len; i++)
		cpufreq_cpu_read_sysfs_current (i);

	return cpufreq_update_plugin ();
}

gboolean
cpufreq_linux_init (void)
{
	if (cpuFreq->cpus == NULL)
		return FALSE;

	if (g_file_test ("/sys/devices/system/cpu/cpu0/cpufreq", G_FILE_TEST_EXISTS))
	{
		return cpufreq_cpu_read_sysfs ();
	}
	else
		return FALSE;
}
