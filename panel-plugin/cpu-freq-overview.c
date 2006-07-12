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

#define BORDER 3 
#define COMBO_FREQ 1
#define COMBO_GOV  2

#include <string.h>
#include <gtk/gtk.h>

#include <libxfcegui4/libxfcegui4.h>
#include <libxfce4panel/xfce-panel-plugin.h>

#include "cpu-freq-plugin.h"
#include "cpu-freq-overview.h"

static void
cpu_freq_overview_add (CpuInfo *cpu,
		       gint cpu_no,
		       GtkWidget *dialog_hbox)
{
	GtkWidget *hbox, *dialog_vbox, *combo, *label, *icon;
	gchar *text, *freq_unit;
	GList *list;
	gint i, j, div;

	dialog_vbox = gtk_vbox_new (FALSE, BORDER);
	gtk_box_pack_start (GTK_BOX (dialog_hbox), dialog_vbox, TRUE, TRUE, 0);

	text = g_strdup_printf ("<b>CPU %d</b>", cpu_no);
	label = gtk_label_new (text);
	gtk_box_pack_start (GTK_BOX (dialog_vbox), label, FALSE, FALSE, 0);
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	g_free (text);

	icon = gtk_image_new_from_icon_name ("cpu", GTK_ICON_SIZE_BUTTON);
	gtk_box_pack_start (GTK_BOX (dialog_vbox), icon, FALSE, FALSE, 0);
	gtk_misc_set_alignment (GTK_MISC (icon), 0.5, 0);
	gtk_misc_set_padding (GTK_MISC (icon), 10, 10);

	hbox = gtk_hbox_new (FALSE, BORDER);
	gtk_box_pack_start (GTK_BOX (dialog_vbox), hbox, FALSE, FALSE, 0);

	label = gtk_label_new (_("Scaling Driver:"));
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	text = g_strdup_printf ("<b>%s</b>", cpu->cur_driver);
	label = gtk_label_new (text);
	gtk_box_pack_end (GTK_BOX (hbox), label, TRUE, TRUE, 0);
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	g_free (text);

	hbox = gtk_hbox_new (FALSE, BORDER);
	gtk_box_pack_start (GTK_BOX (dialog_vbox), hbox, FALSE, FALSE, 0);

	label = gtk_label_new (_("Available Frequencies:"));
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	combo = gtk_combo_box_new_text ();
	gtk_box_pack_end (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
	list = g_list_first (cpu->available_freqs);
	j = 0;
	while (list)
	{
		if (list->data > 999999)
		{
			div = (1000 * 1000);
			freq_unit = g_strdup ("GHz");
		}
		else
		{
			div = 1000;
			freq_unit = g_strdup ("MHz");
		}
		
		if ( (GPOINTER_TO_INT(list->data) % div) == 0 || div == 1000 )
			text = g_strdup_printf ("%d %s", GPOINTER_TO_INT(list->data)/div, freq_unit);
		else
			text = g_strdup_printf ("%3.2f %s", 
					((gfloat)GPOINTER_TO_INT(list->data)/div), freq_unit);
		
		if (GPOINTER_TO_INT(list->data) == cpu->cur_freq)
			i = j;
		
		gtk_combo_box_append_text (GTK_COMBO_BOX (combo),
				text);
		g_free (text);
		g_free (freq_unit);
		list = g_list_next (list);
		j++;
	}
	gtk_combo_box_set_active (GTK_COMBO_BOX (combo), i);
			
	hbox = gtk_hbox_new (FALSE, BORDER);
	gtk_box_pack_start (GTK_BOX (dialog_vbox), hbox, FALSE, FALSE, 0);

	label = gtk_label_new (_("Available Governors:"));\
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	combo = gtk_combo_box_new_text ();
	gtk_box_pack_end (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
	list = g_list_first (cpu->available_governors);
	j = 0;
	while (list)
	{
		gtk_combo_box_append_text (GTK_COMBO_BOX (combo),
			list->data);
		if (g_ascii_strcasecmp (list->data, cpu->cur_governor) == 0)
			i = j;
		list = g_list_next (list);
		j++;
	}
	gtk_combo_box_set_active (GTK_COMBO_BOX (combo), i);
}

static void
cpu_freq_overview_response (GtkWidget *dialog,
			    gint response,
			    CpuFreqPlugin *cpuFreq)
{
	g_object_set_data (G_OBJECT (cpuFreq->plugin), "overview", NULL);

	gtk_widget_destroy (dialog);
}

gboolean
cpu_freq_overview (GtkWidget *widget,
		   GdkEventButton *ev,
		   CpuFreqPlugin *cpuFreq)
{
	GtkWidget *dialog, *dialog_vbox, *window;
	GtkWidget *header, *dialog_hbox, *separator;
	gint i;

	if (ev->button != 1)
		return FALSE;

	window = g_object_get_data (G_OBJECT (cpuFreq->plugin), "overview");

	if (window)
		gtk_widget_destroy (window);

	dialog = xfce_titled_dialog_new_with_buttons (_("CPU Information"),
					NULL,
					GTK_DIALOG_NO_SEPARATOR,
					GTK_STOCK_CLOSE,
					GTK_RESPONSE_OK,
					NULL);
	xfce_titled_dialog_set_subtitle (XFCE_TITLED_DIALOG (dialog),
			_("An overview of all the CPUs in the system"));

	gtk_window_set_position   (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
	gtk_window_set_icon_name  (GTK_WINDOW (dialog), "cpu");

	g_object_set_data (G_OBJECT (cpuFreq->plugin), "overview", dialog);

	dialog_vbox = GTK_DIALOG (dialog)->vbox;

	dialog_hbox = gtk_hbox_new (FALSE, BORDER);
	gtk_box_pack_start (GTK_BOX (dialog_vbox), dialog_hbox, FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (dialog_hbox), BORDER);

	for (i = 0; i < cpuFreq->cpus->len;)
	{
		CpuInfo *cpu = g_ptr_array_index (cpuFreq->cpus, i);
		cpu_freq_overview_add (cpu, i, dialog_hbox);

		if (++i != cpuFreq->cpus->len)
		{
			separator = gtk_vseparator_new ();
			gtk_box_pack_start (GTK_BOX (dialog_hbox),
					separator, FALSE, FALSE, 0);
		}
	}

	g_signal_connect(dialog, "response", 
			G_CALLBACK(cpu_freq_overview_response), cpuFreq);

	gtk_widget_show_all (dialog);

	return TRUE;
}
