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

/* The fixes file has to be included before any other #include directives */
#include "xfce4++/util/fixes.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libxfce4ui/libxfce4ui.h>
#include "xfce4-cpufreq-plugin.h"
#include "xfce4-cpufreq-configure.h"



CpuFreqPluginConfigure::~CpuFreqPluginConfigure()
{
  g_info ("%s", __PRETTY_FUNCTION__);
}



static void
update_sensitivity (const Ptr<CpuFreqPluginConfigure> &configure)
{
  auto options = cpuFreq->options;

  if (!options->show_label_freq && !options->show_label_governor)
  {
    gtk_widget_set_sensitive (configure->display_icon, false);
    gtk_widget_set_sensitive (configure->fontcolor_hbox, false);
    gtk_widget_set_sensitive (configure->fontname_hbox, false);
  }
  else
  {
    gtk_widget_set_sensitive (configure->display_icon, true);
    gtk_widget_set_sensitive (configure->fontcolor_hbox, true);
    gtk_widget_set_sensitive (configure->fontname_hbox, true);
  }

  gtk_widget_set_sensitive (configure->icon_color_freq, options->show_icon);
}



static void
validate_configuration (const Ptr<CpuFreqPluginConfigure> &configure)
{
  auto options = cpuFreq->options;

  if (!options->show_label_freq && !options->show_label_governor)
  {
    if (!options->show_icon)
    {
      options->show_icon = true;
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (configure->display_icon), true);
      update_sensitivity (configure);
    }
  }
}



static void
check_button_changed (GtkWidget *button, const Ptr<CpuFreqPluginConfigure> &configure)
{
  auto options = cpuFreq->options;

  if (button == configure->display_icon)
    options->show_icon = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));

  else if (button == configure->display_freq)
    options->show_label_freq = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));

  else if (button == configure->display_governor)
    options->show_label_governor = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));

  else if (button == configure->icon_color_freq)
    options->icon_color_freq = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));

  else if (button == configure->keep_compact)
    options->keep_compact = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));

  else if (button == configure->one_line)
    options->one_line = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));

  update_sensitivity (configure);
  validate_configuration (configure);

  cpufreq_prepare_label ();
  cpufreq_update_icon ();
  cpufreq_update_plugin (true);
}



static void
button_fontname_update(GtkButton *button, gboolean update_plugin)
{
  if (cpuFreq->options->fontname.empty())
  {
    gtk_button_set_label (button, _("Select font..."));
    gtk_widget_set_tooltip_text (GTK_WIDGET (button), _("Select font family and size to use for the labels."));
  }
  else
  {
    gtk_button_set_label (button, cpuFreq->options->fontname.c_str());
    gtk_widget_set_tooltip_text (GTK_WIDGET (button), _("Right-click to revert to the default font."));
  }

  if (update_plugin)
    cpufreq_update_plugin (true);
}



static void
button_fontname_clicked(GtkButton *button)
{
  GtkWidget *fc = gtk_font_chooser_dialog_new (_("Select font"), GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (button))));

  if (!cpuFreq->options->fontname.empty())
    gtk_font_chooser_set_font (GTK_FONT_CHOOSER (fc), cpuFreq->options->fontname.c_str());

  gint result = gtk_dialog_run(GTK_DIALOG(fc));

  if (result == GTK_RESPONSE_OK || result == GTK_RESPONSE_ACCEPT)
  {
    gchar *fontname = gtk_font_chooser_get_font (GTK_FONT_CHOOSER (fc));

    if (fontname != NULL)
    {
      gtk_button_set_label (button, fontname);
      cpuFreq->set_font (fontname);
      g_free (fontname);
    }

    button_fontname_update(button, true);
  }

  gtk_widget_destroy(GTK_WIDGET(fc));
}



static xfce4::Propagation
button_fontname_pressed(GtkWidget *button, GdkEventButton *event)
{
  if (event->type != GDK_BUTTON_PRESS)
    return xfce4::PROPAGATE;

  /* right mouse click clears the font name and resets the button */
  if (event->button == 3 && !cpuFreq->options->fontname.empty())
  {
    cpuFreq->set_font ("");
    button_fontname_update(GTK_BUTTON (button), true);
    return xfce4::STOP;
  }

  /* left mouse click will be handled in a different function */
  return xfce4::PROPAGATE;
}



