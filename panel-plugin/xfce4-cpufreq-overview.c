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
#define BORDER 1

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libxfce4ui/libxfce4ui.h>
#ifndef _
# include <libintl.h>
# define _(String) gettext (String)
#endif

#include "xfce4-cpufreq-plugin.h"
#ifdef __linux__
#include "xfce4-cpufreq-linux.h"
#endif /* __linux__ */
#include "xfce4-cpufreq-overview.h"
#include "xfce4-cpufreq-utils.h"

static void
cpufreq_overview_add (CpuInfo *cpu, guint cpu_number, GtkWidget *dialog_hbox)
{
	gint	  i = 0, j;
	gchar	  *text;
	GtkWidget *hbox, *dialog_vbox, *combo, *label, *icon;
	GtkSizeGroup *sg0, *sg1;
	GList 	  *list;

	dialog_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, BORDER);
	gtk_box_pack_start (GTK_BOX (dialog_hbox), dialog_vbox, TRUE, TRUE, 0);

	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, BORDER);
	gtk_box_pack_start (GTK_BOX (dialog_vbox), hbox, TRUE, TRUE, 0);

	icon = gtk_image_new_from_icon_name ("xfce4-cpufreq-plugin", GTK_ICON_SIZE_BUTTON);
	gtk_misc_set_padding (GTK_MISC (icon), 5, 10);
	gtk_widget_set_halign (icon, GTK_ALIGN_END);
	gtk_widget_set_valign (icon, GTK_ALIGN_CENTER);
	gtk_box_pack_start (GTK_BOX (hbox), icon, TRUE, TRUE, 0);
	text = g_strdup_printf ("<b>CPU %d</b>", cpu_number);
	label = gtk_label_new (text);
	gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
	gtk_label_set_xalign (GTK_LABEL (label), 0);
	gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	g_free (text);

	sg0 = gtk_size_group_new (GTK_SIZE_GROUP_BOTH);
	sg1 = gtk_size_group_new (GTK_SIZE_GROUP_BOTH);

	/* display driver */
	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, BORDER);
	gtk_box_pack_start (GTK_BOX (dialog_vbox), hbox, FALSE, FALSE, 0);

	label = gtk_label_new (_("Scaling driver:"));
	gtk_size_group_add_widget (sg0, label);
	gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
	gtk_label_set_xalign (GTK_LABEL (label), 0);
	gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);

	if (cpu->scaling_driver != NULL)
		text = g_strdup_printf ("<b>%s</b>", cpu->scaling_driver);
	else
		text = g_strdup_printf (_("No scaling driver available"));

	label = gtk_label_new (text);
	gtk_size_group_add_widget (sg1, label);
	gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
	gtk_label_set_xalign (GTK_LABEL (label), 0);
	gtk_box_pack_end (GTK_BOX (hbox), label, TRUE, TRUE, 0);
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	g_free (text);

	/* display list of available freqs */
	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, BORDER);
	gtk_box_pack_start (GTK_BOX (dialog_vbox), hbox, FALSE, FALSE, 0);

	label = gtk_label_new (_("Available frequencies:"));
	gtk_size_group_add_widget (sg0, label);
	gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
	gtk_label_set_xalign (GTK_LABEL (label), 0);
	gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);

	if (cpu->available_freqs != NULL) /* Linux 2.6 with scaling support */
	{
		combo = gtk_combo_box_new_text ();
		gtk_size_group_add_widget (sg1, combo);
		gtk_box_pack_end (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
		list = g_list_first (cpu->available_freqs);
		j = 0;
		while (list)
		{
			text = cpufreq_get_human_readable_freq (GPOINTER_TO_INT (list->data));
			if (GPOINTER_TO_INT (list->data) == cpu->cur_freq)
				i = j;
			gtk_combo_box_append_text (GTK_COMBO_BOX (combo), text);
			g_free (text);
			list = g_list_next (list);
			j++;
		}
		gtk_combo_box_set_active (GTK_COMBO_BOX (combo), i);
	}
	else if (cpu->cur_freq && cpu->min_freq && cpu->max_freq) /* Linux 2.4 with scaling support */
	{
		combo = gtk_combo_box_new_text ();
		gtk_size_group_add_widget (sg1, combo);
		gtk_box_pack_end (GTK_BOX (hbox), combo, TRUE, TRUE, 0);

                text = cpufreq_get_human_readable_freq (cpu->cur_freq);
                gtk_combo_box_append_text (GTK_COMBO_BOX (combo), text);
                g_free (text);
		text = cpufreq_get_human_readable_freq (cpu->max_freq);
                gtk_combo_box_append_text (GTK_COMBO_BOX (combo), text);
                g_free (text);
		text = cpufreq_get_human_readable_freq (cpu->min_freq);
                gtk_combo_box_append_text (GTK_COMBO_BOX (combo), text);
                g_free (text);

                gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);
	}
	else /* If there is no scaling support only show the cpu freq */
	{
		text = cpufreq_get_human_readable_freq (cpu->cur_freq);
		text = g_strdup_printf ("<b>%s</b> (current frequency)", text);
		label = gtk_label_new (text);
		gtk_size_group_add_widget (sg1, label);
		gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
		gtk_label_set_xalign (GTK_LABEL (label), 0);
		gtk_box_pack_end (GTK_BOX (hbox), label, TRUE, TRUE, 0);
		gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
		g_free (text);
	}

