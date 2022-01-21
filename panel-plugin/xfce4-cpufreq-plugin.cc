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

#define SPACING           2  /* Space between the widgets */
#define BORDER            1  /* Space between the frame and the widgets */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libxfce4ui/libxfce4ui.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "plugin.h"
#include "xfce4-cpufreq-plugin.h"
#include "xfce4-cpufreq-configure.h"
#include "xfce4-cpufreq-overview.h"
#include "xfce4-cpufreq-utils.h"

#ifdef __linux__
#include "xfce4-cpufreq-linux.h"
#endif /* __linux__ */

CpuFreqPlugin *cpuFreq = NULL;



/*
 * Returns a single string describing governors of all CPUs, or NULL.
 * The returned string should be freed with g_free().
 */
static gchar*
cpufreq_governors ()
{
  const gchar *array[cpuFreq->cpus->len];
  guint count = 0;

  if (cpuFreq->cpus->len == 0)
    return NULL;

  for (guint i = 0; i < cpuFreq->cpus->len; i++)
  {
    auto cpu = (CpuInfo*) g_ptr_array_index (cpuFreq->cpus, i);

    if (!cpu->online)
      continue;

    if (!cpu->cur_governor || cpu->cur_governor[0] == '\0')
      continue;

    guint j;
    for (j = 0; j < count; j++)
      if (strcmp (cpu->cur_governor, array[j]) == 0)
        break;
    if (j == count)
      array[count++] = cpu->cur_governor;
  }

  if (count != 0)
  {
    // Bubble sort
    for (guint i = 0; G_UNLIKELY (i < count-1); i++)
      for (guint j = i+1; j < count; j++)
        if (strcmp (array[i], array[j]) > 0)
        {
          const gchar *tmp = array[i];
          array[i] = array[j];
          array[j] = tmp;
        }

    gsize s_length = (count-1) * strlen (",");
    for (guint i = 0; i < count; i++)
      s_length += strlen (array[i]);

    auto s = (gchar*) g_malloc (s_length+1);

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
cpufreq_cpus_calc_min ()
{
  gchar *const governors = cpufreq_governors ();
  gchar *const old_governor = cpuFreq->cpu_min ? g_strdup (cpuFreq->cpu_min->cur_governor) : g_strdup ("");
  guint freq = G_MAXUINT, max_freq_measured = G_MAXUINT, max_freq_nominal = G_MAXUINT, min_freq = G_MAXUINT;
  guint count = 0;

  for (guint i = 0; i < cpuFreq->cpus->len; i++)
  {
    auto cpu = (const CpuInfo*) g_ptr_array_index (cpuFreq->cpus, i);

    if (!cpu->online)
      continue;

    freq = MIN (freq, cpu->cur_freq);
    max_freq_measured = MIN (max_freq_measured, cpu->max_freq_measured);
    max_freq_nominal = MIN (max_freq_nominal, cpu->max_freq_nominal);
    min_freq = MIN (min_freq, cpu->min_freq);
    count++;
  }

  if (count == 0)
    freq = max_freq_measured = max_freq_nominal = min_freq = 0;

  cpuinfo_free (cpuFreq->cpu_min);
  cpuFreq->cpu_min = g_new0 (CpuInfo, 1);
  cpuFreq->cpu_min->cur_freq = freq;
  cpuFreq->cpu_min->cur_governor = governors ? governors : g_strdup (_("current min"));
  cpuFreq->cpu_min->max_freq_measured = max_freq_measured;
  cpuFreq->cpu_min->max_freq_nominal = max_freq_nominal;
  cpuFreq->cpu_min->min_freq = min_freq;

  if (cpuFreq->options->show_label_governor && strcmp(cpuFreq->cpu_min->cur_governor, old_governor) != 0)
  {
    cpuFreq->label.reset_size = TRUE;
    cpuFreq->layout_changed = TRUE;
  }

  g_free (old_governor);
  return cpuFreq->cpu_min;
}



static CpuInfo *
cpufreq_cpus_calc_avg ()
{
  gchar *const governors = cpufreq_governors ();
  gchar *const old_governor = cpuFreq->cpu_avg ? g_strdup (cpuFreq->cpu_avg->cur_governor) : g_strdup ("");
  guint freq = 0, max_freq_measured = 0, max_freq_nominal = 0, min_freq = 0;
  guint count = 0;

  for (guint i = 0; i < cpuFreq->cpus->len; i++)
  {
    auto cpu = (const CpuInfo*) g_ptr_array_index (cpuFreq->cpus, i);

    if (!cpu->online)
      continue;

    freq += cpu->cur_freq;
    max_freq_measured += cpu->max_freq_measured;
    max_freq_nominal += cpu->max_freq_nominal;
    min_freq += cpu->min_freq;
    count++;
  }

  if (count != 0)
  {
    freq /= count;
    max_freq_measured /= count;
    max_freq_nominal /= count;
    min_freq /= count;
  }

  cpuinfo_free (cpuFreq->cpu_avg);
  cpuFreq->cpu_avg = g_new0 (CpuInfo, 1);
  cpuFreq->cpu_avg->cur_freq = freq;
  cpuFreq->cpu_avg->cur_governor = governors ? governors : g_strdup (_("current avg"));
  cpuFreq->cpu_avg->max_freq_measured = max_freq_measured;
  cpuFreq->cpu_avg->max_freq_nominal = max_freq_nominal;
  cpuFreq->cpu_avg->min_freq = min_freq;

  if (cpuFreq->options->show_label_governor && strcmp(cpuFreq->cpu_avg->cur_governor, old_governor) != 0)
  {
    cpuFreq->label.reset_size = TRUE;
    cpuFreq->layout_changed = TRUE;
  }

  g_free (old_governor);
  return cpuFreq->cpu_avg;
}



static CpuInfo *
cpufreq_cpus_calc_max ()
{
  gchar *const governors = cpufreq_governors ();
  gchar *const old_governor = cpuFreq->cpu_max ? g_strdup (cpuFreq->cpu_max->cur_governor) : g_strdup ("");
  guint freq = 0, max_freq_measured = 0, max_freq_nominal = 0, min_freq = 0;

  for (guint i = 0; i < cpuFreq->cpus->len; i++)
  {
    auto cpu = (const CpuInfo*) g_ptr_array_index (cpuFreq->cpus, i);

    if (!cpu->online)
      continue;

    freq = MAX (freq, cpu->cur_freq);
    max_freq_measured = MAX (max_freq_measured, cpu->max_freq_measured);
    max_freq_nominal = MAX (max_freq_nominal, cpu->max_freq_nominal);
    min_freq = MAX (min_freq, cpu->min_freq);
  }

  cpuinfo_free (cpuFreq->cpu_max);
  cpuFreq->cpu_max = g_new0 (CpuInfo, 1);
  cpuFreq->cpu_max->cur_freq = freq;
  cpuFreq->cpu_max->cur_governor = governors ? governors : g_strdup (_("current max"));
  cpuFreq->cpu_max->max_freq_measured = max_freq_measured;
  cpuFreq->cpu_max->max_freq_nominal = max_freq_nominal;
  cpuFreq->cpu_max->min_freq = min_freq;

  if (cpuFreq->options->show_label_governor && strcmp(cpuFreq->cpu_max->cur_governor, old_governor) != 0)
  {
    cpuFreq->label.reset_size = TRUE;
    cpuFreq->layout_changed = TRUE;
  }

  g_free (old_governor);
  return cpuFreq->cpu_max;
}



static void
cpufreq_update_label (const CpuInfo *cpu)
{
  const CpuFreqPluginOptions *const options = cpuFreq->options;

  if (!cpuFreq->label.draw_area)
    return;

  GtkWidget *label_widget = cpuFreq->label.draw_area;

  if (!options->show_label_governor && !options->show_label_freq)
  {
    gtk_widget_hide (label_widget);
    return;
  }

  gboolean both = cpu->cur_governor && options->show_label_freq && options->show_label_governor;

  gchar *freq = cpufreq_get_human_readable_freq (cpu->cur_freq, options->unit);
  gchar *label = g_strconcat
    (options->show_label_freq ? freq : "",
     both ? (options->one_line ? " " : "\n") : "",
     cpu->cur_governor != NULL &&
     options->show_label_governor ? cpu->cur_governor : "",
     NULL);

  if (*label != '\0')
  {
    if (!gtk_widget_is_visible (label_widget))
      gtk_widget_show (label_widget);

    if (!cpuFreq->label.text || strcmp (cpuFreq->label.text, label) != 0)
    {
      g_free (cpuFreq->label.text);
      cpuFreq->label.text = g_strdup (label);
      gtk_widget_queue_draw (label_widget);
    }
  }
  else
  {
    if (cpuFreq->label.text)
    {
      g_free (cpuFreq->label.text);
      cpuFreq->label.text = NULL;
    }
    gtk_widget_hide (label_widget);
  }

  g_free (freq);
  g_free (label);
}



static void
cpufreq_widgets_layout ()
{
  GtkOrientation orientation = GTK_ORIENTATION_HORIZONTAL;
  gboolean resized = FALSE;
  const gboolean hide_label = (!cpuFreq->options->show_label_freq && !cpuFreq->options->show_label_governor);
  gint pos = 1, lw = 0, lh = 0, iw = 0, ih = 0;

  /* keep plugin small if label is hidden or user requested compact size */
  const gboolean small = (hide_label ? TRUE : cpuFreq->options->keep_compact);

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
  if (cpuFreq->label.draw_area && !hide_label)
  {
    GtkRequisition label_size;
    gtk_widget_get_preferred_size (cpuFreq->label.draw_area, NULL, &label_size);
    lw = label_size.width;
    lh = label_size.height;
  }
  if (GTK_IS_WIDGET(cpuFreq->icon))
  {
    GtkRequisition icon_size;
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
    }
    else
    {
      if (cpuFreq->icon)
        gtk_widget_set_valign (cpuFreq->icon, GTK_ALIGN_CENTER);
    }

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
    }
    else
    {
      if (cpuFreq->icon)
        gtk_widget_set_valign (cpuFreq->icon, GTK_ALIGN_CENTER);

      pos = resized ? 1 : 0;
    }

    if (cpuFreq->icon)
      gtk_box_set_child_packing (GTK_BOX (cpuFreq->box),
        cpuFreq->icon, TRUE, TRUE, 0, GTK_PACK_START);
  }

  if (cpuFreq->label.draw_area)
  {
    gtk_box_reorder_child (GTK_BOX (cpuFreq->box), cpuFreq->label.draw_area, pos);
    gtk_widget_queue_draw (cpuFreq->label.draw_area);
  }

  cpuFreq->layout_changed = FALSE;
}



