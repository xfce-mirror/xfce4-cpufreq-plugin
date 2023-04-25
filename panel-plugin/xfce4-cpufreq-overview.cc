/*  xfce4-cpu-freq-plugin - panel plugin for cpu informations
 *
 *  Copyright (c) 2006 Thomas Schreck <shrek@xfce.org>
 *  Copyright (c) 2010,2011 Florian Rivoal <frivoal@xfce.org>
 *  Copyright (c) 2013 Harald Judt <h.judt@gmx.at>
 *  Copyright (c) 2022 Jan Ziak <0xe2.0x9a.0x9b@xfce.org>
 *  Copyright (c) 2023 Elia Yehuda <z4ziggy@gmail.com>
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
#include "xfce4-cpufreq-linux-dbus.h"
#endif /* __linux__ */

#ifdef __linux__
static void
combo_frequency_changed (GtkWidget *combo, gpointer p)
{
  int cpu = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (combo), "cpu"));
  bool all = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (combo), "all"));
  gchar *frequency = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (combo));
  if (frequency != NULL)
  {
    int mhz = std::stoi (frequency);
    if (g_strrstr (frequency, "GHz") != NULL)
      mhz = std::stod (frequency) * 1000 * 1000;
    cpufreq_dbus_set_frequency (std::to_string (mhz).c_str(), cpu, all);
    g_free (frequency);
  }
}

static void
combo_governor_changed (GtkWidget *combo, gpointer p)
{
  int cpu = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (combo), "cpu"));
  bool all = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (combo), "all"));
  gchar *governor = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (combo));
  if (governor != NULL)
  {
    cpufreq_dbus_set_governor (governor, cpu, all);
    g_free (governor);
  }
}
#endif

static void
cpufreq_overview_add (const Ptr<const CpuInfo> &cpu, guint cpu_number, GtkWidget *dialog_hbox, bool all = false)
{
  GtkWidget *hbox, *label;
  const CpuFreqUnit unit = cpuFreq->options->unit;

  /* Copy shared fields to a local variable. The local variable can then be accessed without a mutex. */
  CpuInfo::Shared cpu_shared;
  {
      std::lock_guard<std::mutex> guard(cpu->mutex);
      cpu_shared = cpu->shared;
  }

  GtkWidget *dialog_vbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, BORDER);
  gtk_widget_set_sensitive (dialog_vbox, cpu_shared.online);
  gtk_box_pack_start (GTK_BOX (dialog_hbox), dialog_vbox, true, true, 0);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, BORDER);
  gtk_box_pack_start (GTK_BOX (dialog_vbox), hbox, true, true, 0);

  if (all)
  {
    label = gtk_label_new (_("<b>All CPUs</b>"));
    gtk_widget_set_margin_start (label, 20 + BORDER*2);
  } 
  else 
  {
    GtkWidget *icon = gtk_image_new_from_icon_name ("xfce4-cpufreq-plugin", GTK_ICON_SIZE_BUTTON);
    gtk_widget_set_halign (icon, GTK_ALIGN_START);
    gtk_widget_set_valign (icon, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_start (icon, 5);
    gtk_widget_set_margin_end (icon, 5);
    gtk_box_pack_start (GTK_BOX (hbox), icon, false, false, 0);
    label = gtk_label_new (xfce4::sprintf ("<b>CPU %u</b>", cpu_number).c_str());
  }
  gtk_widget_set_margin_top (label, 10);
  gtk_widget_set_margin_bottom (label, 10);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_label_set_xalign (GTK_LABEL (label), 0);
  gtk_widget_set_margin_end (label, 20);
  gtk_box_pack_start (GTK_BOX (hbox), label, true, true, 0);
  gtk_label_set_use_markup (GTK_LABEL (label), true);

  GtkSizeGroup *sg0 = gtk_size_group_new (GTK_SIZE_GROUP_BOTH);
  GtkSizeGroup *sg1 = gtk_size_group_new (GTK_SIZE_GROUP_BOTH);

  /* display list of available freqs */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, BORDER);
  gtk_box_pack_start (GTK_BOX (dialog_vbox), hbox, false, false, 0);

  if (!cpu->available_freqs.empty()) /* Linux 2.6 with scaling support */
  {
    GtkWidget *combo = gtk_combo_box_text_new ();
    gtk_size_group_add_widget (sg1, combo);
    gtk_box_pack_end (GTK_BOX (hbox), combo, true, true, 0);
    gint i = 0;
    for(size_t j = 0; j < cpu->available_freqs.size(); j++)
    {
      std::string available_freq = cpufreq_get_human_readable_freq (cpu->available_freqs[j], unit);
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), available_freq.c_str());

      if (cpu->available_freqs[j] == cpu_shared.cur_freq)
        i = j;
    }
    gtk_combo_box_set_active (GTK_COMBO_BOX (combo), i);
#ifdef __linux__
    g_object_set_data (G_OBJECT (combo), "all", GINT_TO_POINTER (all));
    g_object_set_data (G_OBJECT (combo), "cpu", GINT_TO_POINTER (cpu_number));
    g_signal_connect (combo, "changed", G_CALLBACK (combo_frequency_changed), NULL);