static void
button_fontcolor_update(GtkWidget *button, bool update_plugin)
{
  if (cpuFreq->options->fontcolor.empty())
  {
    GdkRGBA color = {};
    gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (button), &color);
    gtk_widget_set_tooltip_text (button, NULL);
  }
  else
  {
    gtk_widget_set_tooltip_text (button, _("Right-click to revert to the default color"));
  }

  if (update_plugin)
    cpufreq_update_plugin (true);
}



static void
button_fontcolor_clicked (GtkColorButton *button)
{
  GdkRGBA color;
  gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (button), &color);
  if (color.alpha != 0)
    cpuFreq->options->fontcolor = gdk_rgba_to_string (&color);
  else
    cpuFreq->options->fontcolor.clear();
  button_fontcolor_update (GTK_WIDGET (button), true);
}



static xfce4::Propagation
button_fontcolor_pressed(GtkWidget *button, GdkEventButton *event)
{
  if (event->type != GDK_BUTTON_PRESS)
    return xfce4::PROPAGATE;

  /* right mouse click clears the font color and resets the button */
  if (event->button == 3 && !cpuFreq->options->fontcolor.empty())
  {
    cpuFreq->options->fontcolor.clear();
    button_fontcolor_update(button, true);
    return xfce4::STOP;
  }

  /* left mouse click will be handled in a different function */
  return xfce4::PROPAGATE;
}



static void
combo_changed (GtkComboBox *combo, const Ptr<CpuFreqPluginConfigure> &configure)
{
  auto options = cpuFreq->options;

  guint selected = gtk_combo_box_get_active (combo);

  if (GTK_WIDGET (combo) == configure->combo_cpu)
  {
    size_t num_cpus = cpuFreq->cpus.size();
    if (selected < num_cpus)
      options->show_cpu = selected;
    else if (selected == num_cpus + 0)
      options->show_cpu = CPU_MIN;
    else if (selected == num_cpus + 1)
      options->show_cpu = CPU_AVG;
    else if (selected == num_cpus + 2)
      options->show_cpu = CPU_MAX;

    cpufreq_update_plugin (true);
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

    cpufreq_update_plugin (true);
  }
}



static void
spinner_changed (GtkSpinButton *spinner)
{
  cpuFreq->options->timeout = gtk_spin_button_get_value (spinner);
  cpufreq_restart_timeout ();
}



static void
cpufreq_configure_response (GtkDialog *dialog)
{
  g_object_set_data (G_OBJECT (cpuFreq->plugin), "configure", NULL);
  xfce_panel_plugin_unblock_menu (cpuFreq->plugin);
  gtk_widget_destroy (GTK_WIDGET (dialog));

  cpufreq_write_config (cpuFreq->plugin);
}



