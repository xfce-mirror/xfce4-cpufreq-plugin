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

#include <libxfce4ui/libxfce4ui.h>
#include <string.h>

#include "xfce4-cpufreq-plugin.h"
#include "xfce4-cpufreq-configure.h"
#include "xfce4-cpufreq-overview.h"
#include "xfce4-cpufreq-utils.h"

#ifdef __linux__
#include "xfce4-cpufreq-linux.h"
#endif /* __linux__ */

CpuFreqPlugin *cpuFreq = NULL;

static void
cpufreq_label_set_font (void)
{
  gchar *css = NULL, *css_font = NULL, *css_color = NULL;
  GtkWidget *label;

  if (G_UNLIKELY (cpuFreq->label_orNull == NULL))
    return;

  label = cpuFreq->label_orNull;

  if (cpuFreq->options->fontname)
  {
    PangoFontDescription *font;

    font = pango_font_description_from_string(cpuFreq->options->fontname);

    css_font = g_strdup_printf("font-family: %s; font-size: %dpt; font-style: %s; font-weight: %s;",
      pango_font_description_get_family (font),
      pango_font_description_get_size (font) / PANGO_SCALE,
      (pango_font_description_get_style (font) == PANGO_STYLE_ITALIC ||
      pango_font_description_get_style (font) == PANGO_STYLE_OBLIQUE) ? "italic" : "normal",
      (pango_font_description_get_weight (font) >= PANGO_WEIGHT_BOLD) ? "bold" : "normal");

    pango_font_description_free (font);
  }

  if (cpuFreq->options->fontcolor)
    css_color = g_strdup_printf ("color: %s;", cpuFreq->options->fontcolor);

  if (css_font && css_color)
    css = g_strdup_printf ("label { %s %s }", css_font, css_color);
  else if (css_font)
    css = g_strdup_printf ("label { %s }", css_font);
  else if (css_color)
    css = g_strdup_printf ("label { %s }", css_color);

  if (css)
  {
    GtkCssProvider *provider;

    if (cpuFreq->label_css_provider)
    {
      gtk_style_context_remove_provider (
        GTK_STYLE_CONTEXT (gtk_widget_get_style_context (label)),
        GTK_STYLE_PROVIDER (cpuFreq->label_css_provider));
      cpuFreq->label_css_provider = NULL;
    }

    provider = gtk_css_provider_new ();

    gtk_css_provider_load_from_data (provider, css, -1, NULL);
    gtk_style_context_add_provider (
      GTK_STYLE_CONTEXT (gtk_widget_get_style_context (label)),
      GTK_STYLE_PROVIDER (provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    cpuFreq->label_css_provider = provider;
  }

  g_free (css);
  g_free (css_font);
  g_free (css_color);
}



/*
 * Returns a single string describing governors of all CPUs, or NULL.
 * The returned string should be freed with g_free().
 */
static gchar*
cpufreq_governors (void)
{
  const gchar *array[cpuFreq->cpus->len];
  guint count = 0;

  if (cpuFreq->cpus->len == 0)
    return NULL;

  for (guint i = 0; i < cpuFreq->cpus->len; i++)
  {
    CpuInfo *cpu = g_ptr_array_index (cpuFreq->cpus, i);
    guint j;

    if (!cpu->online)
      continue;

    if (!cpu->cur_governor || cpu->cur_governor[0] == '\0')
      continue;

    for (j = 0; j < count; j++)
      if (strcmp (cpu->cur_governor, array[j]) == 0)
        break;
    if (j == count)
      array[count++] = cpu->cur_governor;
  }

  if (count != 0)
  {
    gchar *s;
    gsize s_length;

    // Bubble sort
    for (guint i = 0; G_UNLIKELY (i < count-1); i++)
      for (guint j = i+1; j < count; j++)
        if (strcmp (array[i], array[j]) > 0)
        {
          const gchar *tmp = array[i];
          array[i] = array[j];
          array[j] = tmp;
        }

    s_length = (count-1) * strlen (",");
    for (guint i = 0; i < count; i++)
      s_length += strlen (array[i]);

    s = g_malloc (s_length+1);

    s_length = 0;
    for (guint i = 0; i < count; i++)
    {
      if (i)
        s[s_length++] = ',';
      strcpy (s + s_length, array[i]);
      s_length += strlen (array[i]);
    }
    s[s_length] = '\0';

    return s;
  }
  else
  {
    return NULL;
  }
}



static CpuInfo *
cpufreq_cpus_calc_min (void)
{
  gchar *const governors = cpufreq_governors ();
  gchar *const old_governor = cpuFreq->cpu_min ? g_strdup (cpuFreq->cpu_min->cur_governor) : g_strdup ("");
  guint freq = G_MAXUINT, max_freq = G_MAXUINT, min_freq = G_MAXUINT;
  guint count = 0;

  for (guint i = 0; i < cpuFreq->cpus->len; i++)
  {
    CpuInfo *cpu = g_ptr_array_index (cpuFreq->cpus, i);

    if (!cpu->online)
      continue;

    freq = MIN (freq, cpu->cur_freq);
    max_freq = MIN (max_freq, cpu->max_freq);
    min_freq = MIN (min_freq, cpu->min_freq);
    count++;
  }

  if (count == 0)
    freq = max_freq = min_freq = 0;

  cpuinfo_free (cpuFreq->cpu_min);
  cpuFreq->cpu_min = g_new0 (CpuInfo, 1);
  cpuFreq->cpu_min->cur_freq = freq;
  cpuFreq->cpu_min->cur_governor = governors ? governors : g_strdup (_("current min"));
  cpuFreq->cpu_min->max_freq = max_freq;
  cpuFreq->cpu_min->min_freq = min_freq;

  if (cpuFreq->options->show_label_governor && strcmp(cpuFreq->cpu_min->cur_governor, old_governor) != 0)
  {
    cpuFreq->layout_changed = TRUE;
    cpuFreq->label_max_width = -1;
  }

  g_free (old_governor);
  return cpuFreq->cpu_min;
}



static CpuInfo *
cpufreq_cpus_calc_avg (void)
{
  gchar *const governors = cpufreq_governors ();
  gchar *const old_governor = cpuFreq->cpu_avg ? g_strdup (cpuFreq->cpu_avg->cur_governor) : g_strdup ("");
  guint freq = 0, max_freq = 0, min_freq = 0;
  guint count = 0;

  for (guint i = 0; i < cpuFreq->cpus->len; i++)
  {
    CpuInfo *cpu = g_ptr_array_index (cpuFreq->cpus, i);

    if (!cpu->online)
      continue;

    freq += cpu->cur_freq;
    max_freq += cpu->max_freq;
    min_freq += cpu->min_freq;
    count++;
  }

  if (count != 0)
  {
    freq /= count;
    max_freq /= count;
    min_freq /= count;
  }

  cpuinfo_free (cpuFreq->cpu_avg);
  cpuFreq->cpu_avg = g_new0 (CpuInfo, 1);
  cpuFreq->cpu_avg->cur_freq = freq;
  cpuFreq->cpu_avg->cur_governor = governors ? governors : g_strdup (_("current avg"));
  cpuFreq->cpu_avg->max_freq = max_freq;
  cpuFreq->cpu_avg->min_freq = min_freq;

  if (cpuFreq->options->show_label_governor && strcmp(cpuFreq->cpu_avg->cur_governor, old_governor) != 0)
  {
    cpuFreq->layout_changed = TRUE;
    cpuFreq->label_max_width = -1;
  }

  g_free (old_governor);
  return cpuFreq->cpu_avg;
}



static CpuInfo *
cpufreq_cpus_calc_max (void)
{
  gchar *const governors = cpufreq_governors ();
  gchar *const old_governor = cpuFreq->cpu_max ? g_strdup (cpuFreq->cpu_max->cur_governor) : g_strdup ("");
  guint freq = 0, max_freq = 0, min_freq = 0;

  for (guint i = 0; i < cpuFreq->cpus->len; i++)
  {
    CpuInfo *cpu = g_ptr_array_index (cpuFreq->cpus, i);

    if (!cpu->online)
      continue;

    freq = MAX (freq, cpu->cur_freq);
    max_freq = MAX (max_freq, cpu->max_freq);
    min_freq = MAX (min_freq, cpu->min_freq);
  }

  cpuinfo_free (cpuFreq->cpu_max);
  cpuFreq->cpu_max = g_new0 (CpuInfo, 1);
  cpuFreq->cpu_max->cur_freq = freq;
  cpuFreq->cpu_max->cur_governor = governors ? governors : g_strdup (_("current max"));
  cpuFreq->cpu_max->max_freq = max_freq;
  cpuFreq->cpu_max->min_freq = min_freq;

  if (cpuFreq->options->show_label_governor && strcmp(cpuFreq->cpu_max->cur_governor, old_governor) != 0)
  {
    cpuFreq->layout_changed = TRUE;
    cpuFreq->label_max_width = -1;
  }

  g_free (old_governor);
  return cpuFreq->cpu_max;
}



static gboolean
cpufreq_update_label (CpuInfo *cpu)
{
  GtkWidget *label_widget;
  GtkRequisition label_size;
  gchar *label, *freq;
  gint both;

  if (!cpuFreq->label_orNull)
    return TRUE;

  label_widget = cpuFreq->label_orNull;

  if (!cpuFreq->options->show_label_governor && !cpuFreq->options->show_label_freq) {
    gtk_widget_hide (label_widget);
    return TRUE;
  }

  both = cpu->cur_governor != NULL &&
    cpuFreq->options->show_label_freq &&
    cpuFreq->options->show_label_governor;

  freq = cpufreq_get_human_readable_freq (cpu->cur_freq, cpuFreq->options->unit);
  label = g_strconcat
    (cpuFreq->options->show_label_freq ? freq : "",
     both ? (cpuFreq->options->one_line ? " " : "\n") : "",
     cpu->cur_governor != NULL &&
     cpuFreq->options->show_label_governor ? cpu->cur_governor : "",
     NULL);

  gtk_label_set_text (GTK_LABEL (label_widget), label);

  if (strcmp(label, ""))
  {
    if (cpuFreq->panel_mode == XFCE_PANEL_PLUGIN_MODE_VERTICAL)
      gtk_label_set_angle (GTK_LABEL(label_widget), -90);
    else
      gtk_label_set_angle (GTK_LABEL(label_widget), 0);

    gtk_widget_show (label_widget);

    /* Set label width to max width if smaller to avoid panel
       resizing/jumping (see bug #10385). */
    gtk_widget_get_preferred_size (label_widget, NULL, &label_size);
    if (cpuFreq->panel_mode == XFCE_PANEL_PLUGIN_MODE_VERTICAL)
    {
      if (label_size.height < cpuFreq->label_max_width)
      {
        gtk_widget_set_size_request (label_widget, -1, cpuFreq->label_max_width);
      }
      else if (label_size.height > cpuFreq->label_max_width)
      {
        cpuFreq->label_max_width = label_size.height;
        cpuFreq->layout_changed = TRUE;
      }
    }
    else
    {
      if (label_size.width < cpuFreq->label_max_width)
      {
        gtk_widget_set_size_request (label_widget, cpuFreq->label_max_width, -1);
      }
      else if (label_size.width > cpuFreq->label_max_width)
      {
        cpuFreq->label_max_width = label_size.width;
        cpuFreq->layout_changed = TRUE;
      }
    }
  }
  else
  {
    gtk_widget_hide (label_widget);
  }

  g_free (freq);
  g_free (label);

  return TRUE;
}



static void
cpufreq_widgets_layout (void)
{
  GtkRequisition icon_size, label_size;
  GtkOrientation orientation = GTK_ORIENTATION_HORIZONTAL;
  gboolean small = cpuFreq->options->keep_compact;
  gboolean resized = FALSE;
  gboolean hide_label = (!cpuFreq->options->show_label_freq &&
    !cpuFreq->options->show_label_governor);
  gint pos = 1, lw = 0, lh = 0, iw = 0, ih = 0;

  /* keep plugin small if label is hidden or user requested compact size */
  small = (hide_label ? TRUE : cpuFreq->options->keep_compact);

  switch (cpuFreq->panel_mode)
  {
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
    xfce_panel_plugin_set_small (cpuFreq->plugin, hide_label ? TRUE : FALSE);
    break;
  }

  /* check if the label fits below the icon, else put them side by side */
  if (cpuFreq->label_orNull && ! hide_label)
  {
    gtk_widget_get_preferred_size (cpuFreq->label_orNull, NULL, &label_size);
    lw = label_size.width;
    lh = label_size.height;
  }
  if (GTK_IS_WIDGET(cpuFreq->icon))
  {
    gtk_widget_get_preferred_size (cpuFreq->icon, NULL, &icon_size);
    iw = icon_size.width;
    ih = icon_size.height;
  }

  if (cpuFreq->panel_mode == XFCE_PANEL_PLUGIN_MODE_HORIZONTAL &&
    orientation == GTK_ORIENTATION_VERTICAL &&
    lh + ih + BORDER * 2 >= cpuFreq->panel_size)
  {
    orientation = GTK_ORIENTATION_HORIZONTAL;
    resized = TRUE;
  }
  else if (orientation == GTK_ORIENTATION_HORIZONTAL &&
    lw + iw + BORDER * 2 >= cpuFreq->panel_size &&
    (cpuFreq->panel_mode == XFCE_PANEL_PLUGIN_MODE_DESKBAR || !small))
  {
    orientation = GTK_ORIENTATION_VERTICAL;
    resized = TRUE;
  }

  gtk_orientable_set_orientation (GTK_ORIENTABLE (cpuFreq->box), orientation);

  if (small)
  {
    if (orientation == GTK_ORIENTATION_VERTICAL)
  {
      if (cpuFreq->icon)
        gtk_widget_set_halign (cpuFreq->icon, GTK_ALIGN_CENTER);
      if (cpuFreq->label_orNull)
        gtk_widget_set_halign (cpuFreq->label_orNull, GTK_ALIGN_CENTER);
    }
    else
    {
      if (cpuFreq->icon)
        gtk_widget_set_valign (cpuFreq->icon, GTK_ALIGN_CENTER);
      if (cpuFreq->label_orNull)
        gtk_widget_set_valign (cpuFreq->label_orNull, GTK_ALIGN_CENTER);
    }

    if (cpuFreq->label_orNull)
      gtk_label_set_justify (GTK_LABEL (cpuFreq->label_orNull),
        resized ? GTK_JUSTIFY_CENTER : GTK_JUSTIFY_LEFT);

    if (cpuFreq->icon)
      gtk_box_set_child_packing (GTK_BOX (cpuFreq->box),
        cpuFreq->icon, FALSE, FALSE, 0, GTK_PACK_START);
  }
  else
  {
    if (orientation == GTK_ORIENTATION_VERTICAL)
    {
      if (cpuFreq->icon)
      {
        gtk_widget_set_halign (cpuFreq->icon, GTK_ALIGN_CENTER);
        gtk_widget_set_valign (cpuFreq->icon, GTK_ALIGN_END);
      }

      if (cpuFreq->label_orNull)
        gtk_widget_set_halign (cpuFreq->label_orNull, GTK_ALIGN_CENTER);
    }
    else
    {
      if (cpuFreq->icon)
        gtk_widget_set_valign (cpuFreq->icon, GTK_ALIGN_CENTER);

      if (cpuFreq->label_orNull)
      {
        gtk_widget_set_halign (cpuFreq->label_orNull, GTK_ALIGN_END);
        gtk_widget_set_valign (cpuFreq->label_orNull, GTK_ALIGN_CENTER);
      }
      pos = resized ? 1 : 0;
    }

    if (cpuFreq->label_orNull)
      gtk_label_set_justify (GTK_LABEL (cpuFreq->label_orNull),
        resized ? GTK_JUSTIFY_LEFT : GTK_JUSTIFY_CENTER);

    if (cpuFreq->icon)
      gtk_box_set_child_packing (GTK_BOX (cpuFreq->box),
        cpuFreq->icon, TRUE, TRUE, 0, GTK_PACK_START);
  }

  if (cpuFreq->label_orNull)
    gtk_box_reorder_child (GTK_BOX (cpuFreq->box), cpuFreq->label_orNull, pos);

  cpuFreq->layout_changed = FALSE;
}



static CpuInfo *
cpufreq_current_cpu (void)
{
  CpuInfo *cpu = NULL;

  if (G_UNLIKELY (cpuFreq->options->show_cpu >= (gint) cpuFreq->cpus->len))
  {
    /* Covered use cases:
     * - The user upgraded the cpufreq plugin to a newer version
     * - The user changed the number of CPU cores or SMT (hyper-threading) in the BIOS
     * - The user moved HDD/SSD to a different machine
     * - The user downgraded the CPU in the machine
     */
    cpuFreq->options->show_cpu = CPU_DEFAULT;
    cpufreq_write_config (cpuFreq->plugin);
    cpufreq_warn_reset ();
  }

  switch (cpuFreq->options->show_cpu)
  {
  case CPU_MIN:
    cpu = cpufreq_cpus_calc_min ();
    break;
  case CPU_AVG:
    cpu = cpufreq_cpus_calc_avg ();
    break;
  case CPU_MAX:
    cpu = cpufreq_cpus_calc_max ();
    break;
  default:
    if (cpuFreq->options->show_cpu >= 0 && cpuFreq->options->show_cpu < (gint) cpuFreq->cpus->len)
      cpu = g_ptr_array_index (cpuFreq->cpus, cpuFreq->options->show_cpu);
  }

  return cpu;
}



gboolean
cpufreq_update_plugin (gboolean reset_label_size)
{
  CpuInfo *cpu;
  gboolean ret;

  cpu = cpufreq_current_cpu ();
  if (!cpu)
    return FALSE;

  if (reset_label_size)
  {
    cpuFreq->label_max_width = -1;
    if (cpuFreq->label_orNull)
      gtk_widget_set_size_request (cpuFreq->label_orNull, -1, -1);
    cpuFreq->layout_changed = TRUE;
  }

  ret = cpufreq_update_label (cpu);

  if (cpuFreq->layout_changed)
  {
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
                        CpuFreqPlugin *_unused)
{
  const CpuFreqPluginOptions *const options = cpuFreq->options;
  CpuInfo *cpu;
  gchar *tooltip_msg;

  cpu = cpufreq_current_cpu ();

  if (G_UNLIKELY (cpu == NULL))
  {
    tooltip_msg = g_strdup (_("No CPU information available."));
  }
  else
  {
    gchar *freq = cpufreq_get_human_readable_freq (cpu->cur_freq, options->unit);
    if (options->show_label_governor && options->show_label_freq)
      tooltip_msg = g_strdup_printf (ngettext ("%d cpu available",
        "%d cpus available", cpuFreq->cpus->len), cpuFreq->cpus->len);
    else
      tooltip_msg =
        g_strconcat
        (!options->show_label_freq ? _("Frequency: ") : "",
         !options->show_label_freq ? freq : "",

         cpu->cur_governor != NULL &&
         !options->show_label_freq &&
         !options->show_label_governor ? "\n" : "",

         cpu->cur_governor != NULL &&
         !options->show_label_governor ? _("Governor: ") : "",
         cpu->cur_governor != NULL &&
         !options->show_label_governor ? cpu->cur_governor : "",
         NULL);
    g_free (freq);
  }

  gtk_tooltip_set_text (tooltip, tooltip_msg);
  g_free (tooltip_msg);
  return TRUE;
}



void
cpufreq_restart_timeout (void)
{
#ifdef __linux__
  g_source_remove (cpuFreq->timeoutHandle);
  cpuFreq->timeoutHandle = g_timeout_add_seconds (
    cpuFreq->options->timeout, cpufreq_update_cpus, NULL);
#endif
}



static void
cpufreq_mode_changed (XfcePanelPlugin *plugin,
                      XfcePanelPluginMode mode,
                      CpuFreqPlugin *cpufreq)
{
  cpuFreq->panel_mode = mode;
  cpuFreq->panel_rows = xfce_panel_plugin_get_nrows (plugin);
  cpufreq_update_plugin (TRUE);
}



void
cpufreq_update_icon (CpuFreqPlugin *cpufreq)
{
  if (cpufreq->icon)
  {
    gtk_widget_destroy (cpufreq->icon);
    cpufreq->icon = NULL;
  }

  if (cpufreq->options->show_icon)
  {
    GdkPixbuf *buf, *scaled;
    gint icon_size;

    icon_size = cpuFreq->panel_size / cpuFreq->panel_rows;

    if (cpufreq->options->keep_compact ||
      (!cpufreq->options->show_label_freq &&
       !cpufreq->options->show_label_governor))
      icon_size -= 4;

    buf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
      "xfce4-cpufreq-plugin", icon_size, 0, NULL);

    if (buf)
    {
      scaled = gdk_pixbuf_scale_simple (buf, icon_size, icon_size, GDK_INTERP_BILINEAR);
      cpufreq->icon = gtk_image_new_from_pixbuf (scaled);
      g_object_unref (G_OBJECT (buf));
      g_object_unref (G_OBJECT (scaled));
    }
    else
    {
      cpufreq->icon = gtk_image_new_from_icon_name ("xfce4-cpufreq-plugin", GTK_ICON_SIZE_BUTTON);
    }

    gtk_box_pack_start (GTK_BOX (cpufreq->box), cpufreq->icon, FALSE, FALSE, 0);
    gtk_widget_show (cpufreq->icon);
  }
}



void
cpufreq_prepare_label (CpuFreqPlugin *cpufreq)
{
  if (cpufreq->label_orNull)
  {
    if (cpuFreq->label_css_provider)
    {
      gtk_style_context_remove_provider (
        GTK_STYLE_CONTEXT (gtk_widget_get_style_context (cpuFreq->label_orNull)),
        GTK_STYLE_PROVIDER (cpuFreq->label_css_provider));
      cpuFreq->label_css_provider = NULL;
    }

    gtk_widget_destroy (cpufreq->label_orNull);
    cpufreq->label_orNull = NULL;
  }

  if (cpuFreq->options->show_label_freq || cpuFreq->options->show_label_governor)
  {
    cpuFreq->label_orNull = gtk_label_new (NULL);
    gtk_box_pack_start (GTK_BOX (cpufreq->box), cpuFreq->label_orNull, TRUE, TRUE, 0);
  }
}



static void
cpufreq_widgets (void)
{
  gchar *css;
  GtkCssProvider *provider;

  /* create panel toggle button which will contain all other widgets */
  cpuFreq->button = xfce_panel_create_toggle_button ();
  xfce_panel_plugin_add_action_widget (cpuFreq->plugin, cpuFreq->button);
  gtk_container_add (GTK_CONTAINER (cpuFreq->plugin), cpuFreq->button);

  css = g_strdup_printf("button { padding: 0px; }");

  provider = gtk_css_provider_new ();
  gtk_css_provider_load_from_data (provider, css, -1, NULL);
  gtk_style_context_add_provider (
    GTK_STYLE_CONTEXT (gtk_widget_get_style_context (cpuFreq->button)),
    GTK_STYLE_PROVIDER(provider),
    GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_free(css);

  cpuFreq->box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, SPACING);
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

  gtk_widget_show_all (cpuFreq->button);

  cpufreq_update_plugin (TRUE);
}



static void
cpufreq_read_config (void)
{
  CpuFreqPluginOptions *const options = cpuFreq->options;
  XfceRc *rc;
  gchar  *file;
  const gchar *value;

  file = xfce_panel_plugin_lookup_rc_file (cpuFreq->plugin);

  if (G_UNLIKELY (!file))
    file = xfce_panel_plugin_save_location (cpuFreq->plugin, FALSE);

  if (G_UNLIKELY (!file))
    return;

  rc = xfce_rc_simple_open (file, FALSE);
  g_free (file);

  options->timeout             = xfce_rc_read_int_entry  (rc, "timeout", 1);
  if (options->timeout > TIMEOUT_MAX || options->timeout < TIMEOUT_MIN)
    options->timeout = TIMEOUT_MIN;
  options->show_cpu            = xfce_rc_read_int_entry  (rc, "show_cpu", CPU_DEFAULT);
  options->show_icon           = xfce_rc_read_bool_entry (rc, "show_icon", TRUE);
  options->show_label_freq     = xfce_rc_read_bool_entry (rc, "show_label_freq", TRUE);
  options->show_label_governor = xfce_rc_read_bool_entry (rc, "show_label_governor", TRUE);
  options->show_warning        = xfce_rc_read_bool_entry (rc, "show_warning", TRUE);
  options->keep_compact        = xfce_rc_read_bool_entry (rc, "keep_compact", FALSE);
  options->one_line            = xfce_rc_read_bool_entry (rc, "one_line", FALSE);
  options->unit                = xfce_rc_read_int_entry  (rc, "freq_unit", UNIT_DEFAULT);

  if (!options->show_label_freq && !options->show_label_governor)
    options->show_icon = TRUE;

  switch (options->unit)
  {
  case UNIT_AUTO:
  case UNIT_GHZ:
  case UNIT_MHZ:
    break;
  default:
    options->unit = UNIT_DEFAULT;
  }

  value = xfce_rc_read_entry (rc, "fontname", NULL);
  if (value)
  {
    g_free (options->fontname);
    options->fontname = g_strdup (value);
  }

  value = xfce_rc_read_entry (rc, "fontcolor", NULL);
  if (value)
  {
    g_free (options->fontcolor);
    options->fontcolor = g_strdup (value);
  }

  xfce_rc_close (rc);
}



void
cpufreq_write_config (XfcePanelPlugin *plugin)
{
  const CpuFreqPluginOptions *const options = cpuFreq->options;
  XfceRc *rc;
  gchar  *file;

  file = xfce_panel_plugin_save_location (plugin, TRUE);

  if (G_UNLIKELY (!file))
    return;

  rc = xfce_rc_simple_open (file, FALSE);
  g_free(file);

  xfce_rc_write_int_entry  (rc, "timeout",             options->timeout);
  xfce_rc_write_int_entry  (rc, "show_cpu",            options->show_cpu);
  xfce_rc_write_bool_entry (rc, "show_icon",           options->show_icon);
  xfce_rc_write_bool_entry (rc, "show_label_freq",     options->show_label_freq);
  xfce_rc_write_bool_entry (rc, "show_label_governor", options->show_label_governor);
  xfce_rc_write_bool_entry (rc, "show_warning",        options->show_warning);
  xfce_rc_write_bool_entry (rc, "keep_compact",        options->keep_compact);
  xfce_rc_write_bool_entry (rc, "one_line",            options->one_line);
  xfce_rc_write_int_entry  (rc, "freq_unit",           options->unit);

  if (options->fontname)
    xfce_rc_write_entry (rc, "fontname", options->fontname);
  else
    xfce_rc_delete_entry (rc, "fontname", FALSE);

  if (options->fontcolor)
    xfce_rc_write_entry (rc, "fontcolor", options->fontcolor);
  else
    xfce_rc_delete_entry (rc, "fontcolor", FALSE);

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
  if (cpuFreq->timeoutHandle)
    g_source_remove (cpuFreq->timeoutHandle);

  g_slice_free (IntelPState, cpuFreq->intel_pstate);

  for (guint i = 0; i < cpuFreq->cpus->len; i++)
  {
    CpuInfo *cpu = g_ptr_array_index (cpuFreq->cpus, i);
    g_ptr_array_remove_fast (cpuFreq->cpus, cpu);
    cpuinfo_free (cpu);
  }

  g_ptr_array_free (cpuFreq->cpus, TRUE);

  g_free (cpuFreq->cpu_avg);
  g_free (cpuFreq->cpu_max);
  g_free (cpuFreq->cpu_min);

  g_free (cpuFreq->options->fontname);
  cpuFreq->plugin = NULL;
  g_free (cpuFreq);
}



static gboolean
cpufreq_set_size (XfcePanelPlugin *plugin, gint size, CpuFreqPlugin *cpufreq)
{
  cpuFreq->panel_size = size;
  cpuFreq->panel_rows = xfce_panel_plugin_get_nrows (plugin);

  cpufreq_update_icon (cpufreq);
  cpufreq_update_plugin (TRUE);

  return TRUE;
}

static void
cpufreq_show_about(XfcePanelPlugin *plugin, CpuFreqPlugin *cpufreq)
{
  const gchar *auth[] = {
    "Thomas Schreck <shrek@xfce.org>",
    "Florian Rivoal <frivoal@xfce.org>",
    "Harald Judt <h.judt@gmx.at>",
    "Andre Miranda <andreldm@xfce.org>",
    NULL };

  GdkPixbuf *icon = xfce_panel_pixbuf_from_source ("xfce4-cpufreq-plugin", NULL, 48);

  gtk_show_about_dialog
    (NULL,
     "logo", icon,
     "license", xfce_get_license_text (XFCE_LICENSE_TEXT_GPL),
     "version", PACKAGE_VERSION,
     "program-name", PACKAGE_NAME,
     "comments", _("Show CPU frequencies and governor"),
     "website", PLUGIN_WEBSITE,
     "copyright", _("Copyright (c) 2003-2021\n"),
     "authors", auth,
     NULL);

  if (icon)
    g_object_unref(G_OBJECT(icon));
}

static void
cpufreq_construct (XfcePanelPlugin *plugin)
{
  xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

  cpuFreq = g_new0 (CpuFreqPlugin, 1);
  cpuFreq->options = g_new0 (CpuFreqPluginOptions, 1);
  cpuFreq->plugin = plugin;
  cpuFreq->panel_mode = xfce_panel_plugin_get_mode (cpuFreq->plugin);
  cpuFreq->panel_rows = xfce_panel_plugin_get_nrows (cpuFreq->plugin);
  cpuFreq->panel_size = xfce_panel_plugin_get_size (cpuFreq->plugin);
  cpuFreq->label_max_width = -1;
  cpuFreq->cpus = g_ptr_array_new ();

  cpufreq_read_config ();
  cpuFreq->layout_changed = TRUE;

#ifdef __linux__
  if (!cpufreq_linux_init ())
    xfce_dialog_show_error (NULL, NULL,
      _("Your system is not configured correctly to support CPU frequency scaling!"));

  gtk_widget_set_size_request (GTK_WIDGET (plugin), -1, -1);
  cpufreq_widgets ();

  cpuFreq->timeoutHandle = g_timeout_add_seconds (
    cpuFreq->options->timeout, cpufreq_update_cpus, NULL);
#else
  xfce_dialog_show_error (NULL, NULL, _("Your system is not supported yet!"));
#endif /* __linux__ */

  g_signal_connect (plugin, "free-data", G_CALLBACK (cpufreq_free), NULL);
  g_signal_connect (plugin, "save", G_CALLBACK (cpufreq_write_config), NULL);
  g_signal_connect (plugin, "size-changed", G_CALLBACK (cpufreq_set_size), cpuFreq);
  g_signal_connect (plugin, "mode-changed", G_CALLBACK (cpufreq_mode_changed), cpuFreq);

  /* the configure and about menu items are hidden by default */
  xfce_panel_plugin_menu_show_configure (plugin);
  g_signal_connect (plugin, "configure-plugin", G_CALLBACK (cpufreq_configure), NULL);
  xfce_panel_plugin_menu_show_about(plugin);
  g_signal_connect (G_OBJECT (plugin), "about", G_CALLBACK (cpufreq_show_about), cpuFreq);
}

XFCE_PANEL_PLUGIN_REGISTER (cpufreq_construct);
