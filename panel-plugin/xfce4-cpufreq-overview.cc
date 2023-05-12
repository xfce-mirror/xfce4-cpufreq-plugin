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



/* format string "Min/Max nn %" */
static gchar*
scale_format_perf_pct (GtkScale *scale, gdouble value, gpointer data)
{
  return g_strdup_printf ("%s %.0f %%", (gchar*)data, value);
}



/* format string "Min/Max nn MHz" or "Min/Max 0.nn GHz" */
static gchar*
scale_format_frequency (GtkScale *scale, gdouble value, gpointer data)
{
  return value >= 1000000 ?
    g_strdup_printf ("%s %.2f GHz", (gchar*)data, value / 1000000) :
    g_strdup_printf ("%s %.0f MHz", (gchar*)data, value / 1000);
}



static gboolean
change_min_perf (gpointer data)
{
  GError *err = NULL;
  gdouble value = gtk_range_get_value (GTK_RANGE (data));

  cpufreq_dbus_set_min_perf_pct (xfce4::sprintf ("%.0f", value).c_str(), &err);
  if (err != NULL)
  {
    xfce_dialog_show_error (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (cpuFreq->plugin))),
      err, "Error while setting min perf");
    g_clear_error (&err);
  }
  return false;
}



static gboolean
change_max_perf (gpointer data)
{
  GError *err = NULL;
  gdouble value = gtk_range_get_value (GTK_RANGE (data));

  cpufreq_dbus_set_max_perf_pct (xfce4::sprintf ("%.0f", value).c_str(), &err);
  if (err != NULL)
  {
    xfce_dialog_show_error (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (cpuFreq->plugin))),
      err, "Error while setting max perf");
    g_clear_error (&err);
  }
  return false;
}


static gboolean
change_min_freq (gpointer data)
{
  GError *err = NULL;
  gint cpu = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (data), "cpu"));
  gdouble value = gtk_range_get_value (GTK_RANGE (data));
  auto switcher = (GtkWidget*) g_object_get_data (G_OBJECT (cpuFreq->plugin), "all-cpu-switcher");
  gboolean all = gtk_switch_get_active (GTK_SWITCH (switcher));

  if (all)
    cpu = cpuFreq->cpus.size();

  cpufreq_dbus_set_min_freq (xfce4::sprintf ("%.0f", value).c_str(), cpu, all, &err);
  if (err != NULL)
  {
    xfce_dialog_show_error (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (cpuFreq->plugin))),
      err, "Error while setting min frequency");
    g_clear_error (&err);
  }
  return false;
}



static gboolean
change_max_freq (gpointer data)
{
  GError *err = NULL;
  gint cpu = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (data), "cpu"));
  gdouble value = gtk_range_get_value (GTK_RANGE (data));
  auto switcher = (GtkWidget*) g_object_get_data (G_OBJECT (cpuFreq->plugin), "all-cpu-switcher");
  gboolean all = gtk_switch_get_active (GTK_SWITCH (switcher));

  if (all)
    cpu = cpuFreq->cpus.size();

  cpufreq_dbus_set_max_freq (xfce4::sprintf ("%.0f", value).c_str(), cpu, all, &err);
  if (err != NULL)
  {
    xfce_dialog_show_error (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (cpuFreq->plugin))),
      err, "Error while setting max frequency");
    g_clear_error (&err);
  }
  return false;
}



/* to avoid continous calling to dbus functions,
 * we wait 1 second to make sure the user finished
 * selecting a new value on the slider (scale).
 */
static void
scale_max_freq_changed (GtkWidget *scale, gpointer data)
{
  g_timeout_add_seconds (1, change_max_freq, scale);
}

static void
scale_min_freq_changed (GtkWidget *scale, gpointer data)
{
  g_timeout_add_seconds (1, change_min_freq, scale);
}

static void
scale_max_perf_changed (GtkWidget *scale, gpointer data)
{
  g_timeout_add_seconds (1, change_max_perf, scale);
}

static void
scale_min_perf_changed (GtkWidget *scale, gpointer data)
{
  g_timeout_add_seconds (1, change_min_perf, scale);
}



