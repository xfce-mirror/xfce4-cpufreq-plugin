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

#define BORDER 2

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


static void
combo_governor_changed (GtkWidget *combo, gpointer p)
{
  int cpu = GPOINTER_TO_INT (p);
  auto switcher = (GtkWidget*) g_object_get_data (G_OBJECT (cpuFreq->plugin), "all-cpu-switcher");
  bool all = gtk_switch_get_active (GTK_SWITCH (switcher));
  if (all)
    cpu = cpuFreq->cpus.size();
  gchar *governor = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (combo));
  if (governor != NULL)
  {
    GError *err = NULL;
    cpufreq_dbus_set_governor (governor, cpu, all, &err);
    if (err != NULL)
    {
      xfce_dialog_show_error (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (cpuFreq->plugin))), 
        err, "Error while setting governor");
      g_clear_error (&err);
    }
    g_free (governor);
  }
}



static void
combo_preference_changed (GtkWidget *combo, gpointer p)
{
  int cpu = GPOINTER_TO_INT (p);
  auto switcher = (GtkWidget*) g_object_get_data (G_OBJECT (cpuFreq->plugin), "all-cpu-switcher");
  bool all = gtk_switch_get_active (GTK_SWITCH (switcher));
  if (all)
    cpu = cpuFreq->cpus.size();
  gchar *preference = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (combo));
  if (preference != NULL)
  {
    GError *err = NULL;
    cpufreq_dbus_set_preference (preference, cpu, all, &err);
    if (err != NULL)
    {
      xfce_dialog_show_error (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (cpuFreq->plugin))), 
        err, "Error while setting preference");
      g_clear_error (&err);
    }
    g_free (preference);
  }
}



static void
switch_no_turbo_click (GtkSwitch *swtch, gboolean state, gpointer p)
{
  GError *err = NULL;
  cpufreq_dbus_set_no_turbo (state ? "1" : "0", &err);
  if (err != NULL)
  {
    xfce_dialog_show_error (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (cpuFreq->plugin))), 
      err, "Error while setting no turbo");
    g_clear_error (&err);
  }
}


static void
switch_hwp_dynamic_boost_click (GtkSwitch *swtch, gboolean state, gpointer p)
{
  GError *err = NULL;
  cpufreq_dbus_set_hwp_dynamic_boost (state ? "1" : "0", &err);
  if (err != NULL)
  {
    xfce_dialog_show_error (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (cpuFreq->plugin))), 
      err, "Error while setting hwp dynamic boost");
    g_clear_error (&err);
  }
}


static void
cpufreq_overview_add (const Ptr<const CpuInfo> &cpu, guint cpu_number, GtkWidget *dialog_hbox)
{
  GtkWidget *hbox, *label;
  const CpuFreqUnit unit = cpuFreq->options->unit;

  /* Copy shared fields to a local variable. The local variable can then be accessed without a mutex. */
  CpuInfo::Shared cpu_shared;
  {
      std::lock_guard<std::mutex> guard(cpu->mutex);
      cpu_shared = cpu->shared;
  }

  GtkSizeGroup *sg0 = gtk_size_group_new (GTK_SIZE_GROUP_BOTH);

  GtkWidget *dialog_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_halign (dialog_vbox, GTK_ALIGN_CENTER);
  gtk_widget_set_sensitive (dialog_vbox, cpu_shared.online);
  gtk_box_pack_start (GTK_BOX (dialog_hbox), dialog_vbox, true, true, 0);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (dialog_vbox), hbox, true, true, BORDER*2);

  GtkWidget *icon = gtk_image_new_from_icon_name ("xfce4-cpufreq-plugin", GTK_ICON_SIZE_BUTTON);
  gtk_widget_set_halign (icon, GTK_ALIGN_END);
  gtk_widget_set_valign (icon, GTK_ALIGN_START);
  gtk_widget_set_margin_end (icon, 5);
  gtk_box_pack_start (GTK_BOX (hbox), icon, false, false, 0);

  GtkWidget *expander = gtk_expander_new (NULL);
  gtk_box_pack_start (GTK_BOX (hbox), expander, false, false, 0);
  gtk_widget_set_valign (expander, GTK_ALIGN_CENTER);

  GtkWidget *titlebox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_expander_set_label_widget (GTK_EXPANDER (expander), titlebox);

  label = gtk_label_new (xfce4::sprintf ("<b>CPU %u</b>", cpu_number).c_str());
  gtk_size_group_add_widget (sg0, label);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
  gtk_label_set_xalign (GTK_LABEL (label), 0);
  gtk_label_set_use_markup (GTK_LABEL (label), true);
  gtk_box_pack_start (GTK_BOX (titlebox), label, true, true, 0);

  std::string current_freq = cpufreq_get_human_readable_freq (cpu_shared.cur_freq, unit);
  label = gtk_label_new (xfce4::sprintf ("<b>%s</b>", current_freq.c_str()).c_str());
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
  gtk_label_set_xalign (GTK_LABEL (label), 0);
  gtk_widget_set_margin_end (label, 10);
  gtk_box_pack_start (GTK_BOX (titlebox), label, true, true, 0);
  gtk_label_set_use_markup (GTK_LABEL (label), true);

