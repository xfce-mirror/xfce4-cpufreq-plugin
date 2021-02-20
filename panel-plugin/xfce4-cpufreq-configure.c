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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libxfce4ui/libxfce4ui.h>
#include "xfce4-cpufreq-plugin.h"
#include "xfce4-cpufreq-configure.h"



static void
update_sensitivity (const CpuFreqPluginConfigure *configure)
{
  const CpuFreqPluginOptions *options = cpuFreq->options;

  if (!options->show_label_freq && !options->show_label_governor)
    gtk_widget_set_sensitive (configure->display_icon, FALSE);
  else
    gtk_widget_set_sensitive (configure->display_icon, TRUE);
}



static void
validate_configuration (const CpuFreqPluginConfigure *configure)
{
  CpuFreqPluginOptions *options = cpuFreq->options;

  if (!options->show_label_freq && !options->show_label_governor)
  {
    if (!options->show_icon)
    {
      options->show_icon = TRUE;
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (configure->display_icon), TRUE);
      update_sensitivity (configure);
    }
  }
}



static void
check_button_changed (GtkWidget *button, const CpuFreqPluginConfigure *configure)
{
  CpuFreqPluginOptions *options = cpuFreq->options;

  if (button == configure->display_icon)
    options->show_icon = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));

  else if (button == configure->display_freq)
    options->show_label_freq = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));

  else if (button == configure->display_governor)
    options->show_label_governor = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));

  else if (button == configure->keep_compact)
    options->keep_compact = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));

  else if (button == configure->one_line)
    options->one_line = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));

  update_sensitivity (configure);
  validate_configuration (configure);

  cpufreq_prepare_label (cpuFreq);
  cpufreq_update_icon (cpuFreq);
  cpufreq_update_plugin (TRUE);
}



static void
button_fontname_update(GtkWidget *button, gboolean update_plugin)
{
  if (cpuFreq->options->fontname == NULL)
  {
    gtk_button_set_label (GTK_BUTTON (button), _("Select font..."));
    gtk_widget_set_tooltip_text (button, _("Select font family and size to use for the labels."));
  }
  else
  {
    gtk_button_set_label (GTK_BUTTON (button), cpuFreq->options->fontname);
    gtk_widget_set_tooltip_text (button, _("Right-click to revert to the default font."));
  }

  if (update_plugin)
    cpufreq_update_plugin (TRUE);
}



static gboolean
button_fontname_clicked(GtkWidget *button, CpuFreqPluginConfigure *configure)
{
  GtkWidget *fc;
  gchar *fontname;
  gint result;

  fc = gtk_font_chooser_dialog_new (_("Select font"),
    GTK_WINDOW(gtk_widget_get_toplevel(button)));

  if (cpuFreq->options->fontname)
    gtk_font_chooser_set_font (GTK_FONT_CHOOSER (fc), cpuFreq->options->fontname);

  result = gtk_dialog_run(GTK_DIALOG(fc));

  if (result == GTK_RESPONSE_OK || result == GTK_RESPONSE_ACCEPT)
  {
    fontname = gtk_font_chooser_get_font (GTK_FONT_CHOOSER (fc));

    if (fontname != NULL)
    {
      gtk_button_set_label(GTK_BUTTON(button), fontname);
      g_free (cpuFreq->options->fontname);
      cpuFreq->options->fontname = fontname;
    }

    button_fontname_update(button, TRUE);
  }

  gtk_widget_destroy(GTK_WIDGET(fc));
  return TRUE;
}



static gboolean
button_fontname_pressed(GtkWidget *button, GdkEventButton *event,
                        CpuFreqPluginConfigure *configure)
{
  if (event->type != GDK_BUTTON_PRESS)
    return FALSE;

  /* right mouse click clears the font name and resets the button */
  if (event->button == 3 && cpuFreq->options->fontname)
  {
    g_free (cpuFreq->options->fontname);
    cpuFreq->options->fontname = NULL;
    button_fontname_update(button, TRUE);
    return TRUE;
  }

  /* left mouse click will be handled in a different function */
  return FALSE;
}



static void
button_fontcolor_clicked (GtkWidget *button, void *data)
{
  GdkRGBA color;
  gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (button), &color);
  g_free (cpuFreq->options->fontcolor);
  cpuFreq->options->fontcolor = gdk_rgba_to_string (&color);
  cpufreq_update_plugin (TRUE);
}



static void
combo_changed (GtkWidget *combo, CpuFreqPluginConfigure *configure)
{
  CpuFreqPluginOptions *const options = cpuFreq->options;
  guint selected = gtk_combo_box_get_active (GTK_COMBO_BOX (combo));

  if (GTK_WIDGET (combo) == configure->combo_cpu)
  {
    guint num_cpus = cpuFreq->cpus->len;
    if (selected < num_cpus)
      options->show_cpu = selected;
    else if (selected == num_cpus + 0)
      options->show_cpu = CPU_MIN;
    else if (selected == num_cpus + 1)
      options->show_cpu = CPU_AVG;
    else if (selected == num_cpus + 2)
      options->show_cpu = CPU_MAX;

    cpufreq_update_plugin (TRUE);
  }
  else if (GTK_WIDGET (combo) == configure->combo_unit)
  {
    switch (selected)
    {
    case UNIT_AUTO:
    case UNIT_GHZ:
    case UNIT_MHZ:
      options->unit = (CpuFreqUnit) selected;
      break;
    }

    cpufreq_update_plugin (TRUE);
  }
}



