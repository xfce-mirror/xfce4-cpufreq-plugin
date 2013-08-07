/*  xfce4-cpu-freq-plugin - panel plugin for cpu informations
 *
 *  Copyright (c) 2006 Thomas Schreck <shrek@xfce.org>
 *  Copyright (c) 2010,2011 Florian Rivoal <frivoal@xfce.org>
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

#define SPACING           2  /* Space between the widgets */
#define BORDER            1  /* Space between the frame and the widgets */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libxfce4util/libxfce4util.h>
#include <libxfce4ui/libxfce4ui.h>
#ifndef _
# include <libintl.h>
# define _(String) gettext (String)
#endif

#include "xfce4-cpufreq-plugin.h"
#ifdef __linux__
#include "xfce4-cpufreq-linux.h"
#endif /* __linux__ */
#include "xfce4-cpufreq-configure.h"
#include "xfce4-cpufreq-overview.h"
#include "xfce4-cpufreq-utils.h"

gboolean
cpufreq_update_label (CpuInfo *cpu)
{
	gchar *label, *freq;

	if (!cpuFreq->options->show_label_governor && !cpuFreq->options->show_label_freq)
		return TRUE;
	
	gint size = xfce_panel_plugin_get_size (cpuFreq->plugin);
	gboolean both = cpu->cur_governor != NULL && cpuFreq->options->show_label_freq && cpuFreq->options->show_label_governor;

	gchar *txt_size = both ?
	                       (size <= 38 ? (size <= 28 ? "<span size=\"xx-small\">" : "<span size=\"x-small\">") : "<span>") :
			       (size <= 19 ? "<span size=\"x-small\">" : "<span>"); 

	freq = cpufreq_get_human_readable_freq (cpu->cur_freq);
	label = g_strconcat (txt_size,

		cpuFreq->options->show_label_freq ? freq : "",
		
		both ? (size <= 25 ? " " : "\n") : "",
	
		cpu->cur_governor != NULL &&
		cpuFreq->options->show_label_governor ? cpu->cur_governor : "",

		"</span>",
		NULL);

	if (strcmp(label,""))
	{
		gtk_label_set_markup (GTK_LABEL(cpuFreq->label), label);
		if (xfce_panel_plugin_get_orientation (cpuFreq->plugin) == GTK_ORIENTATION_VERTICAL)
			gtk_label_set_angle (GTK_LABEL(cpuFreq->label), -90);
		else
			gtk_label_set_angle (GTK_LABEL(cpuFreq->label), 0);
		gtk_widget_show (cpuFreq->label);
	}
	else
	{
		gtk_widget_hide (cpuFreq->label);
	}

	g_free (freq);
	g_free (label);
	return TRUE;
}

gboolean
cpufreq_update_tooltip (CpuInfo *cpu)
{
	gchar *tooltip_msg, *freq;

	freq = cpufreq_get_human_readable_freq (cpu->cur_freq);
	if (cpuFreq->options->show_label_governor && cpuFreq->options->show_label_freq)
		tooltip_msg = g_strdup_printf (ngettext("%d cpu available", "%d cpus available", cpuFreq->cpus->len), cpuFreq->cpus->len);
	else
		tooltip_msg = g_strconcat (!cpuFreq->options->show_label_freq ? _("Frequency: ") : "",
			!cpuFreq->options->show_label_freq ? freq : "",
			
			cpu->cur_governor != NULL && 
			!cpuFreq->options->show_label_freq && !cpuFreq->options->show_label_governor ? "\n" : "",
			
			cpu->cur_governor != NULL &&
			!cpuFreq->options->show_label_governor ? _("Governor: ") : "",
			cpu->cur_governor != NULL &&
			!cpuFreq->options->show_label_governor ? cpu->cur_governor : "",
			NULL);

	gtk_tooltips_set_tip (cpuFreq->tooltip, cpuFreq->ebox, tooltip_msg, NULL);

	g_free (freq);
	g_free (tooltip_msg);
	return TRUE;
}

gboolean
cpufreq_update_plugin (void)
{
	gint i;
	for (i = 0; i < cpuFreq->cpus->len; i++)
	{
		CpuInfo *cpu = g_ptr_array_index (cpuFreq->cpus, i);
		if (cpufreq_update_label (cpu)   == FALSE)
			return FALSE;
		if (cpufreq_update_tooltip (cpu) == FALSE)
			return FALSE;
	}
	return TRUE;
}

void
cpufreq_restart_timeout (void)
{
#ifdef __linux__
	g_source_remove (cpuFreq->timeoutHandle);
	cpuFreq->timeoutHandle = g_timeout_add_seconds (
			cpuFreq->options->timeout,
			(GSourceFunc)cpufreq_update_cpus,
			NULL);
#endif
}