static CpuInfo *
cpufreq_current_cpu ()
{
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

  CpuInfo *cpu = NULL;
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
      cpu = (CpuInfo*) g_ptr_array_index (cpuFreq->cpus, cpuFreq->options->show_cpu);
  }

  return cpu;
}



static void
cpufreq_update_pixmap (CpuInfo *cpu)
{
  const gdouble min_range = 100*1000; /* frequency in kHz */

  if (G_UNLIKELY (!cpuFreq->icon || !cpuFreq->base_icon))
    return;

  /* Note:
   *   max_freq_nominal can have values that are outside
   *   of the actual maximum frequency of the CPU.
   *   For example, Linux kernel 5.10.17 reports 5554687 kHz
   *   for some CPU cores on Ryzen 3700X, but the actual top
   *   frequency of this CPU is about 4.4 GHz.
   *   Therefore, the following algorithm relies on measured
   *   frequencies only.
   */

  /* Compute the 99th percentile of all measured frequencies
   * and use it as the "true" maximum frequency. This is required
   * because in a small percentage of cases the current CPU
   * frequency reported by Linux can be unrealistic, for example
   * 4.59 GHz on a CPU that has an actual maximum frequency
   * of about 4.4 GHz.
   */
  gdouble freq_99 = cpu->max_freq_measured;
  gint total_count = 0;
  for (gsize i = 0; i < G_N_ELEMENTS (cpuFreq->freq_hist); i++)
    total_count += cpuFreq->freq_hist[i];
  if (total_count * 0.01 < 1)
  {
    /* Not enough data to reliably compute the percentile,
     * resort to a value that isn't based on statistics */
    freq_99 = MAX (cpu->max_freq_nominal, cpu->max_freq_measured);
  }
  else
  {
    gint percentile_2 = total_count * 0.01;
    for (gssize i = G_N_ELEMENTS (cpuFreq->freq_hist) - 1; i >= 0; i--)
    {
      guint16 count = cpuFreq->freq_hist[i];
      if (count < percentile_2)
        percentile_2 -= count;
      else
      {
        freq_99 = FREQ_HIST_MIN + i * ((gdouble) (FREQ_HIST_MAX - FREQ_HIST_MIN) / FREQ_HIST_BINS);
        break;
      }
    }
  }

  const gdouble range = freq_99 - cpu->min_freq;
  gdouble normalized_freq;
  if (cpu->cur_freq > cpu->min_freq && range >= min_range)
    normalized_freq = (cpu->cur_freq - cpu->min_freq) / range;
  else
    normalized_freq = 0;

  gint index = round (normalized_freq * (G_N_ELEMENTS (cpuFreq->icon_pixmaps) - 1));
  if (G_UNLIKELY (index < 0))
    index = 0;
  if (index >= (gint) G_N_ELEMENTS (cpuFreq->icon_pixmaps))
    /* This codepath is expected to be reached in 100-99=1% of cases */
    index = G_N_ELEMENTS (cpuFreq->icon_pixmaps) - 1;

  GdkPixbuf *pixmap = cpuFreq->icon_pixmaps[index];
  if (!pixmap)
  {
    guchar color = (guchar) (255 * index / (G_N_ELEMENTS (cpuFreq->icon_pixmaps) - 1));

    pixmap = gdk_pixbuf_copy (cpuFreq->base_icon);
    if (G_UNLIKELY (!pixmap))
      return;

    guchar *pixels = gdk_pixbuf_get_pixels (pixmap);
    gsize plength = gdk_pixbuf_get_byte_length (pixmap);
    int n_channels = gdk_pixbuf_get_n_channels (pixmap);
    if (G_UNLIKELY (n_channels <= 0))
    {
      g_object_unref (G_OBJECT (pixmap));
      return;
    }

    for (gsize p = 0; p+2 < plength; p += n_channels)
    {
      gint delta1 = abs ((gint) pixels[p] - (gint) pixels[p+1]);
      gint delta2 = abs ((gint) pixels[p] - (gint) pixels[p+2]);
      if (delta1 < 10 && delta2 < 10)
        pixels[p] = MAX (pixels[p], color);
    }

    cpuFreq->icon_pixmaps[index] = pixmap;
  }

  if (cpuFreq->current_icon_pixmap != pixmap)
  {
    cpuFreq->current_icon_pixmap = pixmap;
    gdk_pixbuf_copy_area (pixmap, 0, 0,
                          gdk_pixbuf_get_width (pixmap),
                          gdk_pixbuf_get_height (pixmap),
                          gtk_image_get_pixbuf (GTK_IMAGE (cpuFreq->icon)),
                          0, 0);
    g_signal_emit_by_name (cpuFreq->icon, "style-updated");
  }
}