static void
spinner_changed (GtkWidget *spinner, CpuFreqPluginConfigure *configure)
{
  cpuFreq->options->timeout =gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (spinner));
  cpufreq_restart_timeout ();
}



static void
cpufreq_configure_response (GtkWidget *dialog, int response, CpuFreqPluginConfigure *configure)
{
  g_object_set_data (G_OBJECT (cpuFreq->plugin), "configure", NULL);
  xfce_panel_plugin_unblock_menu (cpuFreq->plugin);
  gtk_widget_destroy (dialog);

  cpufreq_write_config (cpuFreq->plugin);

  g_free (configure);
}



void
cpufreq_configure (XfcePanelPlugin *plugin)
{
  CpuFreqPluginOptions *const options = cpuFreq->options;
  gchar *cpu_name;
  GtkWidget *dialog, *dialog_vbox;
  GtkWidget *frame, *align, *label, *vbox, *hbox;
  GtkWidget *spinner, *button;
  GtkSizeGroup *sg0;
  CpuFreqPluginConfigure *configure;
  GdkRGBA *color;

  configure = g_new0 (CpuFreqPluginConfigure, 1);

  xfce_panel_plugin_block_menu (cpuFreq->plugin);

  dialog = xfce_titled_dialog_new_with_mixed_buttons (_("Configure CPU Frequency Monitor"),
    GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (plugin))),
    GTK_DIALOG_DESTROY_WITH_PARENT,
    "window-close", _("_Close"), GTK_RESPONSE_OK,
    NULL);

  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
  gtk_window_set_icon_name (GTK_WINDOW (dialog), "xfce4-cpufreq-plugin");
  gtk_window_set_keep_above (GTK_WINDOW (dialog), TRUE);
  gtk_window_stick (GTK_WINDOW (dialog));

  g_object_set_data (G_OBJECT (cpuFreq->plugin), "configure", dialog);

  dialog_vbox = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  gtk_container_set_border_width (GTK_CONTAINER (dialog_vbox), 12);
  gtk_box_set_spacing (GTK_BOX (dialog_vbox), 18);

  sg0 = gtk_size_group_new(GTK_SIZE_GROUP_BOTH);

  /* monitor behaviours */
  frame = gtk_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (dialog_vbox), frame, FALSE, TRUE, 0);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);

  label = gtk_label_new (_("<b>Monitor</b>"));
  gtk_frame_set_label_widget (GTK_FRAME (frame), label);
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);

  align = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_halign(align, GTK_ALIGN_START);
  gtk_widget_set_valign(align, GTK_ALIGN_CENTER);
  gtk_widget_set_hexpand(align, TRUE);
  gtk_widget_set_vexpand(align, TRUE);

  gtk_container_add (GTK_CONTAINER (frame), align);
  gtk_widget_set_margin_top (align, 6);
  gtk_widget_set_margin_start (align, 12);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_container_add (GTK_CONTAINER (align), hbox);

  label = gtk_label_new_with_mnemonic (_("_Update interval:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
  gtk_size_group_add_widget (sg0, label);

  spinner = configure->spinner_timeout =
    gtk_spin_button_new_with_range (TIMEOUT_MIN, TIMEOUT_MAX, TIMEOUT_STEP);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), spinner);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spinner), (gdouble) options->timeout);
  gtk_box_pack_start (GTK_BOX (hbox), spinner, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (spinner), "value-changed",
                    G_CALLBACK (spinner_changed), configure);

  /* panel behaviours */
  frame = gtk_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (dialog_vbox), frame, FALSE, TRUE, 0);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);

  label = gtk_label_new (_("<b>Panel</b>"));
  gtk_frame_set_label_widget (GTK_FRAME (frame), label);
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);

  align = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_halign(align, GTK_ALIGN_FILL);
  gtk_widget_set_valign(align, GTK_ALIGN_CENTER);
  gtk_widget_set_hexpand(align, TRUE);
  gtk_widget_set_vexpand(align, TRUE);

  gtk_container_add (GTK_CONTAINER (frame), align);
  gtk_widget_set_margin_top (align, 6);
  gtk_widget_set_margin_start (align, 12);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (align), vbox);

  /* font settings */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  label = gtk_label_new_with_mnemonic (_("_Font:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
  gtk_label_set_xalign (GTK_LABEL (label), 0);
  gtk_size_group_add_widget (sg0, label);

  button = configure->fontname = gtk_button_new ();
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), button);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (button_fontname_clicked), configure);
  g_signal_connect (G_OBJECT (button), "button_press_event",
                    G_CALLBACK (button_fontname_pressed), configure);
  button_fontname_update (button, FALSE);

  /* font color */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  label = gtk_label_new_with_mnemonic (_("_Font color:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
  gtk_label_set_xalign (GTK_LABEL (label), 0);
  gtk_size_group_add_widget (sg0, label);

  color = g_new0 (GdkRGBA, 1);
  gdk_rgba_parse (color, options->fontcolor ? options->fontcolor : "#000000");

  button = configure->fontcolor = gtk_color_button_new_with_rgba (color);
  gtk_color_button_set_title (GTK_COLOR_BUTTON (button), _("Select font color"));
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, TRUE, 0);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), button);
  g_signal_connect (button, "color-set", G_CALLBACK (button_fontcolor_clicked), NULL);
  g_free (color);

  /* which cpu to show in panel */
  {
    GtkWidget *combo;

    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new_with_mnemonic (_("_Display CPU:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
    gtk_label_set_xalign (GTK_LABEL (label), 0);
    gtk_size_group_add_widget (sg0, label);

    combo = configure->combo_cpu = gtk_combo_box_text_new ();
    gtk_box_pack_start (GTK_BOX (hbox), combo, FALSE, TRUE, 0);
    gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);

    for (guint i = 0; i < cpuFreq->cpus->len; ++i)
    {
      cpu_name = g_strdup_printf ("%d", i);
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), cpu_name);
      g_free (cpu_name);
    }

    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), _("min"));
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), _("avg"));
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), _("max"));

  retry_cpu:
    switch (options->show_cpu)
    {
    case CPU_MIN:
      gtk_combo_box_set_active (GTK_COMBO_BOX (combo), cpuFreq->cpus->len + 0);
      break;
    case CPU_AVG:
      gtk_combo_box_set_active (GTK_COMBO_BOX (combo), cpuFreq->cpus->len + 1);
      break;
    case CPU_MAX:
      gtk_combo_box_set_active (GTK_COMBO_BOX (combo), cpuFreq->cpus->len + 2);
      break;
    default:
      if (options->show_cpu >= 0 && options->show_cpu < (gint) cpuFreq->cpus->len)
        gtk_combo_box_set_active (GTK_COMBO_BOX (combo), options->show_cpu);
      else
      {
        options->show_cpu = CPU_DEFAULT;
        goto retry_cpu;
      }
    }

    g_signal_connect (G_OBJECT (combo), "changed", G_CALLBACK (combo_changed), configure);
  }

  /* which unit to use when displaying the frequency */
  {
    GtkWidget *combo;

    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new_with_mnemonic (_("Unit:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
    gtk_label_set_xalign (GTK_LABEL (label), 0);
    gtk_size_group_add_widget (sg0, label);

    combo = configure->combo_unit = gtk_combo_box_text_new ();
    gtk_box_pack_start (GTK_BOX (hbox), combo, FALSE, TRUE, 0);
    gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);

    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), _("Auto"));
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), _("GHz"));
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), _("MHz"));

  retry_unit:
    switch (options->unit)
    {
    case UNIT_AUTO:
      gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);
      break;
    case UNIT_GHZ:
      gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 1);
      break;
    case UNIT_MHZ:
      gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 2);
      break;
    default:
      options->unit = UNIT_DEFAULT;
      goto retry_unit;
    }

    g_signal_connect (G_OBJECT (combo), "changed", G_CALLBACK (combo_changed), configure);
  }

  /* check buttons for display widgets in panel */
  button = configure->keep_compact = gtk_check_button_new_with_mnemonic (_("_Keep compact"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), options->keep_compact);
  g_signal_connect (G_OBJECT (button), "toggled", G_CALLBACK (check_button_changed), configure);

  button = configure->one_line = gtk_check_button_new_with_mnemonic (_("Show text in a single _line"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), options->one_line);
  g_signal_connect (G_OBJECT (button), "toggled", G_CALLBACK (check_button_changed), configure);

  button = configure->display_icon = gtk_check_button_new_with_mnemonic (_("Show CPU _icon"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), options->show_icon);
  g_signal_connect (G_OBJECT (button), "toggled", G_CALLBACK (check_button_changed), configure);

  button = configure->display_freq = gtk_check_button_new_with_mnemonic (_("Show CPU fre_quency"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), options->show_label_freq);
  g_signal_connect (G_OBJECT (button), "toggled", G_CALLBACK (check_button_changed), configure);

  button = configure->display_governor = gtk_check_button_new_with_mnemonic (_("Show CPU _governor"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), options->show_label_governor);
  g_signal_connect (G_OBJECT (button), "toggled", G_CALLBACK (check_button_changed), configure);

  g_signal_connect(G_OBJECT (dialog), "response", G_CALLBACK(cpufreq_configure_response), configure);

  update_sensitivity (configure);
  validate_configuration (configure);

  g_object_unref (sg0);
  gtk_widget_show_all (dialog);
}
