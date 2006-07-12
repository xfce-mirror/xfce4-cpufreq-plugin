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

#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>

#include "cpu-freq-plugin.h"
#include "cpu-freq-monitor.h"
#include "cpu-freq-dialogs.h"
#include "cpu-freq-overview.h"

#define BORDER  4
#define SPACING 2
#define SMALL_PANEL_SIZE 34

static void
cpu_freq_update_tooltip (CpuInfo* cpu)
{
	gchar* tooltip;
	gchar* freq;
	gchar* freq_unit;
	gint div;
	
	if (cpu->cur_freq > 999999)
	{
		div = (1000 * 1000);
		freq_unit = g_strdup ("GHz");
	}
	else
	{
		div = 1000;
		freq_unit = g_strdup ("MHz");
	}

	if ( (cpu->cur_freq % div) == 0 || div == 1000 )
		freq = g_strdup_printf ("%d %s", cpu->cur_freq/div, freq_unit);
	else
		freq = g_strdup_printf ("%3.2f %s", 
				((gfloat)cpu->cur_freq/div), freq_unit);


	if (cpuFreq->options->show_label_governor &&
		cpuFreq->options->show_label_freq)
	{
		
		tooltip = g_strdup_printf (_("CPU Frequency Plugin"));
		goto set_tooltip;
	}

	if (!cpuFreq->options->show_label_governor &&
		!cpuFreq->options->show_label_freq)
	{
		
		tooltip = g_strdup_printf (_("Frequency: %s\nGovernor: %s"),
				freq, cpu->cur_governor);
		goto set_tooltip;
	}

	if (!cpuFreq->options->show_label_governor &&
		cpuFreq->options->show_label_freq)
	{
		tooltip = g_strdup_printf (_("Governor: %s"), 
				cpu->cur_governor);
		goto set_tooltip;
	}

	if (cpuFreq->options->show_label_governor &&
		!cpuFreq->options->show_label_freq)
	{
		tooltip = g_strdup_printf (_("Frequency: %s"), 
				freq);
		goto set_tooltip;
	}

set_tooltip:
	gtk_tooltips_set_tip (cpuFreq->tooltip,
			cpuFreq->ebox,
			tooltip,
			NULL);
	
	g_free (freq_unit);
	g_free (freq);
	g_free (tooltip);
}

static void
cpu_freq_update_label (CpuInfo* cpu)
{
	if (!cpuFreq->options->show_label_governor &&
		!cpuFreq->options->show_label_freq)
		return;

	gchar *label;
	gchar *freq;
	gchar *freq_unit;
	gint div;
	gboolean small;
		
	if (cpu->cur_freq > 999999)
	{
		div = (1000 * 1000);
		freq_unit = g_strdup ("GHz");
	}
	else
	{
		div = 1000;
		freq_unit = g_strdup ("MHz");
	}

	if ( (cpu->cur_freq % div) == 0 || div == 1000 )
		freq = g_strdup_printf ("%d %s", cpu->cur_freq/div, freq_unit);
	else
		freq = g_strdup_printf ("%3.2f %s", 
				((gfloat)cpu->cur_freq/div), freq_unit);

	small = xfce_panel_plugin_get_size (cpuFreq->plugin) 
						<= SMALL_PANEL_SIZE;

	label = g_strconcat (small ? "<span size=\"xx-small\">" : 
			 "<span size=\"x-small\">",
			 
			 cpuFreq->options->show_label_freq ? freq : "",

			 cpuFreq->options->show_label_freq &&
			 cpuFreq->options->show_label_governor ? "\n" : "",

			 cpuFreq->options->show_label_governor ?
		 				cpu->cur_governor : "",
			 "</span>",
			 NULL
			);

	gtk_label_set_label (GTK_LABEL(cpuFreq->label), label);

	g_free (label);
	g_free (freq);
	g_free (freq_unit);
}