gboolean
cpufreq_update_plugin (gboolean reset_label_size)
{
  CpuInfo *cpu = cpufreq_current_cpu ();
  if (!cpu)
    return FALSE;

  if (reset_label_size)
  {
    cpuFreq->label.reset_size = TRUE;
    cpuFreq->layout_changed = TRUE;
  }

  cpufreq_update_label (cpu);

  if (cpuFreq->options->icon_color_freq)
    cpufreq_update_pixmap (cpu);

  if (cpuFreq->layout_changed)
    cpufreq_widgets_layout ();

  return TRUE;
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
  gchar *tooltip_msg;

  CpuInfo *cpu = cpufreq_current_cpu ();

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
cpufreq_restart_timeout ()
{
#ifdef __linux__
  g_source_remove (cpuFreq->timeoutHandle);
  cpuFreq->timeoutHandle = g_timeout_add_seconds (
    cpuFreq->options->timeout, cpufreq_update_cpus, NULL);
#endif
}



void
cpufreq_set_font (const gchar *fontname_or_null)
{
  if (cpuFreq->label.font_desc)
  {
    pango_font_description_free (cpuFreq->label.font_desc);
    cpuFreq->label.font_desc = NULL;
  }

  if (fontname_or_null)
  {
    g_free (cpuFreq->options->fontname);
    cpuFreq->options->fontname = g_strdup (fontname_or_null);
    cpuFreq->label.font_desc = pango_font_description_from_string (fontname_or_null);
  }
  else
  {
    g_free (cpuFreq->options->fontname);
    cpuFreq->options->fontname = NULL;
  }
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



static void
cpufreq_destroy_icons ()
{
  if (cpuFreq->icon)
  {
    gtk_widget_destroy (cpuFreq->icon);
    cpuFreq->icon = NULL;
  }

  if (cpuFreq->base_icon)
  {
    g_object_unref (G_OBJECT (cpuFreq->base_icon));
    cpuFreq->base_icon = NULL;
  }

  for (gsize i = 0; i < G_N_ELEMENTS (cpuFreq->icon_pixmaps); i++)
    if (cpuFreq->icon_pixmaps[i])
    {
      g_object_unref (G_OBJECT (cpuFreq->icon_pixmaps[i]));
      cpuFreq->icon_pixmaps[i] = NULL;
    }

  cpuFreq->current_icon_pixmap = NULL;
}



void
cpufreq_update_icon ()
{
  const CpuFreqPluginOptions *options = cpuFreq->options;

  cpufreq_destroy_icons ();

  /* Load and scale the icon */
  if (options->show_icon)
  {
    gint icon_size = cpuFreq->panel_size / cpuFreq->panel_rows;

    if (options->keep_compact ||
      (!options->show_label_freq &&
       !options->show_label_governor))
      icon_size -= 4;

    GdkPixbuf *buf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
      "xfce4-cpufreq-plugin", icon_size, (GtkIconLookupFlags) 0, NULL);

    if (buf)
    {
      GdkPixbuf *scaled = gdk_pixbuf_scale_simple (buf, icon_size, icon_size, GDK_INTERP_BILINEAR);
      if (G_LIKELY (scaled != NULL))
      {
        g_object_unref (G_OBJECT (buf));
        buf = scaled;
      }
      cpuFreq->icon = gtk_image_new_from_pixbuf (buf);
      cpuFreq->base_icon = gdk_pixbuf_copy (buf);
      g_object_unref (G_OBJECT (buf));
    }
    else
    {
      cpuFreq->icon = gtk_image_new_from_icon_name ("xfce4-cpufreq-plugin", GTK_ICON_SIZE_BUTTON);
      /* At this point: The storage type of the icon isn't GTK_IMAGE_PIXBUF */
    }

    if (G_LIKELY (cpuFreq->icon))
    {
      gtk_box_pack_start (GTK_BOX (cpuFreq->box), cpuFreq->icon, FALSE, FALSE, 0);
      gtk_box_reorder_child (GTK_BOX (cpuFreq->box), cpuFreq->icon, 0);
      gtk_widget_show (cpuFreq->icon);
    }
  }
}



static void
label_draw (GtkWidget *widget, cairo_t *cr, gpointer data)
{
  if (G_UNLIKELY (!cpuFreq->label.text))
    return;

  cairo_save (cr);

  GtkAllocation alloc;
  gtk_widget_get_allocation (widget, &alloc);
  PangoContext *pango_context = gtk_widget_get_pango_context (widget);
  GtkStyleContext *style_context = gtk_widget_get_style_context (widget);

  GdkRGBA color;
  if (cpuFreq->options->fontcolor)
  {
    gdk_rgba_parse (&color, cpuFreq->options->fontcolor);
  }
  else
  {
    gtk_style_context_get_color (style_context,
                                 gtk_style_context_get_state (style_context),
                                 &color);
  }
  gdk_cairo_set_source_rgba (cr, &color);

  PangoLayout *layout = pango_layout_new (pango_context);

  if (cpuFreq->label.font_desc)
    pango_layout_set_font_description (layout, cpuFreq->label.font_desc);

  pango_layout_set_text (layout, cpuFreq->label.text, -1);

  PangoRectangle extents;
  if (cpuFreq->panel_mode != XFCE_PANEL_PLUGIN_MODE_VERTICAL)
  {
    pango_layout_get_extents (layout, NULL, &extents);
    if (alloc.width > PANGO_PIXELS_CEIL (extents.width))
    {
      double x0 = (double) extents.x / PANGO_SCALE;
      double x1 = alloc.width / 2.0 - extents.width / 2.0 / PANGO_SCALE;
      cairo_translate (cr, x1 - x0, 0);
    }
    if (alloc.height < PANGO_PIXELS_CEIL (extents.height))
    {
      double y0 = (double) extents.y / PANGO_SCALE;
      double y1 = alloc.height / 2.0 - extents.height / 2.0 / PANGO_SCALE;
      cairo_translate (cr, 0, y1 - y0);
    }

    /* Set label width to max width if smaller to avoid panel
       resizing/jumping (see bug #10385). */
    cpuFreq->label.reset_size |= alloc.width < PANGO_PIXELS_CEIL (extents.width);
  }
  else
  {
    PangoRectangle non_transformed_extents;

    /* rotate by 90° */
    cairo_rotate (cr, M_PI_2);
    cairo_translate (cr, 0, -alloc.width);
    pango_cairo_update_layout (cr, layout);

    pango_layout_get_extents (layout, NULL, &non_transformed_extents);
    extents.x = non_transformed_extents.y;
    extents.y = non_transformed_extents.x;
    extents.width = non_transformed_extents.height;
    extents.height = non_transformed_extents.width;
    if (alloc.width < PANGO_PIXELS_CEIL (extents.width))
    {
      double x0 = (double) extents.x / PANGO_SCALE;
      double x1 = alloc.width / 2.0 - extents.width / 2.0 / PANGO_SCALE;
      /* cairo_translate: X and Y are swapped because of the rotation by 90° */
      cairo_translate (cr, 0, x1 - x0);
    }
    if (alloc.height > PANGO_PIXELS_CEIL (extents.height))
    {
      double y0 = (double) extents.y / PANGO_SCALE;
      double y1 = alloc.height / 2.0 - extents.height / 2.0 / PANGO_SCALE;
      /* cairo_translate: X and Y are swapped because of the rotation by 90° */
      cairo_translate (cr, y1 - y0, 0);
    }

    /* Set label height to max height if smaller to avoid panel
       resizing/jumping (see bug #10385). */
    cpuFreq->label.reset_size |= alloc.height < PANGO_PIXELS_CEIL (extents.height);
  }

  if (cpuFreq->label.reset_size)
  {
    gtk_widget_set_size_request (widget,
                                 PANGO_PIXELS_CEIL (extents.width),
                                 PANGO_PIXELS_CEIL (extents.height));
    cpuFreq->label.reset_size = FALSE;
    cpuFreq->layout_changed = TRUE;
  }

  pango_cairo_show_layout (cr, layout);

  g_object_unref (layout);
  cairo_restore (cr);
}

static gboolean
label_enter (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
  gtk_widget_set_state_flags (cpuFreq->button, GTK_STATE_FLAG_PRELIGHT, FALSE);
  return FALSE;
}

static gboolean
label_leave (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
  gtk_widget_unset_state_flags (cpuFreq->button, GTK_STATE_FLAG_PRELIGHT);
  return FALSE;
}



void
cpufreq_prepare_label ()
{
  if (cpuFreq->options->show_label_freq || cpuFreq->options->show_label_governor)
  {
    if (!cpuFreq->label.draw_area)
    {
      GtkWidget *draw_area = gtk_drawing_area_new ();
      gtk_widget_add_events (draw_area, GDK_ALL_EVENTS_MASK);
      g_signal_connect (draw_area, "draw", G_CALLBACK (label_draw), NULL);
      g_signal_connect (draw_area, "enter-notify-event", G_CALLBACK (label_enter), NULL);
      g_signal_connect (draw_area, "leave-notify-event", G_CALLBACK (label_leave), NULL);
      gtk_widget_set_halign (draw_area, GTK_ALIGN_CENTER);
      gtk_widget_set_valign (draw_area, GTK_ALIGN_CENTER);
      gtk_box_pack_start (GTK_BOX (cpuFreq->box), draw_area, TRUE, TRUE, 0);
      cpuFreq->label.draw_area = draw_area;
    }
  }
  else
  {
    if (cpuFreq->label.draw_area)
    {
      gtk_widget_destroy (cpuFreq->label.draw_area);
      cpuFreq->label.draw_area = NULL;
    }

    if (cpuFreq->label.text)
    {
      g_free (cpuFreq->label.text);
      cpuFreq->label.text = NULL;
    }
  }
}



static void
cpufreq_widgets ()
{
  /* create panel toggle button which will contain all other widgets */
  cpuFreq->button = xfce_panel_create_toggle_button ();
  xfce_panel_plugin_add_action_widget (cpuFreq->plugin, cpuFreq->button);
  gtk_container_add (GTK_CONTAINER (cpuFreq->plugin), cpuFreq->button);

  gchar *css = g_strdup_printf("button { padding: 0px; }");

  GtkCssProvider *provider = gtk_css_provider_new ();
  gtk_css_provider_load_from_data (provider, css, -1, NULL);
  gtk_style_context_add_provider (
    GTK_STYLE_CONTEXT (gtk_widget_get_style_context (cpuFreq->button)),
    GTK_STYLE_PROVIDER(provider),
    GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_free(css);

  cpuFreq->box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, SPACING);
  gtk_container_set_border_width (GTK_CONTAINER (cpuFreq->box), BORDER);
  gtk_container_add (GTK_CONTAINER (cpuFreq->button), cpuFreq->box);

  cpufreq_update_icon ();

  cpufreq_prepare_label ();

  g_signal_connect (cpuFreq->button, "button-press-event",
                    G_CALLBACK (cpufreq_overview), cpuFreq);

  /* activate panel widget tooltip */
  g_object_set (G_OBJECT (cpuFreq->button), "has-tooltip", TRUE, NULL);
  g_signal_connect (cpuFreq->button, "query-tooltip",
                    G_CALLBACK (cpufreq_update_tooltip), cpuFreq);

  gtk_widget_show_all (cpuFreq->button);

  cpufreq_update_plugin (TRUE);
}



static void
cpufreq_read_config ()
{
  CpuFreqPluginOptions *const options = cpuFreq->options;
  const gchar *value;

  gchar *file = xfce_panel_plugin_lookup_rc_file (cpuFreq->plugin);

  if (G_UNLIKELY (!file))
    file = xfce_panel_plugin_save_location (cpuFreq->plugin, FALSE);

  if (G_UNLIKELY (!file))
    return;

  XfceRc *rc = xfce_rc_simple_open (file, FALSE);
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
  options->icon_color_freq     = xfce_rc_read_bool_entry (rc, "icon_color_freq", FALSE);
  options->unit                = (CpuFreqUnit) xfce_rc_read_int_entry  (rc, "freq_unit", UNIT_DEFAULT);

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
    cpufreq_set_font (value);

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

  gchar *file = xfce_panel_plugin_save_location (plugin, TRUE);

  if (G_UNLIKELY (!file))
    return;

  XfceRc *rc = xfce_rc_simple_open (file, FALSE);
  g_free(file);

  xfce_rc_write_int_entry  (rc, "timeout",             options->timeout);
  xfce_rc_write_int_entry  (rc, "show_cpu",            options->show_cpu);
  xfce_rc_write_bool_entry (rc, "show_icon",           options->show_icon);
  xfce_rc_write_bool_entry (rc, "show_label_freq",     options->show_label_freq);
  xfce_rc_write_bool_entry (rc, "show_label_governor", options->show_label_governor);
  xfce_rc_write_bool_entry (rc, "show_warning",        options->show_warning);
  xfce_rc_write_bool_entry (rc, "keep_compact",        options->keep_compact);
  xfce_rc_write_bool_entry (rc, "one_line",            options->one_line);
  xfce_rc_write_bool_entry (rc, "icon_color_freq",     options->icon_color_freq);
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
    auto cpu = (CpuInfo*) g_ptr_array_index (cpuFreq->cpus, i);
    g_ptr_array_remove_fast (cpuFreq->cpus, cpu);
    cpuinfo_free (cpu);
  }

  g_ptr_array_free (cpuFreq->cpus, TRUE);

  g_free (cpuFreq->cpu_avg);
  g_free (cpuFreq->cpu_max);
  g_free (cpuFreq->cpu_min);

  cpufreq_destroy_icons ();

  if (cpuFreq->label.font_desc)
    pango_font_description_free (cpuFreq->label.font_desc);
  g_free (cpuFreq->label.text);

  g_free (cpuFreq->options->fontname);
  g_free (cpuFreq->options->fontcolor);
  g_free (cpuFreq->options);
  cpuFreq->plugin = NULL;
  g_free (cpuFreq);
  cpuFreq = NULL;
}



