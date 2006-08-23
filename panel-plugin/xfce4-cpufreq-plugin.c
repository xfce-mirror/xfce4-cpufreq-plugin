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

#define SPACING           2  /* Space between the widgets */
#define FRAME_BORDER      2  /* Space between the frame and the panel */
#define BORDER            1  /* Space between the frame and the widgets */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>

#include "xfce4-cpufreq-plugin.h"
#include "xfce4-cpufreq-linux.h"
#include "xfce4-cpufreq-configure.h"
#include "xfce4-cpufreq-overview.h"
#include "xfce4-cpufreq-utils.h"

gboolean
cpufreq_update_label (CpuInfo *cpu)
{
	gchar *label, *freq;
	gboolean small;

	if (!cpuFreq->options->show_label_governor && !cpuFreq->options->show_label_freq)
		return TRUE;
	
	freq = cpufreq_get_human_readable_freq (cpu->cur_freq);
	label = g_strconcat (small ? "<span size=\"xx-small\">" : "<span size=\"x-small\">",

		cpuFreq->options->show_label_freq ? freq : "",
		cpuFreq->options->show_label_freq && cpuFreq->options->show_label_governor ? "\n" : "",
		cpuFreq->options->show_label_governor ? cpu->cur_governor : "",
		"</span>",
		NULL);

	gtk_label_set_label (GTK_LABEL(cpuFreq->label), label);

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
		tooltip_msg = g_strdup_printf (_("CPU Frequency Plugin"));
	else
		tooltip_msg = g_strconcat (!cpuFreq->options->show_label_freq ? _("Frequency: ") : "",
			!cpuFreq->options->show_label_freq ? freq : "",
			!cpuFreq->options->show_label_freq && !cpuFreq->options->show_label_governor ? "\n" : "",
			!cpuFreq->options->show_label_governor ? _("Governor: ") : "",
			!cpuFreq->options->show_label_governor ? cpu->cur_governor : "",
			NULL);

	gtk_tooltips_set_tip (cpuFreq->tooltip, cpuFreq->ebox, tooltip_msg, NULL);

	g_free (freq);
	g_free (tooltip_msg);
	return TRUE;
}

gboolean
cpufreq_update_icon (CpuInfo *cpu)
{
	gchar     *cpu_icon_name;
	guint     psize, isize;
	GdkPixbuf *pixbuf = NULL;

	if (!cpuFreq->options->show_icon)
		return TRUE;

	if (cpu->cur_freq == cpu->max_freq)
		cpu_icon_name = g_strdup ("cpu");
	else
	{
		if (cpu->cur_freq == cpu->min_freq)
			cpu_icon_name = g_strdup ("cpu");
		else
			cpu_icon_name = g_strdup ("cpu");
	}

	psize = xfce_panel_plugin_get_size (cpuFreq->plugin);
	isize = psize - (2 * BORDER) - 
		(2 * (psize > 26 ? 2 : 0)) - 
		(2 * MAX (cpuFreq->frame->style->xthickness, cpuFreq->frame->style->ythickness));

	pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (), cpu_icon_name, isize , 0, NULL);

	if (pixbuf)
	{
		gtk_image_set_from_pixbuf (GTK_IMAGE (cpuFreq->icon), pixbuf);
		g_object_unref (G_OBJECT (pixbuf));
	}
	else
		return FALSE;

	g_free (cpu_icon_name);

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
		if (cpufreq_update_icon (cpu) == FALSE)
			return FALSE;
	}
	return TRUE;
}

gboolean
cpufreq_restart_timeout (void)
{
	g_source_remove (cpuFreq->timeoutHandle);
	cpuFreq->timeoutHandle = g_timeout_add (
			100,
			(GSourceFunc) cpufreq_update_cpus,
			NULL);
}