void
cpu_freq_widgets (void)
{
	GtkOrientation orientation =
		xfce_panel_plugin_get_orientation (cpuFreq->plugin);
	GtkWidget *box;
	gchar *tooltip_msg;
	gint size;
	
	if (cpuFreq->ebox)
		gtk_widget_destroy (cpuFreq->ebox);

	cpuFreq->ebox = gtk_event_box_new ();

	if (orientation == GTK_ORIENTATION_HORIZONTAL)
		box = gtk_hbox_new (FALSE, SPACING);
	else
		box = gtk_vbox_new (FALSE, SPACING);

	gtk_container_set_border_width (GTK_CONTAINER (box), BORDER);
	gtk_container_add (GTK_CONTAINER (cpuFreq->ebox), box);

	tooltip_msg = "CPU Frequency Plugin";
	gtk_tooltips_set_tip (cpuFreq->tooltip,
			      cpuFreq->ebox,
			      tooltip_msg,
			      NULL);

	if(cpuFreq->options->show_icon)
	{
		size = xfce_panel_plugin_get_size (cpuFreq->plugin) - (2 * BORDER);
		cpuFreq->icon = gtk_image_new_from_icon_name ("cpu", size);
		gtk_box_pack_start
			(GTK_BOX (box), cpuFreq->icon,
			 FALSE, FALSE, 0);
	}

	if (cpuFreq->options->show_label_freq || cpuFreq->options->show_label_governor)
	{
		cpuFreq->label = gtk_label_new ("");
		gtk_box_pack_start
			(GTK_BOX (box), cpuFreq->label,
			 FALSE, FALSE, 0);
		gtk_label_set_use_markup
			(GTK_LABEL (cpuFreq->label), TRUE);
	}

	gtk_widget_show_all (cpuFreq->ebox);

	g_signal_connect (cpuFreq->ebox, "button-press-event", 
			  G_CALLBACK (cpu_freq_overview), cpuFreq);
	
	gtk_container_add (GTK_CONTAINER (cpuFreq->plugin), cpuFreq->ebox);
	xfce_panel_plugin_add_action_widget (cpuFreq->plugin, cpuFreq->ebox);
}

void
cpu_freq_update_plugin (void)
{
	gint i;

	for (i = 0; i < cpuFreq->cpus->len; i++)
	{
		if (G_LIKELY (i == cpuFreq->options->cpu_number))
		{
			CpuInfo *cpu = g_ptr_array_index (cpuFreq->cpus, i);
			cpu_freq_update_label (cpu);
			cpu_freq_update_tooltip (cpu);
			return;
		}
	}
}

void
cpu_freq_restart_monitor_timeout_handle (void)
{
	g_source_remove (cpuFreq->timeoutHandle);
	cpuFreq->timeoutHandle = g_timeout_add (
			cpuFreq->options->monitor_timeout,
			(GSourceFunc) file_modified,
			NULL);
}

static void
cpu_freq_stop_monitor (void)
{
	gint i;
	g_source_remove (cpuFreq->timeoutHandle);
	cpu_freq_stop_monitor_timeout ();
}

static gboolean
cpu_freq_start_monitor (void)
{
	gboolean status;
	gint i;
	status = cpu_freq_start_monitor_timeout ();
	cpuFreq->timeoutHandle = g_timeout_add (
		cpuFreq->options->monitor_timeout,
		(GSourceFunc) file_modified,
		NULL);
	return status;
}

static void
cpu_freq_read_config (void)
{
	XfceRc *rc;
	gchar  *file;

	file = xfce_panel_plugin_save_location (cpuFreq->plugin, FALSE);

	if (G_UNLIKELY (!file))
		return;

	rc = xfce_rc_simple_open (file, FALSE);
	g_free (file);

	cpuFreq->options->cpu_number =
		xfce_rc_read_int_entry  (rc, "cpu_number", 0);
	cpuFreq->options->monitor_timeout =
		xfce_rc_read_int_entry  (rc, "monitor_timeout", 100);
	cpuFreq->options->show_icon  =
		xfce_rc_read_bool_entry (rc, "show_icon",  TRUE);
	cpuFreq->options->show_label_freq =
		xfce_rc_read_bool_entry (rc, "show_label_freq", TRUE);
	cpuFreq->options->show_label_governor =
		xfce_rc_read_bool_entry (rc, "show_label_governor", TRUE);

	xfce_rc_close (rc);
}

