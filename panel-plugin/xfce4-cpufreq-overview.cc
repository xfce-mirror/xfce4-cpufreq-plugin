/*  xfce4-cpu-freq-plugin - panel plugin for cpu informations
 *
 *  Copyright (c) 2006 Thomas Schreck <shrek@xfce.org>
 *  Copyright (c) 2010,2011 Florian Rivoal <frivoal@xfce.org>
 *  Copyright (c) 2013 Harald Judt <h.judt@gmx.at>
 *  Copyright (c) 2022 Jan Ziak <0xe2.0x9a.0x9b@xfce.org>
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

#include "xfce4-cpufreq-plugin.h"
#include "xfce4-cpufreq-overview.h"
#include "xfce4-cpufreq-utils.h"

#ifdef __linux__
#include "xfce4-cpufreq-linux.h"
#endif /* __linux__ */



static void
cpufreq_overview_add (const CpuInfo *cpu, guint cpu_number, GtkWidget *dialog_hbox)
{
  gchar *text;
  GtkWidget *hbox, *label;
  const CpuFreqUnit unit = cpuFreq->options->unit;

  GtkWidget *dialog_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, BORDER);
  gtk_widget_set_sensitive (dialog_vbox, cpu->online);
  gtk_box_pack_start (GTK_BOX (dialog_hbox), dialog_vbox, true, true, 0);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, BORDER);
  gtk_box_pack_start (GTK_BOX (dialog_vbox), hbox, true, true, 0);

  GtkWidget *icon = gtk_image_new_from_icon_name ("xfce4-cpufreq-plugin", GTK_ICON_SIZE_BUTTON);
  gtk_widget_set_halign (icon, GTK_ALIGN_END);
  gtk_widget_set_valign (icon, GTK_ALIGN_CENTER);
  gtk_widget_set_margin_top (icon, 10);
  gtk_widget_set_margin_bottom (icon, 10);
  gtk_widget_set_margin_start (icon, 5);
  gtk_widget_set_margin_end (icon, 5);

  gtk_box_pack_start (GTK_BOX (hbox), icon, true, true, 0);
  text = g_strdup_printf ("<b>CPU %d</b>", cpu_number);
  label = gtk_label_new (text);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
  gtk_label_set_xalign (GTK_LABEL (label), 0);
  gtk_box_pack_start (GTK_BOX (hbox), label, true, true, 0);
  gtk_label_set_use_markup (GTK_LABEL (label), true);
  g_free (text);

  GtkSizeGroup *sg0 = gtk_size_group_new (GTK_SIZE_GROUP_BOTH);
  GtkSizeGroup *sg1 = gtk_size_group_new (GTK_SIZE_GROUP_BOTH);

  /* display driver */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, BORDER);
  gtk_box_pack_start (GTK_BOX (dialog_vbox), hbox, false, false, 0);

  label = gtk_label_new (_("Scaling driver:"));
  gtk_size_group_add_widget (sg0, label);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
  gtk_label_set_xalign (GTK_LABEL (label), 0);
  gtk_box_pack_start (GTK_BOX (hbox), label, true, true, 0);

  if (cpu->scaling_driver != NULL)
    text = g_strdup_printf ("<b>%s</b>", cpu->scaling_driver);
  else
    text = g_strdup_printf (_("No scaling driver available"));

  label = gtk_label_new (text);
  gtk_size_group_add_widget (sg1, label);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
  gtk_label_set_xalign (GTK_LABEL (label), 0);
  gtk_box_pack_end (GTK_BOX (hbox), label, true, true, 0);
  gtk_label_set_use_markup (GTK_LABEL (label), true);
  g_free (text);

  /* display list of available freqs */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, BORDER);
  gtk_box_pack_start (GTK_BOX (dialog_vbox), hbox, false, false, 0);

  label = gtk_label_new (_("Available frequencies:"));
  gtk_size_group_add_widget (sg0, label);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
  gtk_label_set_xalign (GTK_LABEL (label), 0);
  gtk_box_pack_start (GTK_BOX (hbox), label, true, true, 0);

  if (cpu->available_freqs != NULL) /* Linux 2.6 with scaling support */
  {
    GtkWidget *combo = gtk_combo_box_text_new ();
    gtk_size_group_add_widget (sg1, combo);
    gtk_box_pack_end (GTK_BOX (hbox), combo, true, true, 0);
    GList *list = g_list_first (cpu->available_freqs);
    gint i = 0, j = 0;
    while (list)
    {
      text = cpufreq_get_human_readable_freq (GPOINTER_TO_UINT (list->data), unit);

      if (GPOINTER_TO_UINT (list->data) == cpu->cur_freq)
        i = j;

      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), text);
      g_free (text);
      list = g_list_next (list);
      j++;
    }
    gtk_combo_box_set_active (GTK_COMBO_BOX (combo), i);
  }
  else if (cpu->cur_freq && cpu->min_freq && cpu->max_freq_nominal) /* Linux 2.4 with scaling support */
  {
    GtkWidget *combo = gtk_combo_box_text_new ();
    gtk_size_group_add_widget (sg1, combo);
    gtk_box_pack_end (GTK_BOX (hbox), combo, true, true, 0);

    text = cpufreq_get_human_readable_freq (cpu->cur_freq, unit);
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), text);
    g_free (text);

    text = cpufreq_get_human_readable_freq (cpu->max_freq_nominal, unit);
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), text);
    g_free (text);

    text = cpufreq_get_human_readable_freq (cpu->min_freq, unit);
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), text);
    g_free (text);

    gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);
  }
  else /* If there is no scaling support only show the cpu freq */
  {
    text = cpufreq_get_human_readable_freq (cpu->cur_freq, unit);
    text = g_strdup_printf ("<b>%s</b> (current frequency)", text);
    label = gtk_label_new (text);
    gtk_size_group_add_widget (sg1, label);
    gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
    gtk_label_set_xalign (GTK_LABEL (label), 0);
    gtk_box_pack_end (GTK_BOX (hbox), label, true, true, 0);
    gtk_label_set_use_markup (GTK_LABEL (label), true);
    g_free (text);
  }

