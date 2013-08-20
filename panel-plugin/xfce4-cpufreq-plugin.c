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


CpuInfo *
cpufreq_cpus_calc_min (void)
{
	guint freq = 0;
	gint i;

	for (i = 0; i < cpuFreq->cpus->len; i++) {
		CpuInfo *cpu = g_ptr_array_index (cpuFreq->cpus, i);
		if (freq > cpu->cur_freq || i == 0)
			freq = cpu->cur_freq;
	}

	cpuinfo_free (cpuFreq->cpu_min);
	cpuFreq->cpu_min = g_new0 (CpuInfo, 1);
	cpuFreq->cpu_min->cur_freq = freq;
	cpuFreq->cpu_min->cur_governor = g_strdup (_("current min"));
	return cpuFreq->cpu_min;
}

CpuInfo *
cpufreq_cpus_calc_avg (void)
{
	guint freq = 0;
	gint i;

	for (i = 0; i < cpuFreq->cpus->len; i++) {
		CpuInfo *cpu = g_ptr_array_index (cpuFreq->cpus, i);
		freq += cpu->cur_freq;
	}

	freq /= cpuFreq->cpus->len;
	cpuinfo_free (cpuFreq->cpu_avg);
	cpuFreq->cpu_avg = g_new0 (CpuInfo, 1);
	cpuFreq->cpu_avg->cur_freq = freq;
	cpuFreq->cpu_avg->cur_governor = g_strdup (_("current avg"));
	return cpuFreq->cpu_avg;
}

CpuInfo *
cpufreq_cpus_calc_max (void)
{
	guint freq = 0;
	gint i;

	for (i = 0; i < cpuFreq->cpus->len; i++) {
		CpuInfo *cpu = g_ptr_array_index (cpuFreq->cpus, i);
		if (freq < cpu->cur_freq)
			freq = cpu->cur_freq;
	}
	cpuinfo_free (cpuFreq->cpu_max);
	cpuFreq->cpu_max = g_new0 (CpuInfo, 1);
	cpuFreq->cpu_max->cur_freq = freq;
	cpuFreq->cpu_max->cur_governor = g_strdup (_("current max"));
	return cpuFreq->cpu_max;
}

void
cpufreq_label_set_font (void)
{
	PangoFontDescription *desc = NULL;

	if (G_UNLIKELY (cpuFreq->label == NULL))
		return;

	if (cpuFreq->options->fontname)
		desc = pango_font_description_from_string (cpuFreq->options->fontname);

	gtk_widget_modify_font (cpuFreq->label, desc);
    pango_font_description_free (desc);
}