void
cpu_freq_write_config (XfcePanelPlugin *plugin)
{
	XfceRc *rc;
	gchar  *file;

	file = xfce_panel_plugin_save_location (plugin, TRUE);

	if (G_UNLIKELY (!file))
		return;

	rc = xfce_rc_simple_open (file, FALSE);
	g_free(file);

	xfce_rc_write_int_entry
		(rc, "cpu_number", cpuFreq->options->cpu_number);
	xfce_rc_write_int_entry
		(rc, "monitor_timeout",
		 cpuFreq->options->monitor_timeout);
	xfce_rc_write_bool_entry
		(rc, "show_icon", cpuFreq->options->show_icon);
	xfce_rc_write_bool_entry
		(rc, "show_label_freq", cpuFreq->options->show_label_freq);
	xfce_rc_write_bool_entry
		(rc, "show_label_governor",
		 cpuFreq->options->show_label_governor);

	xfce_rc_close (rc);
}

static void
cpu_freq_free (XfcePanelPlugin *plugin)
{
	gint i;
	gboolean removedTimeoutHandle = FALSE;

	cpu_freq_stop_monitor ();
	g_ptr_array_free (cpuFreq->cpus, TRUE);

	gtk_tooltips_set_tip (cpuFreq->tooltip, cpuFreq->ebox, NULL, NULL);
	g_object_unref (cpuFreq->tooltip);

	cpuFreq->plugin = NULL;
	g_free (cpuFreq);
}

static gboolean
cpu_freq_set_size (XfcePanelPlugin *plugin, gint wsize)
{
	if (xfce_panel_plugin_get_orientation (plugin) ==
			GTK_ORIENTATION_HORIZONTAL)
		gtk_widget_set_size_request (GTK_WIDGET (plugin), -1, wsize);
	else
		gtk_widget_set_size_request (GTK_WIDGET (plugin), wsize, -1);

	cpu_freq_update_plugin ();

	return TRUE;
}

static void
cpu_freq_orientation_changed (XfcePanelPlugin *plugin,
			      GtkOrientation orientation)
{
	cpu_freq_widgets ();
}

static void
cpu_freq_construct (XfcePanelPlugin *plugin)
{
	xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

	cpuFreq = g_new0 (CpuFreqPlugin, 1);
	cpuFreq->options = g_new0 (CpuFreqPluginOptions, 1);
	cpuFreq->plugin = plugin;

	cpuFreq->tooltip = gtk_tooltips_new ();
	g_object_ref (cpuFreq->tooltip);

	cpuFreq->cpus = g_ptr_array_new ();

	cpu_freq_read_config ();
	
	cpu_freq_widgets ();

	cpu_freq_start_monitor ();

	g_signal_connect (plugin, "free-data", G_CALLBACK (cpu_freq_free),
			  NULL);

	g_signal_connect (plugin, "save", G_CALLBACK (cpu_freq_write_config),
			  NULL);

	g_signal_connect (plugin, "size-changed",
			  G_CALLBACK (cpu_freq_set_size), NULL);

	g_signal_connect (plugin, "orientation-changed",
			  G_CALLBACK (cpu_freq_orientation_changed), NULL);

	/* the configure and about menu items are hidden by default */
	xfce_panel_plugin_menu_show_configure (plugin);
	g_signal_connect (plugin, "configure-plugin",
			  G_CALLBACK (cpu_freq_configure), NULL);
}

XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL (cpu_freq_construct);