#ifdef __linux__
  /* display list of available governors */
  if (cpu->available_governors != NULL) /* Linux 2.6 and cpu scaling support */
  {
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, BORDER);
    gtk_box_pack_start (GTK_BOX (dialog_vbox), hbox, false, false, 0);

    label = gtk_label_new (_("Available governors:"));
    gtk_size_group_add_widget (sg0, label);
    gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
    gtk_label_set_xalign (GTK_LABEL (label), 0);
    gtk_box_pack_start (GTK_BOX (hbox), label, true, true, 0);

    GtkWidget *combo = gtk_combo_box_text_new ();
    gtk_size_group_add_widget (sg1, combo);
    gtk_box_pack_end (GTK_BOX (hbox), combo, true, true, 0);

    GList *list = g_list_first (cpu->available_governors);
    gint i = 0, j = 0;
    while (list)
    {
      auto list_data = (const gchar*) list->data;

      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), list_data);

      if (g_ascii_strcasecmp (list_data, cpu->cur_governor) == 0)
        i = j;

      list = g_list_next (list);
      j++;
    }

    gtk_combo_box_set_active (GTK_COMBO_BOX (combo), i);
  }
  else if (cpu->cur_governor != NULL) /* Linux 2.4 and cpu scaling support */
  {
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, BORDER);
    gtk_box_pack_start (GTK_BOX (dialog_vbox), hbox, false, false, 0);

    label = gtk_label_new (_("Current governor:"));
    gtk_size_group_add_widget (sg0, label);
    gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
    gtk_label_set_xalign (GTK_LABEL (label), 0);
    gtk_box_pack_start (GTK_BOX (hbox), label, true, true, 0);

    text = g_strdup_printf ("<b>%s</b>", cpu->cur_governor);
    label = gtk_label_new (text);
    gtk_size_group_add_widget (sg1, label);
    gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
    gtk_label_set_xalign (GTK_LABEL (label), 0);
    gtk_box_pack_end (GTK_BOX (hbox), label, true, true, 0);
    gtk_label_set_use_markup (GTK_LABEL (label), true);
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
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cpuFreq->button), false);
}



gboolean
cpufreq_overview (GtkWidget *widget, GdkEventButton *ev, CpuFreqPlugin *cpufreq)
{
  if (ev->button != 1)
    return false;

  auto window = (GtkWidget*) g_object_get_data (G_OBJECT (cpufreq->plugin), "overview");

  if (window) {
    g_object_set_data (G_OBJECT (cpufreq->plugin), "overview", NULL);
    gtk_widget_destroy (window);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cpufreq->button), false);
    return true;
  }

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cpufreq->button), true);

  GtkWidget *dialog = xfce_titled_dialog_new_with_mixed_buttons (_("CPU Information"),
    GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (cpufreq->plugin))),
    GTK_DIALOG_DESTROY_WITH_PARENT,
    "window-close-symbolic", _("_Close"), GTK_RESPONSE_OK,
    NULL);

  xfce_titled_dialog_set_subtitle (XFCE_TITLED_DIALOG (dialog),
    _("An overview of all the CPUs in the system"));

  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
  gtk_window_set_icon_name (GTK_WINDOW (dialog), "xfce4-cpufreq-plugin");

  g_object_set_data (G_OBJECT (cpufreq->plugin), "overview", dialog);

  GtkWidget *dialog_vbox = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

  /* choose how many columns and rows depending on cpu count */
  gint step;
  if (cpufreq->cpus->len < 4)
    step = 1;
  else if (cpufreq->cpus->len < 9)
    step = 2;
  else if (cpufreq->cpus->len % 3)
    step = 4;
  else
    step = 3;

  for (guint i = 0; i < cpufreq->cpus->len; i += step) {
    GtkWidget *dialog_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, BORDER * 2);
    gtk_box_pack_start (GTK_BOX (dialog_vbox), dialog_hbox, false, false, BORDER * 2);
    gtk_container_set_border_width (GTK_CONTAINER (dialog_hbox), BORDER * 2);

    for (guint j = i; j < cpufreq->cpus->len && j < i + step; j++) {
      auto cpu = (const CpuInfo*) g_ptr_array_index (cpufreq->cpus, j);
      cpufreq_overview_add (cpu, j, dialog_hbox);

      if (j + 1 < cpufreq->cpus->len && j + 1 == i + step) {
        GtkWidget *separator = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
        gtk_box_pack_start (GTK_BOX (dialog_vbox), separator, false, false, 0);
      }

      if (j + 1 < cpufreq->cpus->len && j + 1 < i + step) {
        GtkWidget *separator = gtk_separator_new (GTK_ORIENTATION_VERTICAL);
        gtk_box_pack_start (GTK_BOX (dialog_hbox), separator, false, false, 0);
      }
    }
  }

  g_signal_connect (G_OBJECT (dialog), "response",
                    G_CALLBACK (cpufreq_overview_response), NULL);

  gtk_widget_show_all (dialog);

  return true;
}