gboolean
cpufreq_update_label (CpuInfo *cpu)
{
	gchar *label, *freq;
	gint size, both;

	if (!cpuFreq->options->show_label_governor &&
		!cpuFreq->options->show_label_freq) {
		if (cpuFreq->label != NULL)
			gtk_widget_hide (cpuFreq->label);
		return TRUE;
	}
	
	both = cpu->cur_governor != NULL &&
		cpuFreq->options->show_label_freq &&
		cpuFreq->options->show_label_governor;

	freq = cpufreq_get_human_readable_freq (cpu->cur_freq);
	label = g_strconcat
		(cpuFreq->options->show_label_freq ? freq : "",
		 both ? (cpuFreq->options->one_line ? " " : "\n") : "",
		 cpu->cur_governor != NULL &&
		 cpuFreq->options->show_label_governor ? cpu->cur_governor : "",
		 NULL);

	gtk_label_set_text (GTK_LABEL (cpuFreq->label), label);

	if (strcmp(label, ""))
	{
		if (cpuFreq->panel_mode == XFCE_PANEL_PLUGIN_MODE_VERTICAL)
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

static void
cpufreq_widgets_layout (void)
{
	GtkRequisition icon_size, label_size;
	GtkOrientation orientation;
	gboolean small = cpuFreq->options->keep_compact;
	gboolean resized = FALSE;
	gboolean hide_label = (!cpuFreq->options->show_label_freq &&
						   !cpuFreq->options->show_label_governor);
	gint panel_size, pos = 1;
	gint lw = 0, lh = 0, iw = 0, ih = 0;

	small = (hide_label ? TRUE : cpuFreq->options->keep_compact);

	switch (cpuFreq->panel_mode) {
	case XFCE_PANEL_PLUGIN_MODE_HORIZONTAL:
		orientation = small ? GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL;
		xfce_panel_plugin_set_small (cpuFreq->plugin, small);
		break;
	case XFCE_PANEL_PLUGIN_MODE_VERTICAL:
		orientation = small ? GTK_ORIENTATION_VERTICAL : GTK_ORIENTATION_HORIZONTAL;
		xfce_panel_plugin_set_small (cpuFreq->plugin, small);
		break;
	case XFCE_PANEL_PLUGIN_MODE_DESKBAR:
		orientation = small ? GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL;
		xfce_panel_plugin_set_small (cpuFreq->plugin, FALSE);
		break;
	}

	/* always set plugin small state when only icon is shown */
	if (hide_label)
		xfce_panel_plugin_set_small (cpuFreq->plugin, TRUE);

	/* check if the label fits below the icon, else put them side by side */
	panel_size = xfce_panel_plugin_get_size(cpuFreq->plugin);
	if (GTK_IS_WIDGET(cpuFreq->label) && ! hide_label) {
		gtk_widget_size_request (cpuFreq->label, &label_size);
		lw = label_size.width;
		lh = label_size.height;
	}
	if (GTK_IS_WIDGET(cpuFreq->icon)) {
		gtk_widget_size_request (cpuFreq->icon, &icon_size);
		iw = icon_size.width;
		ih = icon_size.height;
	}
	if (cpuFreq->panel_mode == XFCE_PANEL_PLUGIN_MODE_HORIZONTAL &&
		orientation == GTK_ORIENTATION_VERTICAL &&
		lh + ih + BORDER * 2 >= panel_size) {
		orientation = GTK_ORIENTATION_HORIZONTAL;
		resized = TRUE;
	} else if (orientation == GTK_ORIENTATION_HORIZONTAL &&
			   lw + iw + BORDER * 2 >= panel_size &&
			   (cpuFreq->panel_mode == XFCE_PANEL_PLUGIN_MODE_DESKBAR ||
				!small)) {
		orientation = GTK_ORIENTATION_VERTICAL;
		resized = TRUE;
	}

	gtk_orientable_set_orientation (GTK_ORIENTABLE (cpuFreq->box), orientation);

	if (small) {
		if (orientation == GTK_ORIENTATION_VERTICAL) {
			if (cpuFreq->icon)
				gtk_misc_set_alignment (GTK_MISC (cpuFreq->icon), 0.5, 0);
			if (cpuFreq->label)
				gtk_misc_set_alignment (GTK_MISC (cpuFreq->label), 0.5, 0);
		} else {
			if (cpuFreq->icon)
				gtk_misc_set_alignment (GTK_MISC (cpuFreq->icon), 0, 0.5);
			if (cpuFreq->label)
				gtk_misc_set_alignment (GTK_MISC (cpuFreq->label), 0, 0.5);
		}
		if (cpuFreq->label)
			gtk_label_set_justify (GTK_LABEL (cpuFreq->label),
								   resized
								   ? GTK_JUSTIFY_CENTER : GTK_JUSTIFY_LEFT);

		if (cpuFreq->icon)
			gtk_box_set_child_packing (GTK_BOX (cpuFreq->box),
									   cpuFreq->icon,
									   FALSE, FALSE, 0, GTK_PACK_START);
	} else {
		if (orientation == GTK_ORIENTATION_VERTICAL) {
			if (cpuFreq->icon)
				gtk_misc_set_alignment (GTK_MISC (cpuFreq->icon), 0.5, 1.0);
			if (cpuFreq->label)
				gtk_misc_set_alignment (GTK_MISC (cpuFreq->label), 0.5, 0);
		} else {
			if (cpuFreq->icon)
				gtk_misc_set_alignment (GTK_MISC (cpuFreq->icon), 0, 0.5);
			if (cpuFreq->label)
				gtk_misc_set_alignment (GTK_MISC (cpuFreq->label), 1.0, 0.5);
			pos = resized ? 1 : 0;
		}
		if (cpuFreq->label)
			gtk_label_set_justify (GTK_LABEL (cpuFreq->label),
								   resized
								   ? GTK_JUSTIFY_LEFT : GTK_JUSTIFY_CENTER);

		if (cpuFreq->icon)
			gtk_box_set_child_packing (GTK_BOX (cpuFreq->box),
									   cpuFreq->icon,
									   TRUE, TRUE, 0, GTK_PACK_START);
	}
	if (cpuFreq->label)
		gtk_box_reorder_child (GTK_BOX (cpuFreq->box), cpuFreq->label, pos);

	cpuFreq->layout_changed = FALSE;
}

CpuInfo *
cpufreq_current_cpu ()
{
	CpuInfo *cpu = NULL;
	if (cpuFreq->options->show_cpu < cpuFreq->cpus->len)
		cpu = g_ptr_array_index (cpuFreq->cpus, cpuFreq->options->show_cpu);
	else if (cpuFreq->options->show_cpu == CPU_MIN)
		cpu = cpufreq_cpus_calc_min ();
	else if (cpuFreq->options->show_cpu == CPU_AVG)
		cpu = cpufreq_cpus_calc_avg ();
	else if (cpuFreq->options->show_cpu == CPU_MAX)
		cpu = cpufreq_cpus_calc_max ();
	return cpu;
}

gboolean
cpufreq_update_plugin (void)
{
	CpuInfo *cpu;
	gboolean ret;

	cpu = cpufreq_current_cpu ();
	ret = cpufreq_update_label (cpu);

	if (cpuFreq->layout_changed) {
		cpufreq_label_set_font ();
		cpufreq_widgets_layout ();
	}

	return ret;
}

static gboolean
cpufreq_update_tooltip (GtkWidget *widget,
						gint x,
						gint y,
						gboolean keyboard_mode,
						GtkTooltip *tooltip,
						CpuFreqPlugin *cpufreq)
{
	CpuInfo *cpu;
	gchar *tooltip_msg, *freq = NULL;

	cpu = cpufreq_current_cpu ();

	if (G_UNLIKELY(cpu == NULL)) {
		tooltip_msg = g_strdup (_("No CPU information available."));
	} else {
		freq = cpufreq_get_human_readable_freq (cpu->cur_freq);
		if (cpuFreq->options->show_label_governor &&
			cpuFreq->options->show_label_freq)
			tooltip_msg =
				g_strdup_printf (ngettext ("%d cpu available",
										   "%d cpus available",
										   cpuFreq->cpus->len),
								 cpuFreq->cpus->len);
		else
			tooltip_msg =
				g_strconcat
				(!cpuFreq->options->show_label_freq ? _("Frequency: ") : "",
				 !cpuFreq->options->show_label_freq ? freq : "",

				 cpu->cur_governor != NULL &&
				 !cpuFreq->options->show_label_freq &&
				 !cpuFreq->options->show_label_governor ? "\n" : "",

				 cpu->cur_governor != NULL &&
				 !cpuFreq->options->show_label_governor ? _("Governor: ") : "",
				 cpu->cur_governor != NULL &&
				 !cpuFreq->options->show_label_governor ? cpu->cur_governor : "",
				 NULL);
	}

	gtk_tooltip_set_text (tooltip, tooltip_msg);

	g_free (freq);
	g_free (tooltip_msg);
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
cpufreq_mode_changed (XfcePanelPlugin *plugin, XfcePanelPluginMode mode, CpuFreqPlugin *cpufreq)
{
	cpuFreq->panel_mode = mode;
	cpuFreq->layout_changed = TRUE;
	cpufreq_update_plugin ();
}

void
cpufreq_update_icon (CpuFreqPlugin *cpufreq)
{
	if (cpufreq->icon) {
		gtk_widget_destroy (cpufreq->icon);
		cpufreq->icon = NULL;
	}

	if (cpufreq->options->show_icon) {
		GdkPixbuf *buf;
		guint nrows;
		gint panel_size;

		panel_size = xfce_panel_plugin_get_size (cpufreq->plugin);
		nrows = xfce_panel_plugin_get_nrows (cpuFreq->plugin);

		cpufreq->icon_size = panel_size / nrows;
		if (cpufreq->options->keep_compact ||
			(!cpufreq->options->show_label_freq &&
			 !cpufreq->options->show_label_governor))
			cpufreq->icon_size -= 4;

		buf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
										"xfce4-cpufreq-plugin",
										cpufreq->icon_size, 0, NULL);
		if (buf) {
			cpufreq->icon = gtk_image_new_from_pixbuf (buf);
			g_object_unref (G_OBJECT (buf));
		} else {
			cpufreq->icon = gtk_image_new_from_icon_name
				("xfce4-cpufreq-plugin", GTK_ICON_SIZE_BUTTON);
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
		gtk_box_pack_start (GTK_BOX (cpufreq->box), cpuFreq->label, TRUE, TRUE, 0);
	}
}

static void
cpufreq_widgets (void)
{
	cpuFreq->icon_size = xfce_panel_plugin_get_size (cpuFreq->plugin);
	cpuFreq->icon_size /= xfce_panel_plugin_get_nrows (cpuFreq->plugin);

	/* create panel toggle button which will contain all other widgets */
	cpuFreq->button = xfce_create_panel_toggle_button ();
	xfce_panel_plugin_add_action_widget (cpuFreq->plugin, cpuFreq->button);
	gtk_container_add (GTK_CONTAINER (cpuFreq->plugin), cpuFreq->button);

	cpuFreq->box = gtk_hbox_new (FALSE, SPACING);
	gtk_container_set_border_width (GTK_CONTAINER (cpuFreq->box), BORDER);
	gtk_container_add (GTK_CONTAINER (cpuFreq->button), cpuFreq->box);

	cpufreq_update_icon (cpuFreq);

	cpufreq_prepare_label (cpuFreq);

	g_signal_connect (cpuFreq->button, "button-press-event",
					  G_CALLBACK (cpufreq_overview), cpuFreq);

	/* activate panel widget tooltip */
	g_object_set (G_OBJECT (cpuFreq->button), "has-tooltip", TRUE, NULL);
	g_signal_connect (G_OBJECT (cpuFreq->button), "query-tooltip",
					  G_CALLBACK (cpufreq_update_tooltip), cpuFreq);

	cpufreq_mode_changed (cpuFreq->plugin,
						  xfce_panel_plugin_get_mode (cpuFreq->plugin),
						  cpuFreq);

	gtk_widget_show (cpuFreq->box);
	gtk_widget_show (cpuFreq->button);

	cpufreq_update_plugin ();
}

static void
cpufreq_read_config (void)
{
	XfceRc *rc;
	gchar  *file;
	const gchar *value;

	file = xfce_panel_plugin_save_location (cpuFreq->plugin, FALSE);

	if (G_UNLIKELY (!file))
		return;

	rc = xfce_rc_simple_open (file, FALSE);
	g_free (file);

	cpuFreq->options->timeout             = xfce_rc_read_int_entry  (rc, "timeout", 1);
	if (cpuFreq->options->timeout > TIMEOUT_MAX || cpuFreq->options->timeout < TIMEOUT_MIN)
		cpuFreq->options->timeout = TIMEOUT_MIN;
	cpuFreq->options->show_cpu            = xfce_rc_read_int_entry  (rc, "show_cpu",  0);
	cpuFreq->options->show_icon           = xfce_rc_read_bool_entry (rc, "show_icon",  TRUE);
	cpuFreq->options->show_label_freq     = xfce_rc_read_bool_entry (rc, "show_label_freq", TRUE);
	cpuFreq->options->show_label_governor =	xfce_rc_read_bool_entry (rc, "show_label_governor", TRUE);
	cpuFreq->options->show_warning        =	xfce_rc_read_bool_entry (rc, "show_warning", TRUE);
	cpuFreq->options->keep_compact        =	xfce_rc_read_bool_entry (rc, "keep_compact", FALSE);
	cpuFreq->options->one_line            =	xfce_rc_read_bool_entry (rc, "one_line", FALSE);

	if (!cpuFreq->options->show_label_freq && !cpuFreq->options->show_label_governor)
		cpuFreq->options->show_icon = TRUE;

	value = xfce_rc_read_entry (rc, "fontname", NULL);
	if (value) {
		g_free (cpuFreq->options->fontname);
		cpuFreq->options->fontname = g_strdup (value);
	}

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
	xfce_rc_write_bool_entry (rc, "show_icon",           cpuFreq->options->show_icon);
	xfce_rc_write_bool_entry (rc, "show_label_freq",     cpuFreq->options->show_label_freq);
	xfce_rc_write_bool_entry (rc, "show_label_governor", cpuFreq->options->show_label_governor);
	xfce_rc_write_bool_entry (rc, "show_warning",        cpuFreq->options->show_warning);
	xfce_rc_write_bool_entry (rc, "keep_compact",        cpuFreq->options->keep_compact);
	xfce_rc_write_bool_entry (rc, "one_line",            cpuFreq->options->one_line);
	if (cpuFreq->options->fontname)
		xfce_rc_write_entry  (rc, "fontname",            cpuFreq->options->fontname);

	xfce_rc_close (rc);
}

void
cpuinfo_free (CpuInfo *cpu)
{
	if (G_UNLIKELY(cpu == NULL))
		return;
	g_free (cpu->cur_governor);
	g_free (cpu->scaling_driver);
	g_list_free (cpu->available_freqs);
	g_list_free_full (cpu->available_governors, g_free);
	g_free (cpu);
}

static void
cpufreq_free (XfcePanelPlugin *plugin)
{
	gint i;

	if (cpuFreq->timeoutHandle)
		g_source_remove (cpuFreq->timeoutHandle);

	g_slice_free (IntelPState, cpuFreq->intel_pstate);

	for (i = 0; i < cpuFreq->cpus->len; i++)
	{
		CpuInfo *cpu = g_ptr_array_index (cpuFreq->cpus, i);
		g_ptr_array_remove_fast (cpuFreq->cpus, cpu);
		cpuinfo_free (cpu);
	}
	g_ptr_array_free (cpuFreq->cpus, TRUE);

	g_free (cpuFreq->options->fontname);
	cpuFreq->plugin = NULL;
	g_free (cpuFreq);
}

static gboolean
cpufreq_set_size (XfcePanelPlugin *plugin, gint size, CpuFreqPlugin *cpufreq)
{
	cpuFreq->icon_size = size / xfce_panel_plugin_get_nrows (cpuFreq->plugin);

	cpuFreq->layout_changed = TRUE;
	cpufreq_update_icon (cpufreq);
	cpufreq_update_plugin ();

	return TRUE;
}

cpufreq_show_about(XfcePanelPlugin *plugin,
				   CpuFreqPlugin *cpufreq)
{
	GdkPixbuf *icon;
	const gchar *auth[] = {
		"Thomas Schreck <shrek@xfce.org>",
		"Florian Rivoal <frivoal@xfce.org>",
		"Harald Judt <h.judt@gmx.at>",
		NULL };
	icon = xfce_panel_pixbuf_from_source("xfce4-cpufreq-plugin", NULL, 48);
	gtk_show_about_dialog
		(NULL,
		 "logo", icon,
		 "license", xfce_get_license_text(XFCE_LICENSE_TEXT_GPL),
		 "version", PACKAGE_VERSION,
		 "program-name", PACKAGE_NAME,
		 "comments", _("Show CPU frequencies and governor"),
		 "website", PLUGIN_WEBSITE,
		 "copyright", _("Copyright (c) 2003-2012\n"),
		 "authors", auth,
		 NULL);

	if (icon)
		g_object_unref(G_OBJECT(icon));
}

static void
cpufreq_construct (XfcePanelPlugin *plugin)
{
	xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

	cpuFreq 	  = g_new0 (CpuFreqPlugin, 1);
	cpuFreq->options  = g_new0 (CpuFreqPluginOptions, 1);
	cpuFreq->plugin   = plugin;
	cpuFreq->cpus    = g_ptr_array_new ();

	cpufreq_read_config ();
	cpuFreq->layout_changed = TRUE;

#ifdef __linux__
	if (cpufreq_linux_init () == FALSE)
		xfce_dialog_show_error (NULL, NULL, _("Your system is not configured correctly to support CPU frequency scaling!"));

	gtk_widget_set_size_request (GTK_WIDGET (plugin), -1, -1);
	cpufreq_widgets ();

	cpuFreq->timeoutHandle = g_timeout_add_seconds (
			cpuFreq->options->timeout,
			(GSourceFunc) cpufreq_update_cpus,
			NULL);
#else
	xfce_dialog_show_error (NULL, NULL, _("Your system is not supported yet!"));
#endif /* __linux__ */

	g_signal_connect (plugin, "free-data", G_CALLBACK (cpufreq_free),
			  NULL);
	g_signal_connect (plugin, "save", G_CALLBACK (cpufreq_write_config),
			  NULL);
	g_signal_connect (plugin, "size-changed",
			  G_CALLBACK (cpufreq_set_size), cpuFreq);
	g_signal_connect (plugin, "mode-changed",
			  G_CALLBACK (cpufreq_mode_changed), cpuFreq);

	/* the configure and about menu items are hidden by default */
	xfce_panel_plugin_menu_show_configure (plugin);
	g_signal_connect (plugin, "configure-plugin",
					  G_CALLBACK (cpufreq_configure), NULL);
	xfce_panel_plugin_menu_show_about(plugin);
	g_signal_connect (G_OBJECT (plugin), "about",
					  G_CALLBACK (cpufreq_show_about), cpuFreq);
}

XFCE_PANEL_PLUGIN_REGISTER (cpufreq_construct);