static void
cpufreq_orientation_changed (XfcePanelPlugin *plugin, GtkOrientation orientation, CpuFreqPlugin *cpufreq)
{
	gtk_orientable_set_orientation (GTK_ORIENTABLE (cpufreq->box), orientation);
	cpufreq_update_plugin ();
}

void
cpufreq_update_icon (CpuFreqPlugin *cpufreq)
{
	if (cpufreq->icon)
	{
		gtk_widget_destroy (cpufreq->icon);
		cpufreq->icon = NULL;
	}

	if(cpufreq->options->show_icon)
	{

		GdkPixbuf *buf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
				                                     "xfce4-cpufreq-plugin", cpufreq->icon_size, 0, NULL);
		if (buf)
		{
			cpufreq->icon = gtk_image_new_from_pixbuf (buf);
			g_object_unref (G_OBJECT (buf));
		}
		else
		{
			cpufreq->icon = gtk_image_new_from_icon_name ("xfce4-cpufreq-plugin", GTK_ICON_SIZE_BUTTON);
		}
		gtk_box_pack_start (GTK_BOX (cpufreq->box), cpufreq->icon, FALSE, FALSE, 0);
		gtk_widget_show (cpufreq->icon);
	}
}

void
cpufreq_prepare_label (CpuFreqPlugin *cpufreq)
{
	if (cpufreq->label)
	{
		gtk_widget_destroy (cpufreq->label);
		cpufreq->label = NULL;
	}
	if (cpuFreq->options->show_label_freq || cpuFreq->options->show_label_governor)
	{
		cpuFreq->label = gtk_label_new (NULL);
		gtk_box_pack_end (GTK_BOX (cpufreq->box), cpuFreq->label, FALSE, FALSE, 0);
	}
}

static void
cpufreq_widgets (void)
{
	GtkOrientation	orientation;

	orientation = xfce_panel_plugin_get_orientation (cpuFreq->plugin);
	cpuFreq->icon_size = xfce_panel_plugin_get_size (cpuFreq->plugin) - 4;

	cpuFreq->ebox = gtk_event_box_new ();
	xfce_panel_plugin_add_action_widget (cpuFreq->plugin, cpuFreq->ebox);
	gtk_container_add (GTK_CONTAINER (cpuFreq->plugin), cpuFreq->ebox);

	cpuFreq->frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (cpuFreq->frame),cpuFreq->options->show_frame ? GTK_SHADOW_IN : GTK_SHADOW_NONE);
	gtk_container_add (GTK_CONTAINER (cpuFreq->ebox), cpuFreq->frame);
	gtk_widget_show (cpuFreq->frame);

	cpuFreq->box = gtk_hbox_new (FALSE, SPACING);
	gtk_container_set_border_width (GTK_CONTAINER (cpuFreq->box), BORDER);
	gtk_container_add (GTK_CONTAINER (cpuFreq->frame), cpuFreq->box);

	gtk_tooltips_set_tip (cpuFreq->tooltip, cpuFreq->ebox, "", NULL);

	cpufreq_update_icon (cpuFreq);

	cpufreq_prepare_label (cpuFreq);

	g_signal_connect (cpuFreq->ebox, "button-press-event", G_CALLBACK (cpufreq_overview), cpuFreq);

	cpufreq_orientation_changed (cpuFreq->plugin, orientation, cpuFreq);
	gtk_widget_show (cpuFreq->box);
	gtk_widget_show (cpuFreq->ebox);

	cpufreq_update_plugin ();
}

static void
cpufreq_read_config (void)
{
	XfceRc *rc;
	gchar  *file;

	file = xfce_panel_plugin_save_location (cpuFreq->plugin, FALSE);

	if (G_UNLIKELY (!file))
		return;

	rc = xfce_rc_simple_open (file, FALSE);
	g_free (file);

	cpuFreq->options->timeout             = xfce_rc_read_int_entry  (rc, "timeout", 1);
	if (cpuFreq->options->timeout > TIMEOUT_MAX || cpuFreq->options->timeout < TIMEOUT_MIN)
		cpuFreq->options->timeout = TIMEOUT_MIN;
	cpuFreq->options->show_cpu            = xfce_rc_read_int_entry  (rc, "show_cpu",  0);
	cpuFreq->options->show_frame          = xfce_rc_read_bool_entry (rc, "show_frame",  TRUE);
	cpuFreq->options->show_icon           = xfce_rc_read_bool_entry (rc, "show_icon",  TRUE);
	cpuFreq->options->show_label_freq     = xfce_rc_read_bool_entry (rc, "show_label_freq", TRUE);
	cpuFreq->options->show_label_governor =	xfce_rc_read_bool_entry (rc, "show_label_governor", TRUE);
	cpuFreq->options->show_warning        =	xfce_rc_read_bool_entry (rc, "show_warning", TRUE);

	xfce_rc_close (rc);
}