#endif
  }
  else if (cpu_shared.cur_freq && cpu->min_freq && cpu->max_freq_nominal) /* Linux 2.4 with scaling support */
  {
    GtkWidget *combo = gtk_combo_box_text_new ();
    gtk_size_group_add_widget (sg1, combo);
    gtk_box_pack_end (GTK_BOX (hbox), combo, true, true, 0);

    std::string cur_freq = cpufreq_get_human_readable_freq (cpu_shared.cur_freq, unit);
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), cur_freq.c_str());

    std::string max_freq_nominal = cpufreq_get_human_readable_freq (cpu->max_freq_nominal, unit);
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), max_freq_nominal.c_str());

    std::string min_freq = cpufreq_get_human_readable_freq (cpu->min_freq, unit);
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), min_freq.c_str());

    gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);
#ifdef __linux__
    g_object_set_data (G_OBJECT (combo), "all", GINT_TO_POINTER (all));
    g_object_set_data (G_OBJECT (combo), "cpu", GINT_TO_POINTER (cpu_number));
    g_signal_connect (combo, "changed", G_CALLBACK (combo_frequency_changed), NULL);
#endif
  }
  else /* If there is no scaling support only show the cpu freq */
  {
    std::string cur_freq = cpufreq_get_human_readable_freq (cpu_shared.cur_freq, unit);
    cur_freq = "<b>" + cur_freq + "</b> (current frequency)";
    label = gtk_label_new (cur_freq.c_str());
    gtk_size_group_add_widget (sg1, label);
    gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
    gtk_label_set_xalign (GTK_LABEL (label), 0);
    gtk_box_pack_end (GTK_BOX (hbox), label, true, true, 0);
    gtk_label_set_use_markup (GTK_LABEL (label), true);
  }

#ifdef __linux__
  /* display list of available governors */
  if (!cpu->available_governors.empty()) /* Linux 2.6 and cpu scaling support */
  {
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, BORDER);
    gtk_box_pack_start (GTK_BOX (dialog_vbox), hbox, false, false, 10);

    GtkWidget *combo = gtk_combo_box_text_new ();
    gtk_size_group_add_widget (sg1, combo);
    gtk_box_pack_end (GTK_BOX (hbox), combo, true, true, 0);

    gint i = 0;
    for(size_t j = 0; j < cpu->available_governors.size(); j++)
    {
      const std::string &available_governor = cpu->available_governors[j];
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), available_governor.c_str());

      if (g_ascii_strcasecmp (available_governor.c_str(), cpu_shared.cur_governor.c_str()) == 0)
        i = j;
    }

    gtk_combo_box_set_active (GTK_COMBO_BOX (combo), i);
    g_object_set_data (G_OBJECT (combo), "all", GINT_TO_POINTER (all));
    g_object_set_data (G_OBJECT (combo), "cpu", GINT_TO_POINTER (cpu_number));
    g_signal_connect (combo, "changed", G_CALLBACK (combo_governor_changed), NULL);
  }
  else if (!cpu_shared.cur_governor.empty()) /* Linux 2.4 and cpu scaling support */
  {
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, BORDER);
    gtk_box_pack_start (GTK_BOX (dialog_vbox), hbox, false, false, 0);

    std::string cur_governor = "<b>" + cpu_shared.cur_governor + "</b>";
    label = gtk_label_new (cur_governor.c_str());
    gtk_size_group_add_widget (sg1, label);
    gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
    gtk_label_set_xalign (GTK_LABEL (label), 0);
    gtk_box_pack_end (GTK_BOX (hbox), label, true, true, 0);
    gtk_label_set_use_markup (GTK_LABEL (label), true);
  }
  /* If there is no scaling support, do not display governor combo */
#endif /* __linux__ */

  g_object_unref (sg0);
  g_object_unref (sg1);
}



static void
cpufreq_overview_response (GtkDialog *dialog, gint response)
{
  g_object_set_data (G_OBJECT (cpuFreq->plugin), "overview", NULL);
  gtk_widget_destroy (GTK_WIDGET (dialog));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cpuFreq->button), false);
}