static void
combo_governor_changed (GtkWidget *combo, gpointer data)
{
  int cpu = GPOINTER_TO_INT (data);
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
combo_preference_changed (GtkWidget *combo, gpointer data)
{
  int cpu = GPOINTER_TO_INT (data);
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
combo_status_changed (GtkWidget *combo, gpointer data)
{
  gchar *status = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (combo));
  if (status != NULL)
  {
    GError *err = NULL;
    cpufreq_dbus_set_status (status, &err);
    if (err != NULL)
    {
      xfce_dialog_show_error (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (cpuFreq->plugin))),
        err, "Error while setting status");
      g_clear_error (&err);
    }
    g_free (status);
  }
}



static void
switch_no_turbo_changed (GtkSwitch *swtch, gboolean state, gpointer data)
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
switch_hwp_dynamic_boost_changed (GtkSwitch *swtch, gboolean state, gpointer data)
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



/* 
 * since gtk_expander_set_label_fill() is deprecated, due to a bug in
 * gtk3 clickable items on the expander label must be attached only after
 * gtk_widget_show_all() is called, or use this method to re-attach them.
 */
static void
expander_fix_clickable_label (GtkWidget *expander, gpointer data)
{
  GtkWidget *hbox;
  hbox = gtk_expander_get_label_widget (GTK_EXPANDER (expander));

  g_object_ref (G_OBJECT (expander));
  g_object_ref (G_OBJECT (hbox));

  gtk_expander_set_label_widget (GTK_EXPANDER (expander), NULL);
  gtk_expander_set_label_widget (GTK_EXPANDER (expander), hbox);

  g_object_unref (G_OBJECT (hbox));
  g_object_unref (G_OBJECT (expander));
}



static void
cpufreq_overview_add (const Ptr<const CpuInfo> &cpu, guint cpu_number, GtkWidget *dialog_hbox)
{
  GtkWidget *hbox, *label, *expander;
  const CpuFreqUnit unit = cpuFreq->options->unit;

  /* Copy shared fields to a local variable. The local variable can then be accessed without a mutex. */
  CpuInfo::Shared cpu_shared;
  {
      std::lock_guard<std::mutex> guard(cpu->mutex);
      cpu_shared = cpu->shared;
  }

  GtkWidget *dialog_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_halign (dialog_vbox, GTK_ALIGN_START);
  gtk_widget_set_sensitive (dialog_vbox, cpu_shared.online);
  gtk_box_pack_start (GTK_BOX (dialog_hbox), dialog_vbox, true, true, 0);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (dialog_vbox), hbox, true, true, BORDER*2);

  GtkWidget *icon = gtk_image_new_from_icon_name ("xfce4-cpufreq-plugin", GTK_ICON_SIZE_BUTTON);
  gtk_widget_set_valign (icon, GTK_ALIGN_START);
  gtk_widget_set_margin_end (icon, 5);
  gtk_box_pack_start (GTK_BOX (hbox), icon, false, false, 0);

  expander = gtk_expander_new (NULL);
  gtk_box_pack_start (GTK_BOX (hbox), expander, true, true, 0);
  gtk_widget_set_valign (expander, GTK_ALIGN_CENTER);
  g_signal_connect (expander, "map", G_CALLBACK (expander_fix_clickable_label), NULL);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_expander_set_label_widget (GTK_EXPANDER (expander), hbox);

  label = gtk_label_new (xfce4::sprintf ("<b>CPU %u</b>", cpu_number).c_str());
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
  gtk_widget_set_margin_end (label, 50);
  gtk_label_set_xalign (GTK_LABEL (label), 0);
  gtk_label_set_use_markup (GTK_LABEL (label), true);
  gtk_box_pack_start (GTK_BOX (hbox), label, true, true, 0);

  {
    std::string current_freq = cpufreq_get_human_readable_freq (cpu_shared.cur_freq, unit);
    label = gtk_label_new (xfce4::sprintf ("<b>%s</b>", current_freq.c_str()).c_str());
    gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_end (label, 10);
    gtk_label_set_xalign (GTK_LABEL (label), 0);
    gtk_label_set_use_markup (GTK_LABEL (label), true);
    gtk_box_pack_start (GTK_BOX (hbox), label, true, true, 0);
  }