#ifdef __linux__
	/* display list of available governors */
	if (cpu->available_governors != NULL) /* Linux 2.6 and cpu scaling support */
	{
		hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, BORDER);
		gtk_box_pack_start (GTK_BOX (dialog_vbox), hbox, FALSE, FALSE, 0);

		label = gtk_label_new (_("Available governors:"));
		gtk_size_group_add_widget (sg0, label);
		gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
		gtk_label_set_xalign (GTK_LABEL (label), 0);
		gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);

		combo = gtk_combo_box_new_text ();
		gtk_size_group_add_widget (sg1, combo);
		gtk_box_pack_end (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
		list = g_list_first (cpu->available_governors);
		j = 0;
		while (list)
		{
			gtk_combo_box_append_text (GTK_COMBO_BOX (combo), list->data);
			if (g_ascii_strcasecmp (list->data, cpu->cur_governor) == 0)
				i = j;
			list = g_list_next (list);
			j++;
		}

		gtk_combo_box_set_active (GTK_COMBO_BOX (combo), i);
	}
	else if (cpu->cur_governor != NULL) /* Linux 2.4 and cpu scaling support */
	{
		hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, BORDER);
		gtk_box_pack_start (GTK_BOX (dialog_vbox), hbox, FALSE, FALSE, 0);

		label = gtk_label_new (_("Current governor:"));
		gtk_size_group_add_widget (sg0, label);
		gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
		gtk_label_set_xalign (GTK_LABEL (label), 0);
		gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);

		text = g_strdup_printf ("<b>%s</b>", cpu->cur_governor);
		label = gtk_label_new (text);
		gtk_size_group_add_widget (sg1, label);
		gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
		gtk_label_set_xalign (GTK_LABEL (label), 0);
		gtk_box_pack_end (GTK_BOX (hbox), label, TRUE, TRUE, 0);
		gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
		g_free (text);
	}
	/* If there is no scaling support, do not display governor combo */
#endif /* __linux__ */

	g_object_unref (sg0);
	g_object_unref (sg1);
}

static void
cpufreq_overview_response (GtkWidget *dialog, gint response, gpointer data)
{
	g_object_set_data (G_OBJECT (cpuFreq->plugin), "overview", NULL);
	gtk_widget_destroy (dialog);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cpuFreq->button), FALSE);
}

gboolean
cpufreq_overview (GtkWidget *widget, GdkEventButton *ev, CpuFreqPlugin *cpuFreq)
{
	gint 	  i, j, step;
	GtkWidget *dialog, *dialog_vbox, *window;
	GtkWidget *dialog_hbox, *separator;

	if (ev->button != 1)
		return FALSE;

	window = g_object_get_data (G_OBJECT (cpuFreq->plugin), "overview");

	if (window) {
		g_object_set_data (G_OBJECT (cpuFreq->plugin), "overview", NULL);
		gtk_widget_destroy (window);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cpuFreq->button),
									  FALSE);
		return TRUE;
	}

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cpuFreq->button),
								  TRUE);

	dialog = xfce_titled_dialog_new_with_buttons (_("CPU Information"),
					NULL,
					GTK_DIALOG_NO_SEPARATOR,
					GTK_STOCK_CLOSE,
					GTK_RESPONSE_OK,
					NULL);
	xfce_titled_dialog_set_subtitle (XFCE_TITLED_DIALOG (dialog),
			_("An overview of all the CPUs in the system"));

	gtk_window_set_position   (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
	gtk_window_set_icon_name  (GTK_WINDOW (dialog), "xfce4-cpufreq-plugin");

	g_object_set_data (G_OBJECT (cpuFreq->plugin), "overview", dialog);

	dialog_vbox = GTK_DIALOG (dialog)->vbox;

	/* choose how many columns and rows depending on cpu count */
	if (cpuFreq->cpus->len < 4)
		step = 1;
	else if (cpuFreq->cpus->len < 9)
		step = 2;
	else if (cpuFreq->cpus->len % 3)
		step = 4;
	else
		step = 3;

	for (i = 0; i < cpuFreq->cpus->len; i += step) {
		dialog_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, BORDER * 2);
		gtk_box_pack_start (GTK_BOX (dialog_vbox), dialog_hbox,
							FALSE, FALSE, BORDER * 2);
		gtk_container_set_border_width (GTK_CONTAINER (dialog_hbox),
										BORDER * 2);

		for (j = i; j < cpuFreq->cpus->len && j < i + step; j++) {
			CpuInfo *cpu = g_ptr_array_index (cpuFreq->cpus, j);
			cpufreq_overview_add (cpu, j, dialog_hbox);

			if (j + 1 < cpuFreq->cpus->len && j + 1 == i + step) {
				separator = gtk_hseparator_new ();
				gtk_box_pack_start (GTK_BOX (dialog_vbox), separator,
									FALSE, FALSE, 0);
			}

			if (j + 1 < cpuFreq->cpus->len && j + 1 < i + step) {
				separator = gtk_vseparator_new ();
				gtk_box_pack_start (GTK_BOX (dialog_hbox), separator,
									FALSE, FALSE, 0);
			}
		}
	}
	g_signal_connect (G_OBJECT (dialog), "response",
					  G_CALLBACK (cpufreq_overview_response), NULL);

	gtk_widget_show_all (dialog);

	return TRUE;
}