static gboolean
cpufreq_set_size (XfcePanelPlugin *plugin, gint size, CpuFreqPlugin *cpufreq)
{
  cpuFreq->panel_size = size;
  cpuFreq->panel_rows = xfce_panel_plugin_get_nrows (plugin);

  cpufreq_update_icon ();
  cpufreq_update_plugin (TRUE);

  return TRUE;
}

static void
cpufreq_show_about(XfcePanelPlugin *plugin, CpuFreqPlugin *cpufreq)
{
  /* List of authors (in alphabetical order) */
  const gchar *auth[] = {
    "Andre Miranda <andreldm@xfce.org>",
    "Florian Rivoal <frivoal@xfce.org>",
    "Harald Judt <h.judt@gmx.at>",
    "Jan Ziak <0xe2.0x9a.0x9b@xfce.org>",
    "Thomas Schreck <shrek@xfce.org>",
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
     "copyright", _("Copyright (c) 2003-2022\n"),
     "authors", auth,
     NULL);

  if (icon)
    g_object_unref(G_OBJECT(icon));
}

void
cpufreq_plugin_construct (XfcePanelPlugin *plugin)
{
  xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

  cpuFreq = g_new0 (CpuFreqPlugin, 1);
  cpuFreq->options = g_new0 (CpuFreqPluginOptions, 1);
  cpuFreq->plugin = plugin;
  cpuFreq->panel_mode = xfce_panel_plugin_get_mode (cpuFreq->plugin);
  cpuFreq->panel_rows = xfce_panel_plugin_get_nrows (cpuFreq->plugin);
  cpuFreq->panel_size = xfce_panel_plugin_get_size (cpuFreq->plugin);
  cpuFreq->cpus = g_ptr_array_new ();

  cpufreq_read_config ();
  cpuFreq->label.reset_size = TRUE;
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
  g_signal_connect (plugin, "about", G_CALLBACK (cpufreq_show_about), cpuFreq);
}