void
cpufreq_configure (XfcePanelPlugin *plugin)
{
  auto options = cpuFreq->options;
  GtkWidget *frame, *align, *label, *vbox, *hbox;
  GtkWidget *spinner, *button;
  GdkRGBA color = {};

  auto configure = xfce4::make<CpuFreqPluginConfigure>();

  xfce_panel_plugin_block_menu (cpuFreq->plugin);

  GtkWidget *dialog = xfce_titled_dialog_new_with_mixed_buttons (_("Configure CPU Frequency Monitor"),
    GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (plugin))),
    GTK_DIALOG_DESTROY_WITH_PARENT,
    "window-close-symbolic", _("_Close"), GTK_RESPONSE_OK,
    NULL);

  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
  gtk_window_set_icon_name (GTK_WINDOW (dialog), "xfce4-cpufreq-plugin");
  gtk_window_set_keep_above (GTK_WINDOW (dialog), true);
  gtk_window_stick (GTK_WINDOW (dialog));

  g_object_set_data (G_OBJECT (cpuFreq->plugin), "configure", dialog);

  GtkWidget *dialog_vbox = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  gtk_container_set_border_width (GTK_CONTAINER (dialog_vbox), 12);
  gtk_box_set_spacing (GTK_BOX (dialog_vbox), 18);

  GtkSizeGroup *sg0 = gtk_size_group_new(GTK_SIZE_GROUP_BOTH);

  /* monitor behaviours */
  frame = gtk_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (dialog_vbox), frame, false, true, 0);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);

  label = gtk_label_new (_("<b>Monitor</b>"));
  gtk_frame_set_label_widget (GTK_FRAME (frame), label);
  gtk_label_set_use_markup (GTK_LABEL (label), true);

  align = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_halign(align, GTK_ALIGN_START);
  gtk_widget_set_valign(align, GTK_ALIGN_CENTER);
  gtk_widget_set_hexpand(align, true);
  gtk_widget_set_vexpand(align, true);

  gtk_container_add (GTK_CONTAINER (frame), align);
  gtk_widget_set_margin_top (align, 6);
  gtk_widget_set_margin_start (align, 12);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_container_add (GTK_CONTAINER (align), hbox);

  label = gtk_label_new_with_mnemonic (_("_Update interval:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, false, false, 0);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
  gtk_size_group_add_widget (sg0, label);

  spinner = configure->spinner_timeout = gtk_spin_button_new_with_range (TIMEOUT_MIN, TIMEOUT_MAX, TIMEOUT_STEP);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), spinner);
  gtk_spin_button_set_digits (GTK_SPIN_BUTTON (spinner), 2);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spinner), options->timeout);
  gtk_box_pack_start (GTK_BOX (hbox), spinner, false, false, 0);
  xfce4::connect_value_changed (GTK_SPIN_BUTTON (spinner), [](GtkSpinButton *sb) {
      spinner_changed (sb);
  });

  /* panel behaviours */
  frame = gtk_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (dialog_vbox), frame, false, true, 0);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);

  label = gtk_label_new (_("<b>Panel</b>"));
  gtk_frame_set_label_widget (GTK_FRAME (frame), label);
  gtk_label_set_use_markup (GTK_LABEL (label), true);

  align = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_halign(align, GTK_ALIGN_FILL);
  gtk_widget_set_valign(align, GTK_ALIGN_CENTER);
  gtk_widget_set_hexpand(align, true);
  gtk_widget_set_vexpand(align, true);

  gtk_container_add (GTK_CONTAINER (frame), align);
  gtk_widget_set_margin_top (align, 6);
  gtk_widget_set_margin_start (align, 12);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (align), vbox);

  /* font settings */
  hbox = configure->fontname_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, false, false, 0);

  label = gtk_label_new_with_mnemonic (_("_Font:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, false, false, 0);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
  gtk_label_set_xalign (GTK_LABEL (label), 0);
  gtk_size_group_add_widget (sg0, label);

  button = configure->fontname = gtk_button_new ();
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), button);
  gtk_box_pack_start (GTK_BOX (hbox), button, true, true, 0);
  xfce4::connect_clicked (GTK_BUTTON (button), [](GtkButton *b) {
      button_fontname_clicked (b);
  });
  xfce4::connect_button_press (button, [](GtkWidget *w, GdkEventButton *event) {
      return button_fontname_pressed (w, event);
  });
  button_fontname_update (GTK_BUTTON (button), false);

  /* font color */
  hbox = configure->fontcolor_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, false, false, 0);

  label = gtk_label_new_with_mnemonic (_("_Font color:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, false, false, 0);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
  gtk_label_set_xalign (GTK_LABEL (label), 0);
  gtk_size_group_add_widget (sg0, label);

  if (!options->fontcolor.empty())
    gdk_rgba_parse (&color, options->fontcolor.c_str());

  button = configure->fontcolor = gtk_color_button_new_with_rgba (&color);
  gtk_color_button_set_title (GTK_COLOR_BUTTON (button), _("Select font color"));
  gtk_box_pack_start (GTK_BOX (hbox), button, false, true, 0);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), button);
  xfce4::connect_color_set (GTK_COLOR_BUTTON (button), button_fontcolor_clicked);
  xfce4::connect_button_press (button, [](GtkWidget *w, GdkEventButton *event) {
      return button_fontcolor_pressed (w, event);
  });
  button_fontcolor_update (button, false);

  /* which cpu to show in panel */
  {
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, false, false, 0);

    label = gtk_label_new_with_mnemonic (_("_Display CPU:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, false, false, 0);
    gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
    gtk_label_set_xalign (GTK_LABEL (label), 0);
    gtk_size_group_add_widget (sg0, label);

    GtkWidget *combo = configure->combo_cpu = gtk_combo_box_text_new ();
    gtk_box_pack_start (GTK_BOX (hbox), combo, false, true, 0);
    gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);

    for (size_t i = 0; i < cpuFreq->cpus.size(); i++)
    {
      auto cpu_name = xfce4::sprintf ("%zu", i);
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), cpu_name.c_str());
    }

    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), _("min"));
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), _("avg"));
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), _("max"));

  retry_cpu:
    switch (options->show_cpu)
    {
    case CPU_MIN:
      gtk_combo_box_set_active (GTK_COMBO_BOX (combo), cpuFreq->cpus.size() + 0);
      break;
    case CPU_AVG:
      gtk_combo_box_set_active (GTK_COMBO_BOX (combo), cpuFreq->cpus.size() + 1);
      break;
    case CPU_MAX:
      gtk_combo_box_set_active (GTK_COMBO_BOX (combo), cpuFreq->cpus.size() + 2);
      break;
    default:
      if (options->show_cpu >= 0 && guint(options->show_cpu) < cpuFreq->cpus.size())
        gtk_combo_box_set_active (GTK_COMBO_BOX (combo), options->show_cpu);
      else
      {
        options->show_cpu = CPU_DEFAULT;
        goto retry_cpu;
      }
    }

    xfce4::connect_changed (GTK_COMBO_BOX (combo), [configure](GtkComboBox *b) {
        combo_changed (b, configure);
    });
  }

  /* which unit to use when displaying the frequency */
  {
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, false, false, 0);

    label = gtk_label_new_with_mnemonic (_("Unit:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, false, false, 0);
    gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
    gtk_label_set_xalign (GTK_LABEL (label), 0);
    gtk_size_group_add_widget (sg0, label);

    GtkWidget *combo = configure->combo_unit = gtk_combo_box_text_new ();
    gtk_box_pack_start (GTK_BOX (hbox), combo, false, true, 0);
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

    xfce4::connect_changed (GTK_COMBO_BOX (combo), [configure](GtkComboBox *b) {
        combo_changed (b, configure);
    });
  }

  /* check buttons for display widgets in panel */
  button = configure->keep_compact = gtk_check_button_new_with_mnemonic (_("_Keep compact"));
  gtk_box_pack_start (GTK_BOX (vbox), button, false, false, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), options->keep_compact);
  xfce4::connect_toggled (GTK_TOGGLE_BUTTON (button), [configure](GtkToggleButton *b) {
      check_button_changed (GTK_WIDGET (b), configure);
  });

  button = configure->one_line = gtk_check_button_new_with_mnemonic (_("Show text in a single _line"));
  gtk_box_pack_start (GTK_BOX (vbox), button, false, false, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), options->one_line);
  xfce4::connect_toggled (GTK_TOGGLE_BUTTON (button), [configure](GtkToggleButton *b) {
      check_button_changed (GTK_WIDGET (b), configure);
  });

  button = configure->display_icon = gtk_check_button_new_with_mnemonic (_("Show CPU _icon"));
  gtk_box_pack_start (GTK_BOX (vbox), button, false, false, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), options->show_icon);
  xfce4::connect_toggled (GTK_TOGGLE_BUTTON (button), [configure](GtkToggleButton *b) {
      check_button_changed (GTK_WIDGET (b), configure);
  });

  button = configure->icon_color_freq = gtk_check_button_new_with_mnemonic (_("Adjust CPU icon color according to frequency"));
  gtk_box_pack_start (GTK_BOX (vbox), button, false, false, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), options->icon_color_freq);
  xfce4::connect_toggled (GTK_TOGGLE_BUTTON (button), [configure](GtkToggleButton *b) {
      check_button_changed (GTK_WIDGET (b), configure);
  });

  button = configure->display_freq = gtk_check_button_new_with_mnemonic (_("Show CPU fre_quency"));
  gtk_box_pack_start (GTK_BOX (vbox), button, false, false, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), options->show_label_freq);
  xfce4::connect_toggled (GTK_TOGGLE_BUTTON (button), [configure](GtkToggleButton *b) {
      check_button_changed (GTK_WIDGET (b), configure);
  });

  button = configure->display_governor = gtk_check_button_new_with_mnemonic (_("Show CPU _governor"));
  gtk_box_pack_start (GTK_BOX (vbox), button, false, false, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), options->show_label_governor);
  xfce4::connect_toggled (GTK_TOGGLE_BUTTON (button), [configure](GtkToggleButton *b) {
      check_button_changed (GTK_WIDGET (b), configure);
  });

  xfce4::connect_response (GTK_DIALOG (dialog), [](GtkDialog *widget, gint response) {
      cpufreq_configure_response (widget);
  });

  update_sensitivity (configure);
  validate_configuration (configure);

  g_object_unref (sg0);
  gtk_widget_show_all (dialog);
}