void
cpufreq_write_config (XfcePanelPlugin *plugin)
{
	XfceRc *rc;
	gchar  *file;

	file = xfce_panel_plugin_save_location (plugin, TRUE);

	if (G_UNLIKELY (!file))
		return;

	rc = xfce_rc_simple_open (file, FALSE);
	g_free(file);

	xfce_rc_write_int_entry	 (rc, "timeout",             cpuFreq->options->timeout);
	xfce_rc_write_int_entry	 (rc, "show_cpu",            cpuFreq->options->show_cpu);
	xfce_rc_write_bool_entry (rc, "show_frame",          cpuFreq->options->show_frame);
	xfce_rc_write_bool_entry (rc, "show_icon",           cpuFreq->options->show_icon);
	xfce_rc_write_bool_entry (rc, "show_label_freq",     cpuFreq->options->show_label_freq);
	xfce_rc_write_bool_entry (rc, "show_label_governor", cpuFreq->options->show_label_governor);
	xfce_rc_write_bool_entry (rc, "show_warning",        cpuFreq->options->show_warning);

	xfce_rc_close (rc);
}

static void
cpufreq_free (XfcePanelPlugin *plugin)
{
	gint i;

	if (cpuFreq->timeoutHandle)
		g_source_remove (cpuFreq->timeoutHandle);

	for (i = 0; i < cpuFreq->cpus->len; i++)
	{
		CpuInfo *cpu = g_ptr_array_index (cpuFreq->cpus, i);
		g_ptr_array_remove_fast (cpuFreq->cpus, cpu);
		g_free (cpu->cur_governor);
		g_list_free (cpu->available_freqs);
		g_list_free (cpu->available_governors);
		g_free (cpu);
	}

	gtk_tooltips_set_tip (cpuFreq->tooltip, cpuFreq->ebox, NULL, NULL);
	g_object_unref (cpuFreq->tooltip);
	g_ptr_array_free (cpuFreq->cpus, TRUE);
	cpuFreq->plugin = NULL;
	g_free (cpuFreq);
}

static gboolean
cpufreq_set_size (XfcePanelPlugin *plugin, gint size, CpuFreqPlugin *cpufreq)
{
	cpufreq->icon_size = size - 4;
	cpufreq_update_icon (cpufreq);
	cpufreq_update_plugin ();

	return TRUE;
}

static void
cpufreq_construct (XfcePanelPlugin *plugin)
{
	xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

	cpuFreq 	  = g_new0 (CpuFreqPlugin, 1);
	cpuFreq->options  = g_new0 (CpuFreqPluginOptions, 1);
	cpuFreq->plugin   = plugin;
	cpuFreq->tooltip = gtk_tooltips_new ();
	g_object_ref (cpuFreq->tooltip);
	cpuFreq->cpus    = g_ptr_array_new ();

	cpufreq_read_config ();

#ifdef __linux__
	if (cpufreq_linux_init () == FALSE)
		xfce_dialog_show_error (NULL, NULL, _("Your system is not configured correctly to support cpu frequency scaling !"));

	gtk_widget_set_size_request (GTK_WIDGET (plugin), -1, -1);
	cpufreq_widgets ();

	cpuFreq->timeoutHandle = g_timeout_add_seconds (
			cpuFreq->options->timeout,
			(GSourceFunc) cpufreq_update_cpus,
			NULL);
#else
	xfce_dialog_show_error (NULL, NULL, _("Your system is not supported yet !"));
#endif /* __linux__ */

	g_signal_connect (plugin, "free-data", G_CALLBACK (cpufreq_free),
			  NULL);
	g_signal_connect (plugin, "save", G_CALLBACK (cpufreq_write_config),
			  NULL);
	g_signal_connect (plugin, "size-changed",
			  G_CALLBACK (cpufreq_set_size), cpuFreq);
	g_signal_connect (plugin, "orientation-changed",
			  G_CALLBACK (cpufreq_orientation_changed), cpuFreq);

	/* the configure and about menu items are hidden by default */
	xfce_panel_plugin_menu_show_configure (plugin);
	g_signal_connect (plugin, "configure-plugin",
			  G_CALLBACK (cpufreq_configure), NULL);
}

XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL (cpufreq_construct);