gboolean
cpufreq_widgets (void)
{
	gint		size;
	gchar		*tooltip_msg;
	GtkWidget	*box;
	GtkOrientation	orientation;

	orientation = xfce_panel_plugin_get_orientation (cpuFreq->plugin);
	size	    = xfce_panel_plugin_get_size (cpuFreq->plugin);

	if (cpuFreq->ebox)
		gtk_widget_destroy (cpuFreq->ebox);
	cpuFreq->ebox = gtk_event_box_new ();
	
	cpuFreq->frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (cpuFreq->frame),cpuFreq->options->show_frame ? GTK_SHADOW_IN : GTK_SHADOW_NONE);
	gtk_container_add (GTK_CONTAINER (cpuFreq->ebox), cpuFreq->frame);
	gtk_container_set_border_width (GTK_CONTAINER (cpuFreq->frame),	size > 26 ? FRAME_BORDER : 0);

	if (orientation == GTK_ORIENTATION_HORIZONTAL)
		box = gtk_hbox_new (FALSE, SPACING);
	else
		box = gtk_vbox_new (FALSE, SPACING);

	gtk_container_set_border_width (GTK_CONTAINER (box), BORDER);
	gtk_container_add (GTK_CONTAINER (cpuFreq->frame), box);

	tooltip_msg = _("CPU Frequency Plugin");
	gtk_tooltips_set_tip (cpuFreq->tooltip, cpuFreq->ebox, tooltip_msg, NULL);

	if(cpuFreq->options->show_icon)
	{
		cpuFreq->icon = gtk_image_new_from_icon_name ("cpu", size);
		gtk_box_pack_start (GTK_BOX (box), cpuFreq->icon, FALSE, FALSE, 0);
	}

	if (cpuFreq->options->show_label_freq || cpuFreq->options->show_label_governor)
	{
		cpuFreq->label = gtk_label_new ("");
		gtk_box_pack_start (GTK_BOX (box), cpuFreq->label, FALSE, FALSE, 0);
		gtk_label_set_use_markup (GTK_LABEL (cpuFreq->label), TRUE);
	}

	g_signal_connect (cpuFreq->ebox, "button-press-event", G_CALLBACK (cpufreq_overview), cpuFreq);  

	gtk_widget_show_all (cpuFreq->ebox);
	gtk_container_add (GTK_CONTAINER (cpuFreq->plugin), cpuFreq->ebox);
	xfce_panel_plugin_add_action_widget (cpuFreq->plugin, cpuFreq->ebox);
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

	cpuFreq->options->timeout             = xfce_rc_read_int_entry  (rc, "timeout", 100);
	cpuFreq->options->show_cpu            = xfce_rc_read_int_entry  (rc, "show_cpu",  0);
	cpuFreq->options->show_frame          = xfce_rc_read_bool_entry (rc, "show_frame",  TRUE);
	cpuFreq->options->show_icon           = xfce_rc_read_bool_entry (rc, "show_icon",  TRUE);
	cpuFreq->options->show_label_freq     = xfce_rc_read_bool_entry (rc, "show_label_freq", TRUE);
	cpuFreq->options->show_label_governor =	xfce_rc_read_bool_entry (rc, "show_label_governor", TRUE);

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

	xfce_rc_close (rc);
}

static void
cpufreq_free (XfcePanelPlugin *plugin)
{
	gint i;

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

	gksu_context_free (cpuFreq->gksu_ctx);

	gtk_tooltips_set_tip (cpuFreq->tooltip, cpuFreq->ebox, NULL, NULL);
	g_object_unref (cpuFreq->tooltip);

	gksu_context_free (cpuFreq->gksu_ctx);
	cpuFreq->plugin = NULL;
	g_free (cpuFreq);
}

static gboolean
cpufreq_set_size (XfcePanelPlugin *plugin, gint wsize)
{
	if (xfce_panel_plugin_get_orientation (plugin) ==
			GTK_ORIENTATION_HORIZONTAL)
		gtk_widget_set_size_request (GTK_WIDGET (plugin), -1, wsize);
	else
		gtk_widget_set_size_request (GTK_WIDGET (plugin), wsize, -1);

	return TRUE;
}

static void
cpufreq_orientation_changed (XfcePanelPlugin *plugin,
			     GtkOrientation orientation)
{
	if (cpufreq_widgets () == FALSE)
		xfce_err (_("Could not create widgets !"));
}

static void
cpufreq_construct (XfcePanelPlugin *plugin)
{
	xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

	cpuFreq 	  = g_new0 (CpuFreqPlugin, 1);
	cpuFreq->options  = g_new0 (CpuFreqPluginOptions, 1);
	cpuFreq->plugin   = plugin;

	cpuFreq->gksu_ctx = gksu_context_new ();

	cpuFreq->tooltip = gtk_tooltips_new ();
	g_object_ref (cpuFreq->tooltip);

	cpuFreq->cpus    = g_ptr_array_new ();

	cpufreq_read_config ();

	if (cpufreq_linux_init () == FALSE)
		xfce_err (_("Could not initialize linux backend !"));

	if (cpufreq_widgets () == FALSE)
		xfce_err (_("Could not create widgets !"));

	cpuFreq->timeoutHandle = g_timeout_add (
			100,
			(GSourceFunc) cpufreq_update_cpus,
			NULL);

	g_signal_connect (plugin, "free-data", G_CALLBACK (cpufreq_free),
			  NULL);
	g_signal_connect (plugin, "save", G_CALLBACK (cpufreq_write_config),
			  NULL);
	g_signal_connect (plugin, "size-changed",
			  G_CALLBACK (cpufreq_set_size), NULL);
	g_signal_connect (plugin, "orientation-changed",
			  G_CALLBACK (cpufreq_orientation_changed), NULL);

	/* the configure and about menu items are hidden by default */
	xfce_panel_plugin_menu_show_configure (plugin);
	g_signal_connect (plugin, "configure-plugin",
			  G_CALLBACK (cpufreq_configure), NULL);
}

XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL (cpufreq_construct);