bool
cpufreq_overview (GdkEventButton *ev)
{
  if (ev->button != 1)
    return false;

  auto window = (GtkWidget*) g_object_get_data (G_OBJECT (cpuFreq->plugin), "overview");

  if (window)
  {
    g_object_set_data (G_OBJECT (cpuFreq->plugin), "overview", NULL);
    gtk_widget_destroy (window);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cpuFreq->button), false);
    return true;
  }

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cpuFreq->button), true);

  GtkWidget *dialog = xfce_titled_dialog_new_with_mixed_buttons (_("CPU Information"),
    GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (cpuFreq->plugin))),
    GTK_DIALOG_DESTROY_WITH_PARENT,
    "window-close-symbolic", _("_Close"), GTK_RESPONSE_OK,
    NULL);

  xfce_titled_dialog_set_subtitle (XFCE_TITLED_DIALOG (dialog),
    _("An overview of all the CPUs in the system"));

  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
  gtk_window_set_icon_name (GTK_WINDOW (dialog), "xfce4-cpufreq-plugin");

  g_object_set_data (G_OBJECT (cpuFreq->plugin), "overview", dialog);

  GtkWidget *dialog_box = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  GtkWidget *dialog_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_pack_start (GTK_BOX (dialog_box), dialog_vbox, true, true, 20);

  /* choose how many columns and rows depending on cpu count */
  size_t step;
  if (cpuFreq->cpus.size() < 4)
    step = 1;
  else if (cpuFreq->cpus.size() < 9)
    step = 2;
  else if (cpuFreq->cpus.size() % 3 != 0)
    step = 4;
  else
    step = 3;

  GtkWidget *dbox, *hbox, *label, *lblbox;

  GtkSizeGroup *sg0 = gtk_size_group_new (GTK_SIZE_GROUP_BOTH);
  GtkSizeGroup *sg1 = gtk_size_group_new (GTK_SIZE_GROUP_BOTH);

  /* display driver */
  dbox = gtk_box_new ((step>1)?GTK_ORIENTATION_HORIZONTAL:GTK_ORIENTATION_VERTICAL, BORDER);
  gtk_box_pack_start (GTK_BOX (dialog_vbox), dbox, false, false, 0);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, BORDER * 2);
  gtk_box_pack_start (GTK_BOX (dbox), hbox, true, true, BORDER * 2);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), BORDER * 2);
  gtk_widget_set_margin_bottom (hbox, 10);

  label = gtk_label_new (_("Scaling driver:"));
  gtk_size_group_add_widget (sg0, label);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
  gtk_label_set_xalign (GTK_LABEL (label), 0);
  gtk_widget_set_margin_start (label, 5);
  gtk_box_pack_start (GTK_BOX (hbox), label, false, false, 0);

  {
    std::string text;
    if (!cpuFreq->cpus[0]->scaling_driver.empty())
      text = "<b>" + cpuFreq->cpus[0]->scaling_driver + "</b>";
    else
      text = xfce4::sprintf (_("No driver available"));

    label = gtk_label_new (text.c_str());
    gtk_size_group_add_widget (sg1, label);
    gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
    gtk_label_set_xalign (GTK_LABEL (label), 0);
    gtk_box_pack_start (GTK_BOX (hbox), label, false, false, 0);
    gtk_label_set_use_markup (GTK_LABEL (label), true);
    gtk_widget_set_margin_end (label, 20);
  }

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, BORDER * 2);
  gtk_box_pack_start (GTK_BOX (dbox), hbox, false, false, BORDER * 2);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), BORDER * 2);

  {
    size_t j = cpuFreq->cpus.size() - 1;
    Ptr<const CpuInfo> cpu = cpuFreq->cpus[j];
    cpufreq_overview_add (cpu, j, hbox, true);
  }

  lblbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, BORDER*2);
  gtk_box_pack_start (GTK_BOX (dialog_vbox), lblbox, false, false, 10);

  for (size_t i = 0; i < step; i++)
  {
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, BORDER*2);
    gtk_box_pack_start (GTK_BOX (lblbox), hbox, true, true, 0);

    label = gtk_label_new (_("Frequency"));
    gtk_size_group_add_widget (sg0, label);
    gtk_widget_set_margin_end (label, 50);
    gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
    gtk_widget_set_halign (label, GTK_ALIGN_END);
    gtk_label_set_xalign (GTK_LABEL (label), 0);
    gtk_box_pack_start (GTK_BOX (hbox), label, true, true, 0);

    label = gtk_label_new (_("Governor"));
    gtk_size_group_add_widget (sg0, label);
    gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
    gtk_label_set_xalign (GTK_LABEL (label), 0);
    gtk_box_pack_end (GTK_BOX (hbox), label, false, false, 0);
  }

  for (size_t i = 0; i < cpuFreq->cpus.size(); i += step)
  {
    GtkWidget *dialog_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, BORDER * 2);
    gtk_box_pack_start (GTK_BOX (dialog_vbox), dialog_hbox, false, false, BORDER * 2);
    gtk_container_set_border_width (GTK_CONTAINER (dialog_hbox), BORDER * 2);

    for (size_t j = i; j < cpuFreq->cpus.size() && j < i + step; j++)
    {
      Ptr<const CpuInfo> cpu = cpuFreq->cpus[j];
      cpufreq_overview_add (cpu, j, dialog_hbox);
      if (j + 1 < cpuFreq->cpus.size() && j + 1 < i + step)
      {
        GtkWidget *separator = gtk_separator_new (GTK_ORIENTATION_VERTICAL);
        gtk_box_pack_start (GTK_BOX (dialog_hbox), separator, false, false, 0);
      }
    }
  }

  xfce4::connect_response (GTK_DIALOG (dialog), cpufreq_overview_response);

  gtk_widget_show_all (dialog);

  return true;
}