#ifdef __linux__
  /* display list of available governors */
  if (!cpu->available_governors.empty()) /* Linux 2.6 and cpu scaling support */
  {
    GtkWidget *combo = gtk_combo_box_text_new ();
    gtk_box_pack_start (GTK_BOX (hbox), combo, true, true, 10);
    gtk_size_group_add_widget (sg0, combo);

    gint i = 0;
    for(size_t j = 0; j < cpu->available_governors.size(); j++)
    {
      const std::string &available_governor = cpu->available_governors[j];
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), available_governor.c_str());

      if (g_ascii_strcasecmp (available_governor.c_str(), cpu_shared.cur_governor.c_str()) == 0)
        i = j;
    }

    gtk_combo_box_set_active (GTK_COMBO_BOX (combo), i);
    g_signal_connect (combo, "changed", G_CALLBACK (combo_governor_changed), GINT_TO_POINTER (cpu_number));
  }
  else if (!cpu_shared.cur_governor.empty()) /* Linux 2.4 and cpu scaling support */
  {
    std::string cur_governor = "<b>" + cpu_shared.cur_governor + "</b>";
    label = gtk_label_new (cur_governor.c_str());
    gtk_size_group_add_widget (sg0, label);
    gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
    gtk_label_set_xalign (GTK_LABEL (label), 0);
    gtk_box_pack_start (GTK_BOX (hbox), label, true, true, 0);
    gtk_label_set_use_markup (GTK_LABEL (label), true);
  }
  /* If there is no scaling support, do not display governor combo */

  /* display list of available preferences */
  if (!cpu->available_preferences.empty())
  {
    GtkWidget *combo = gtk_combo_box_text_new ();
    gtk_box_pack_start (GTK_BOX (hbox), combo, true, true, 0);

    gint i = 0;
    for(size_t j = 0; j < cpu->available_preferences.size(); j++)
    {
      const std::string &available_prefernce = cpu->available_preferences[j];
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), available_prefernce.c_str());

      if (g_ascii_strcasecmp (available_prefernce.c_str(), cpu_shared.cur_preference.c_str()) == 0)
        i = j;
    }

    gtk_combo_box_set_active (GTK_COMBO_BOX (combo), i);
    g_signal_connect (combo, "changed", G_CALLBACK (combo_preference_changed), GINT_TO_POINTER (cpu_number));
  }
  else if (!cpu_shared.cur_preference.empty())
  {
    std::string cur_preference = "<b>" + cpu_shared.cur_preference + "</b>";
    label = gtk_label_new (cur_preference.c_str());
    gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
    gtk_label_set_xalign (GTK_LABEL (label), 0);
    gtk_box_pack_start (GTK_BOX (hbox), label, true, true, 0);
    gtk_label_set_use_markup (GTK_LABEL (label), true);
  }