#ifdef __linux__
  /* display list of available governors */
  if (!cpu->available_governors.empty()) /* Linux 2.6 and cpu scaling support */
  {
    GtkWidget *combo = gtk_combo_box_text_new ();
    gtk_box_pack_start (GTK_BOX (hbox), combo, true, true, 10);

    gint i = 0;
    for (size_t j = 0; j < cpu->available_governors.size(); j++)
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
    for (size_t j = 0; j < cpu->available_preferences.size(); j++)
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

  GtkWidget *vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (expander), vbox);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, true, true, 20);

  label = gtk_label_new (_("Frequency"));
  gtk_widget_set_margin_end (label, 20);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
  gtk_label_set_xalign (GTK_LABEL (label), 0);
  gtk_box_pack_start (GTK_BOX (hbox), label, false, false, 0);

  GtkWidget *scale = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, cpu->min_freq, cpu->max_freq_nominal, 100);
  g_object_set_data (G_OBJECT (scale), "cpu", GINT_TO_POINTER (cpu_number));
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_LEFT);
  gtk_range_set_value (GTK_RANGE (scale), cpu_shared.cur_min_freq);
  gtk_box_pack_start (GTK_BOX (hbox), scale, true, true, 0);
  g_signal_connect (scale, "format-value", G_CALLBACK (scale_format_frequency), (gpointer) "Min");
  g_signal_connect (scale, "value-changed", G_CALLBACK (scale_min_freq_changed), NULL);

  scale = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, cpu->min_freq, cpu->max_freq_nominal, 100);
  g_object_set_data (G_OBJECT (scale), "cpu", GINT_TO_POINTER (cpu_number));
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_LEFT);
  gtk_range_set_value (GTK_RANGE (scale), cpu_shared.cur_max_freq);
  gtk_box_pack_start (GTK_BOX (hbox), scale, true, true, 0);
  g_signal_connect (scale, "format-value", G_CALLBACK (scale_format_frequency), (gpointer) "Max");
  g_signal_connect (scale, "value-changed", G_CALLBACK (scale_max_freq_changed), NULL);
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
    step = 2;
  else
    step = 3;

  GtkWidget *hbox, *vbox, *label, *switcher, *icon, *expander;
  GtkSizeGroup *sgv = gtk_size_group_new (GTK_SIZE_GROUP_VERTICAL);
  GtkSizeGroup *sgh = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  GtkSizeGroup *sg0 = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  /* display driver */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_set_margin_bottom (hbox, 20);
  gtk_box_pack_start (GTK_BOX (dialog_vbox), hbox, true, true, 0);

  icon = gtk_image_new_from_icon_name ("xfce4-cpufreq-plugin", GTK_ICON_SIZE_BUTTON);
  gtk_widget_set_halign (icon, GTK_ALIGN_END);
  gtk_widget_set_valign (icon, GTK_ALIGN_START);
  gtk_widget_set_margin_end (icon, 5);
  gtk_box_pack_start (GTK_BOX (hbox), icon, false, false, 0);
  gtk_size_group_add_widget (sgv, icon);

  expander = gtk_expander_new (NULL);
  gtk_box_pack_end (GTK_BOX (hbox), expander, true, true, 0);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_size_group_add_widget (sgh, hbox);
  gtk_expander_set_label_widget (GTK_EXPANDER (expander), hbox);

  label = gtk_label_new (_("<b>Scaling driver:</b>"));
  gtk_widget_set_margin_start (label, 5);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
  gtk_label_set_xalign (GTK_LABEL (label), 0);
  gtk_label_set_use_markup (GTK_LABEL (label), true);
  gtk_box_pack_start (GTK_BOX (hbox), label, true, true, 0);
  gtk_size_group_add_widget (sgv, label);

  {
    std::string text;
    if (!cpuFreq->cpus[0]->scaling_driver.empty())
      text = "<b>" + cpuFreq->cpus[0]->scaling_driver + "</b>";
    else
      text = xfce4::sprintf (_("No scaling driver available"));

    label = gtk_label_new (text.c_str());
    gtk_widget_set_margin_end (label, 20);
    gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
    gtk_label_set_xalign (GTK_LABEL (label), 1);
    gtk_label_set_use_markup (GTK_LABEL (label), true);
    gtk_box_pack_end (GTK_BOX (hbox), label, false, false, 0);
    gtk_size_group_add_widget (sgv, label);
  }

  if (cpuFreq->intel_pstate != nullptr)
  {
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_container_add (GTK_CONTAINER (expander), hbox);

    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, BORDER*2);
    gtk_box_pack_start (GTK_BOX (hbox), vbox, true, true, BORDER*8);

    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, true, true, 0);

    label = gtk_label_new (_("Status"));
    gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
    gtk_widget_set_halign (label, GTK_ALIGN_START);
    gtk_label_set_xalign (GTK_LABEL (label), 0);
    gtk_box_pack_start (GTK_BOX (hbox), label, true, true, 0);

    GtkWidget *combo = gtk_combo_box_text_new ();
    gtk_box_pack_end (GTK_BOX (hbox), combo, false, false, 0);
    gtk_size_group_add_widget (sg0, combo);

    const gchar *pstate_status[] = {"active", "passive", "off"};
    const size_t size = std::extent<decltype(pstate_status)>::value;
    for (size_t i=0; i < size; i++)
    {
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), pstate_status[i]);
      if (g_ascii_strcasecmp (pstate_status[i], cpuFreq->intel_pstate->status.c_str()) == 0)
      {
        gtk_combo_box_set_active (GTK_COMBO_BOX (combo), i);
      }
    }
    g_signal_connect (combo, "changed", G_CALLBACK (combo_status_changed), NULL);

    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, false, false, 0);

    label = gtk_label_new (_("HWP Dynamic Boost"));
    gtk_widget_set_tooltip_text (label, _("If set, causes the minimum P-state limit to be increased dynamically for a short time whenever a task previously waiting on I/O is selected to run on a given logical CPU (the purpose of this mechanism is to improve performance)"));
    gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
    gtk_label_set_xalign (GTK_LABEL (label), 0);
    gtk_box_pack_start (GTK_BOX (hbox), label, false, false, 0);

    switcher = gtk_switch_new ();
    gtk_switch_set_active (GTK_SWITCH (switcher), cpuFreq->intel_pstate->hwp_dynamic_boost);
    gtk_box_pack_end (GTK_BOX (hbox), switcher, false, false, 0);
    g_signal_connect (switcher, "state-set", G_CALLBACK (switch_hwp_dynamic_boost_changed), NULL);

    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, false, false, BORDER*2);

    label = gtk_label_new (_("No Turbo"));
    gtk_widget_set_tooltip_text (label, _("If set, the driver is not allowed to set any turbo P-states (see Turbo P-states Support). If unset (which is the default), turbo P-states can be set by the driver"));
    gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
    gtk_label_set_xalign (GTK_LABEL (label), 0);
    gtk_box_pack_start (GTK_BOX (hbox), label, false, false, 0);

    switcher = gtk_switch_new ();
    gtk_switch_set_active (GTK_SWITCH (switcher), cpuFreq->intel_pstate->no_turbo);
    gtk_box_pack_end (GTK_BOX (hbox), switcher, false, false, 0);
    g_signal_connect (switcher, "state-set", G_CALLBACK (switch_no_turbo_changed), NULL);

    /* display Performance attributes only if available */
    if (cpuFreq->intel_pstate->max_perf_pct)
    {
      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, false, false, BORDER*2);

      label = gtk_label_new (_("Performance"));
      gtk_widget_set_tooltip_text (label, _("Set limits the driver is allowed to set (in percent) of the minimum/maximum supported performance level"));
      gtk_widget_set_margin_end (label, 50);
      gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
      gtk_label_set_xalign (GTK_LABEL (label), 0);
      gtk_box_pack_start (GTK_BOX (hbox), label, false, false, 0);
      gtk_size_group_add_widget (sg0, label);

      GtkWidget *scale = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
      gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_LEFT);
      gtk_range_set_value (GTK_RANGE (scale), cpuFreq->intel_pstate->min_perf_pct);
      gtk_box_pack_start (GTK_BOX (hbox), scale, true, true, 0);
      g_signal_connect (scale, "format-value", G_CALLBACK (scale_format_perf_pct), (gpointer) "Min");
      g_signal_connect (scale, "value-changed", G_CALLBACK (scale_min_perf_changed), NULL);

      scale = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
      gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_LEFT);
      gtk_range_set_value (GTK_RANGE (scale), cpuFreq->intel_pstate->max_perf_pct);
      gtk_box_pack_start (GTK_BOX (hbox), scale, true, true, 0);
      g_signal_connect (scale, "format-value", G_CALLBACK (scale_format_perf_pct), (gpointer) "Max");
      g_signal_connect (scale, "value-changed", G_CALLBACK (scale_max_perf_changed), NULL);
    }
  }

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (dialog_vbox), hbox, true, true, 0);

  icon = gtk_image_new_from_icon_name ("xfce4-cpufreq-plugin", GTK_ICON_SIZE_BUTTON);
  gtk_widget_set_halign (icon, GTK_ALIGN_END);
  gtk_widget_set_valign (icon, GTK_ALIGN_START);
  gtk_widget_set_margin_end (icon, 5);
  gtk_box_pack_start (GTK_BOX (hbox), icon, false, false, 0);
  gtk_size_group_add_widget (sgv, icon);

  expander = gtk_expander_new (NULL);
  gtk_widget_set_valign (expander, GTK_ALIGN_CENTER);
  gtk_expander_set_expanded (GTK_EXPANDER (expander), true);
  gtk_box_pack_start (GTK_BOX (hbox), expander, true, true, 0);

  label = gtk_label_new (_("<b>CPU Settings:</b>"));
  gtk_widget_set_margin_start (label, 5);
  gtk_widget_set_margin_end (label, 20);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
  gtk_label_set_xalign (GTK_LABEL (label), 0);
  gtk_label_set_use_markup (GTK_LABEL (label), true);
  gtk_expander_set_label_widget ( GTK_EXPANDER (expander), label);
  gtk_size_group_add_widget (sgv, label);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (expander), vbox);
  gtk_size_group_add_widget (sgh, vbox);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, true, true, 0);

  for (size_t i = 0; i < step; i++)
  {
    GtkWidget *box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_halign (box, GTK_ALIGN_END);
    gtk_box_pack_start (GTK_BOX (hbox), box, true, true, 0);

    if (i + 1 < step)
    {
      GtkWidget *separator = gtk_separator_new (GTK_ORIENTATION_VERTICAL);
      gtk_box_pack_end (GTK_BOX (box), separator, false, false, 0);
    }
    label = gtk_label_new (_("Governor"));
    gtk_widget_set_margin_end (label, 70);
    gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
    gtk_widget_set_halign (label, GTK_ALIGN_END);
    gtk_box_pack_start (GTK_BOX (box), label, false, false, 0);

    label = gtk_label_new (_("Preference"));
    gtk_widget_set_margin_end (label, 110);
    gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
    gtk_widget_set_halign (label, GTK_ALIGN_END);
    gtk_box_pack_start (GTK_BOX (box), label, false, false, 0);
  }

  /* display list of CPUs */
  for (size_t i = 0; i < cpuFreq->cpus.size(); i += step)
  {
    GtkWidget *dialog_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start (GTK_BOX (vbox), dialog_hbox, true, true, 0);
    gtk_container_set_border_width (GTK_CONTAINER (dialog_hbox), 0);

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

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, true, true, 10);

  label = gtk_label_new (_("Changes applies to ALL CPUs"));
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
  gtk_label_set_xalign (GTK_LABEL (label), 0);
  gtk_box_pack_start (GTK_BOX (hbox), label, true, true, 0);

  switcher = gtk_switch_new ();
  gtk_switch_set_active (GTK_SWITCH (switcher), true);
  gtk_box_pack_end (GTK_BOX (hbox), switcher, false, false, BORDER*8);
  g_object_set_data (G_OBJECT (cpuFreq->plugin), "all-cpu-switcher", switcher);

  xfce4::connect_response (GTK_DIALOG (dialog), cpufreq_overview_response);

  gtk_widget_show_all (dialog);

  return true;
}