#endif /* __linux__ */

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

  if (window) {
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
  gtk_box_pack_start (GTK_BOX (dialog_box), dialog_vbox, false, false, 0);
  gtk_widget_set_margin_top (dialog_vbox, 20);
  gtk_widget_set_margin_start (dialog_vbox, 10);
  gtk_widget_set_margin_end (dialog_vbox, 10);

  /* choose how many columns and rows depending on cpu count */
  size_t step;
  if (cpuFreq->cpus.size() < 9)
    step = 1;
  else if (cpuFreq->cpus.size() < 17)
    step = 2;
  else if (cpuFreq->cpus.size() % 3 != 0)
    step = 4;
  else
    step = 3;

  GtkWidget *hbox, *vbox, *label, *switcher, *icon, *expander;
  GtkSizeGroup *sgv = gtk_size_group_new (GTK_SIZE_GROUP_VERTICAL);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (dialog_vbox), hbox, true, true, 0);

  icon = gtk_image_new_from_icon_name ("xfce4-cpufreq-plugin", GTK_ICON_SIZE_BUTTON);
  gtk_widget_set_halign (icon, GTK_ALIGN_END);
  gtk_widget_set_valign (icon, GTK_ALIGN_START);
  gtk_widget_set_margin_end (icon, 5);
  gtk_box_pack_start (GTK_BOX (hbox), icon, false, false, 0);
  gtk_size_group_add_widget (sgv, icon);

  expander = gtk_expander_new (NULL);
  gtk_expander_set_expanded (GTK_EXPANDER (expander), true);
  // gtk_widget_set_vexpand (expander, TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), expander, true, true, 0);
  gtk_widget_set_valign (expander, GTK_ALIGN_CENTER);

  label = gtk_label_new (_("<b>CPU Settings:</b>"));
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
  gtk_label_set_xalign (GTK_LABEL (label), 0);
  gtk_widget_set_margin_start (label, 5);
  gtk_widget_set_margin_end (label, 20);
  gtk_label_set_use_markup (GTK_LABEL (label), true);
  gtk_expander_set_label_widget ( GTK_EXPANDER (expander), label);
  gtk_size_group_add_widget (sgv, label);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (expander), vbox);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, true, true, 0);

  for (size_t i = 0; i < step; i++)
  {
    GtkWidget *box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start (GTK_BOX (hbox), box, true, true, 0);

      if (i + 1 < step) {
        GtkWidget *separator = gtk_separator_new (GTK_ORIENTATION_VERTICAL);
        gtk_box_pack_end (GTK_BOX (box), separator, false, false, 0);
      }
    label = gtk_label_new (_("Preference"));
    gtk_widget_set_margin_end (label, 90);
    gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
    gtk_label_set_xalign (GTK_LABEL (label), 1);
    gtk_box_pack_end (GTK_BOX (box), label, false, false, 0);

    label = gtk_label_new (_("Governor"));
    gtk_widget_set_margin_end (label, 60);
    gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
    gtk_label_set_xalign (GTK_LABEL (label), 1);
    gtk_box_pack_end (GTK_BOX (box), label, true, true, 0);
  }

  for (size_t i = 0; i < cpuFreq->cpus.size(); i += step)
  {
    GtkWidget *dialog_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start (GTK_BOX (vbox), dialog_hbox, true, true, 0);
    gtk_container_set_border_width (GTK_CONTAINER (dialog_hbox), 0);

    for (size_t j = i; j < cpuFreq->cpus.size() && j < i + step; j++) {
      Ptr<const CpuInfo> cpu = cpuFreq->cpus[j];
      cpufreq_overview_add (cpu, j, dialog_hbox);

      if (j + 1 < cpuFreq->cpus.size() && j + 1 < i + step) {
        GtkWidget *separator = gtk_separator_new (GTK_ORIENTATION_VERTICAL);
        gtk_box_pack_start (GTK_BOX (dialog_hbox), separator, false, false, 0);
      }
    }
  }

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, true, true, BORDER*2);

  label = gtk_label_new (_("Changes applies to ALL CPUs"));
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
  gtk_label_set_xalign (GTK_LABEL (label), 0);
  gtk_box_pack_start (GTK_BOX (hbox), label, true, true, 0);

  switcher = gtk_switch_new ();
  gtk_switch_set_active (GTK_SWITCH (switcher), true);
  gtk_box_pack_end (GTK_BOX (hbox), switcher, false, false, 0);
  g_object_set_data (G_OBJECT (cpuFreq->plugin), "all-cpu-switcher", switcher);
  g_object_bind_property (switcher, "active", G_OBJECT (cpuFreq->plugin), "all-cpu", G_BINDING_DEFAULT);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (dialog_vbox), hbox, true, true, 20);

  icon = gtk_image_new_from_icon_name ("xfce4-cpufreq-plugin", GTK_ICON_SIZE_BUTTON);
  gtk_widget_set_halign (icon, GTK_ALIGN_END);
  gtk_widget_set_valign (icon, GTK_ALIGN_START);
  gtk_widget_set_margin_end (icon, 5);
  gtk_box_pack_start (GTK_BOX (hbox), icon, false, false, 0);
  gtk_size_group_add_widget (sgv, icon);

  expander = gtk_expander_new (NULL);
  gtk_expander_set_expanded (GTK_EXPANDER (expander), true);
  // gtk_widget_set_vexpand (expander, TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), expander, true, true, 0);
  gtk_widget_set_valign (expander, GTK_ALIGN_CENTER);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_expander_set_label_widget ( GTK_EXPANDER (expander), hbox);
  gtk_size_group_add_widget (sgv, hbox);

  /* display driver */
  label = gtk_label_new (_("<b>Scaling driver:</b>"));
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
  gtk_label_set_xalign (GTK_LABEL (label), 0);
  gtk_widget_set_margin_start (label, 5);
  gtk_label_set_use_markup (GTK_LABEL (label), true);
  gtk_widget_set_margin_end (label, 20);
  gtk_box_pack_start (GTK_BOX (hbox), label, true, true, 0);

  {
    std::string text;
    if (!cpuFreq->cpus[0]->scaling_driver.empty())
      text = "<b>" + cpuFreq->cpus[0]->scaling_driver + "</b>";
    else
      text = xfce4::sprintf (_("No scaling driver available"));

    label = gtk_label_new (text.c_str());
    gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
    gtk_label_set_xalign (GTK_LABEL (label), 0);
    gtk_box_pack_start (GTK_BOX (hbox), label, true, true, 0);
    gtk_label_set_use_markup (GTK_LABEL (label), true);
  }

  if (cpuFreq->intel_pstate != nullptr)
  {
    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, BORDER*2);
    gtk_container_add (GTK_CONTAINER (expander), vbox);
    gtk_widget_set_margin_top (vbox, 10);

    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, false, false, BORDER*2);

    label = gtk_label_new (_("HWP Dynamic Boost"));
    gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
    gtk_label_set_xalign (GTK_LABEL (label), 0);
    gtk_box_pack_start (GTK_BOX (hbox), label, false, false, 0);

    switcher = gtk_switch_new ();
    gtk_switch_set_active (GTK_SWITCH (switcher), cpuFreq->intel_pstate->hwp_dynamic_boost);
    gtk_box_pack_end (GTK_BOX (hbox), switcher, false, false, 0);
    g_signal_connect (switcher, "state-set", G_CALLBACK (switch_hwp_dynamic_boost_click), NULL);

    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, false, false, 0);

    label = gtk_label_new (_("No Turbo"));
    gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
    gtk_label_set_xalign (GTK_LABEL (label), 0);
    gtk_box_pack_start (GTK_BOX (hbox), label, false, false, 0);

    switcher = gtk_switch_new ();
    gtk_switch_set_active (GTK_SWITCH (switcher), cpuFreq->intel_pstate->no_turbo);
    gtk_box_pack_end (GTK_BOX (hbox), switcher, false, false, 0);
    g_signal_connect (switcher, "state-set", G_CALLBACK (switch_no_turbo_click), NULL);
  }

  xfce4::connect_response (GTK_DIALOG (dialog), cpufreq_overview_response);

  gtk_widget_show_all (dialog);

  return true;
}
