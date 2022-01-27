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

#include <algorithm>
#include <libxfce4ui/libxfce4ui.h>
#include <math.h>
#include <set>
#include <stdlib.h>
#include <string.h>

#include "plugin.h"
#include "xfce4-cpufreq-plugin.h"
#include "xfce4-cpufreq-configure.h"
#include "xfce4-cpufreq-overview.h"
#include "xfce4-cpufreq-utils.h"
#include "xfce4++/util.h"

#ifdef __linux__
#include "xfce4-cpufreq-linux.h"
#endif /* __linux__ */

Ptr0<CpuFreqPlugin> cpuFreq;



/*
 * Returns a single string describing governors of all CPUs, or an empty string.
 */
static std::string
cpufreq_governors ()
{
  /* Governors (in alphabetical ASCII order) */
  std::set<std::string> set;

  for (const Ptr<CpuInfo> &cpu : cpuFreq->cpus)
  {
    std::lock_guard<std::mutex> guard(cpu->mutex);
    if (cpu->shared.online && !cpu->shared.cur_governor.empty())
      set.insert(cpu->shared.cur_governor);
  }

  switch (set.size())
  {
  case 0:
    return std::string();
  case 1:
    return *set.begin();
  default:
    return xfce4::join(std::vector<std::string>(set.cbegin(), set.cend()), ",");
  }
}



static Ptr<CpuInfo>
cpufreq_cpus_calc_min ()
{
  const std::string governors = cpufreq_governors ();
  const std::string old_governor = cpuFreq->cpu_min ? cpuFreq->cpu_min->get_cur_governor() : std::string();
  guint freq = G_MAXUINT, max_freq_measured = G_MAXUINT, max_freq_nominal = G_MAXUINT, min_freq = G_MAXUINT;
  guint count = 0;

  for (const Ptr<CpuInfo> &cpu : cpuFreq->cpus)
  {
    std::lock_guard<std::mutex> guard(cpu->mutex);

    if (!cpu->shared.online)
      continue;

    freq = std::min (freq, cpu->shared.cur_freq);
    max_freq_measured = std::min (max_freq_measured, cpu->max_freq_measured);
    max_freq_nominal = std::min (max_freq_nominal, cpu->max_freq_nominal);
    min_freq = std::min (min_freq, cpu->min_freq);
    count++;
  }

  if (count == 0)
    freq = max_freq_measured = max_freq_nominal = min_freq = 0;

  Ptr<CpuInfo> cpu = xfce4::make<CpuInfo>();
  {
    std::lock_guard<std::mutex> guard(cpu->mutex);

    cpu->shared.cur_freq = freq;
    cpu->shared.cur_governor = !governors.empty() ? governors : _("current min");
    cpu->max_freq_measured = max_freq_measured;
    cpu->max_freq_nominal = max_freq_nominal;
    cpu->min_freq = min_freq;

    if (cpuFreq->options->show_label_governor && cpu->shared.cur_governor != old_governor)
    {
      cpuFreq->label.reset_size = true;
      cpuFreq->layout_changed = true;
    }
  }

  cpuFreq->cpu_min = cpu;
  return cpu;
}



static Ptr<CpuInfo>
cpufreq_cpus_calc_avg ()
{
  const std::string governors = cpufreq_governors ();
  const std::string old_governor = cpuFreq->cpu_avg ? cpuFreq->cpu_avg->get_cur_governor() : std::string();
  guint freq = 0, max_freq_measured = 0, max_freq_nominal = 0, min_freq = 0;
  guint count = 0;

  for (const Ptr<CpuInfo> &cpu : cpuFreq->cpus)
  {
    std::lock_guard<std::mutex> guard(cpu->mutex);

    if (!cpu->shared.online)
      continue;

    freq += cpu->shared.cur_freq;
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

  Ptr<CpuInfo> cpu = xfce4::make<CpuInfo>();
  {
    std::lock_guard<std::mutex> guard(cpu->mutex);

    cpu->shared.cur_freq = freq;
    cpu->shared.cur_governor = !governors.empty() ? governors : _("current avg");
    cpu->max_freq_measured = max_freq_measured;
    cpu->max_freq_nominal = max_freq_nominal;
    cpu->min_freq = min_freq;

    if (cpuFreq->options->show_label_governor && cpu->shared.cur_governor != old_governor)
    {
      cpuFreq->label.reset_size = true;
      cpuFreq->layout_changed = true;
    }
  }

  cpuFreq->cpu_avg = cpu;
  return cpu;
}



static Ptr<CpuInfo>
cpufreq_cpus_calc_max ()
{
  const std::string governors = cpufreq_governors ();
  const std::string old_governor = cpuFreq->cpu_max ? cpuFreq->cpu_max->get_cur_governor() : std::string();
  guint freq = 0, max_freq_measured = 0, max_freq_nominal = 0, min_freq = 0;

  for (const Ptr<CpuInfo> &cpu : cpuFreq->cpus)
  {
    std::lock_guard<std::mutex> guard(cpu->mutex);

    if (!cpu->shared.online)
      continue;

    freq = std::max (freq, cpu->shared.cur_freq);
    max_freq_measured = std::max (max_freq_measured, cpu->max_freq_measured);
    max_freq_nominal = std::max (max_freq_nominal, cpu->max_freq_nominal);
    min_freq = std::max (min_freq, cpu->min_freq);
  }

  Ptr<CpuInfo> cpu = xfce4::make<CpuInfo>();
  {
    std::lock_guard<std::mutex> guard(cpu->mutex);

    cpu->shared.cur_freq = freq;
    cpu->shared.cur_governor = !governors.empty() ? governors : _("current max");
    cpu->max_freq_measured = max_freq_measured;
    cpu->max_freq_nominal = max_freq_nominal;
    cpu->min_freq = min_freq;

    if (cpuFreq->options->show_label_governor && cpu->shared.cur_governor != old_governor)
    {
      cpuFreq->label.reset_size = true;
      cpuFreq->layout_changed = true;
    }
  }

  cpuFreq->cpu_max = cpu;
  return cpu;
}



static void
cpufreq_update_label (const Ptr<CpuInfo> &cpu)
{
  auto options = cpuFreq->options;

  if (!cpuFreq->label.draw_area)
    return;

  GtkWidget *label_widget = cpuFreq->label.draw_area;

  if (!options->show_label_governor && !options->show_label_freq)
  {
    gtk_widget_hide (label_widget);
    return;
  }

  std::string label;
  {
    std::lock_guard<std::mutex> guard(cpu->mutex);

    if (options->show_label_freq)
    {
      std::string freq = cpufreq_get_human_readable_freq (cpu->shared.cur_freq, options->unit);
      label += freq;
    }
    if (options->show_label_governor && !cpu->shared.cur_governor.empty())
    {
      if (!label.empty())
        label += options->one_line ? " " : "\n";
      label += cpu->shared.cur_governor;
    }
  }

  if (!label.empty())
  {
    if (!gtk_widget_is_visible (label_widget))
      gtk_widget_show (label_widget);

    if (cpuFreq->label.text != label)
    {
      cpuFreq->label.text = label;
      gtk_widget_queue_draw (label_widget);
    }
  }
  else
  {
    if (gtk_widget_is_visible (label_widget))
      gtk_widget_hide (label_widget);

    cpuFreq->label.text.clear();
  }
}



static void
cpufreq_widgets_layout ()
{
  GtkOrientation orientation = GTK_ORIENTATION_HORIZONTAL;
  bool resized = false;
  const bool hide_label = (!cpuFreq->options->show_label_freq && !cpuFreq->options->show_label_governor);
  gint pos = 1, lw = 0, lh = 0, iw = 0, ih = 0;

  /* keep plugin small if label is hidden or user requested compact size */
  const bool small = (hide_label ? true : cpuFreq->options->keep_compact);

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
    xfce_panel_plugin_set_small (cpuFreq->plugin, hide_label ? true : false);
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
    resized = true;
  }
  else if (orientation == GTK_ORIENTATION_HORIZONTAL &&
    lw + iw + BORDER * 2 >= cpuFreq->panel_size &&
    (cpuFreq->panel_mode == XFCE_PANEL_PLUGIN_MODE_DESKBAR || !small))
  {
    orientation = GTK_ORIENTATION_VERTICAL;
    resized = true;
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
        cpuFreq->icon, false, false, 0, GTK_PACK_START);
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
        cpuFreq->icon, true, true, 0, GTK_PACK_START);
  }

  if (cpuFreq->label.draw_area)
  {
    gtk_box_reorder_child (GTK_BOX (cpuFreq->box), cpuFreq->label.draw_area, pos);
    gtk_widget_queue_draw (cpuFreq->label.draw_area);
  }

  cpuFreq->layout_changed = false;
}



static Ptr0<CpuInfo>
cpufreq_current_cpu ()
{
  if (G_UNLIKELY (cpuFreq->options->show_cpu >= (ssize_t) cpuFreq->cpus.size()))
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

  Ptr0<CpuInfo> cpu;
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
    if (cpuFreq->options->show_cpu >= 0 && guint(cpuFreq->options->show_cpu) < cpuFreq->cpus.size())
      cpu = cpuFreq->cpus[cpuFreq->options->show_cpu];
  }

  return cpu;
}



static void
cpufreq_update_pixmap (const Ptr<CpuInfo> &cpu)
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
  for (auto count : cpuFreq->freq_hist)
    total_count += count;
  if (total_count * 0.01 < 1)
  {
    /* Not enough data to reliably compute the percentile,
     * resort to a value that isn't based on statistics */
    freq_99 = std::max (cpu->max_freq_nominal, cpu->max_freq_measured);
  }
  else
  {
    gint percentile_1 = total_count * 0.01;
    for (gssize i = G_N_ELEMENTS (cpuFreq->freq_hist) - 1; i >= 0; i--)
    {
      guint16 count = cpuFreq->freq_hist[i];
      if (count < percentile_1)
        percentile_1 -= count;
      else
      {
        freq_99 = FREQ_HIST_MIN + i * ((gdouble) (FREQ_HIST_MAX - FREQ_HIST_MIN) / FREQ_HIST_BINS);
        break;
      }
    }
  }

  const gdouble range = freq_99 - cpu->min_freq;
  gdouble normalized_freq;
  {
    std::lock_guard<std::mutex> guard(cpu->mutex);
    if (cpu->shared.cur_freq > cpu->min_freq && range >= min_range)
      normalized_freq = (cpu->shared.cur_freq - cpu->min_freq) / range;
    else
      normalized_freq = 0;
  }

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
        pixels[p] = std::max (pixels[p], color);
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



void
cpufreq_update_plugin (bool reset_label_size)
{
  Ptr0<CpuInfo> cpu0 = cpufreq_current_cpu ();
  if (!cpu0)
    return;

  Ptr<CpuInfo> cpu = cpu0.toPtr();

  if (reset_label_size)
  {
    cpuFreq->label.reset_size = true;
    cpuFreq->layout_changed = true;
  }

  cpufreq_update_label (cpu);

  if (cpuFreq->options->icon_color_freq)
    cpufreq_update_pixmap (cpu);

  if (cpuFreq->layout_changed)
    cpufreq_widgets_layout ();
}

static xfce4::TooltipTime
cpufreq_update_tooltip (GtkTooltip *tooltip)
{
  Ptr0<CpuInfo> cpu = cpufreq_current_cpu ();

  std::string tooltip_msg;
  if (G_UNLIKELY (cpu == nullptr))
  {
    tooltip_msg = _("No CPU information available.");
  }
  else
  {
    auto options = cpuFreq->options;
    if (options->show_label_governor && options->show_label_freq)
    {
      size_t num_cpu = cpuFreq->cpus.size();
      tooltip_msg = xfce4::sprintf (ngettext ("%zu cpu available", "%zu cpus available", num_cpu), num_cpu);
    }
    else
    {
      std::lock_guard<std::mutex> guard(cpu->mutex);

      if(!options->show_label_freq)
      {
        tooltip_msg += _("Frequency: ");
        tooltip_msg += cpufreq_get_human_readable_freq (cpu->shared.cur_freq, options->unit);
      }
      if(!options->show_label_governor && !cpu->shared.cur_governor.empty())
      {
        if(!tooltip_msg.empty())
          tooltip_msg += "\n";
        tooltip_msg += _("Governor: ");
        tooltip_msg += cpu->shared.cur_governor;
      }
    }
  }

  gtk_tooltip_set_text (tooltip, tooltip_msg.c_str());
  return xfce4::NOW;
}



void
cpufreq_restart_timeout ()
{
#ifdef __linux__
  g_source_remove (cpuFreq->timeoutHandle);
  cpuFreq->timeoutHandle = 0;

  int timeout_ms = int(1000 * cpuFreq->options->timeout);
  if (G_LIKELY (timeout_ms >= 10))
  {
    cpuFreq->timeoutHandle = xfce4::timeout_add (timeout_ms, []() {
        cpufreq_update_cpus ();
        return xfce4::TIMEOUT_AGAIN;
    });
  }
#endif
}



static void
cpufreq_mode_changed (XfcePanelPlugin *plugin, XfcePanelPluginMode mode)
{
  cpuFreq->panel_mode = mode;
  cpuFreq->panel_rows = xfce_panel_plugin_get_nrows (plugin);
  cpufreq_update_plugin (true);
}



void
cpufreq_update_icon ()
{
  auto options = cpuFreq->options;

  cpuFreq->destroy_icons();

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
      gtk_box_pack_start (GTK_BOX (cpuFreq->box), cpuFreq->icon, false, false, 0);
      gtk_box_reorder_child (GTK_BOX (cpuFreq->box), cpuFreq->icon, 0);
      gtk_widget_show (cpuFreq->icon);
    }
  }
}



static xfce4::Propagation
label_draw (GtkWidget *widget, cairo_t *cr)
{
  if (cpuFreq->label.text.empty())
    return xfce4::PROPAGATE;

  cairo_save (cr);

  GtkAllocation alloc;
  gtk_widget_get_allocation (widget, &alloc);
  PangoContext *pango_context = gtk_widget_get_pango_context (widget);
  GtkStyleContext *style_context = gtk_widget_get_style_context (widget);

  GdkRGBA color;
  if (!cpuFreq->options->fontcolor.empty())
  {
    gdk_rgba_parse (&color, cpuFreq->options->fontcolor.c_str());
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

  pango_layout_set_text (layout, cpuFreq->label.text.c_str(), -1);

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
    cpuFreq->label.reset_size = false;
    cpuFreq->layout_changed = true;
  }

  pango_cairo_show_layout (cr, layout);

  g_object_unref (layout);
  cairo_restore (cr);

  return xfce4::PROPAGATE;
}

static xfce4::Propagation
label_enter (GtkWidget*, GdkEventCrossing*)
{
  gtk_widget_set_state_flags (cpuFreq->button, GTK_STATE_FLAG_PRELIGHT, false);
  return xfce4::STOP;
}

static xfce4::Propagation
label_leave (GtkWidget*, GdkEventCrossing*)
{
  gtk_widget_unset_state_flags (cpuFreq->button, GTK_STATE_FLAG_PRELIGHT);
  return xfce4::STOP;
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
      xfce4::connect_draw (draw_area, label_draw);
      xfce4::connect_enter_notify (draw_area, label_enter);
      xfce4::connect_leave_notify (draw_area, label_leave);
      gtk_widget_set_halign (draw_area, GTK_ALIGN_CENTER);
      gtk_widget_set_valign (draw_area, GTK_ALIGN_CENTER);
      gtk_box_pack_start (GTK_BOX (cpuFreq->box), draw_area, true, true, 0);
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

    cpuFreq->label.text.clear();
  }
}



static void
cpufreq_widgets ()
{
  /* create panel toggle button which will contain all other widgets */
  cpuFreq->button = xfce_panel_create_toggle_button ();
  xfce_panel_plugin_add_action_widget (cpuFreq->plugin, cpuFreq->button);
  gtk_container_add (GTK_CONTAINER (cpuFreq->plugin), cpuFreq->button);

  const gchar *css = "button { padding: 0px; }";

  GtkCssProvider *provider = gtk_css_provider_new ();
  gtk_css_provider_load_from_data (provider, css, -1, NULL);
  gtk_style_context_add_provider (
    GTK_STYLE_CONTEXT (gtk_widget_get_style_context (cpuFreq->button)),
    GTK_STYLE_PROVIDER(provider),
    GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  cpuFreq->box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, SPACING);
  gtk_container_set_border_width (GTK_CONTAINER (cpuFreq->box), BORDER);
  gtk_container_add (GTK_CONTAINER (cpuFreq->button), cpuFreq->box);

  cpufreq_update_icon ();

  cpufreq_prepare_label ();

  xfce4::connect_button_press (cpuFreq->button, [](GtkWidget*, GdkEventButton *event) {
    return cpufreq_overview (event) ? xfce4::STOP : xfce4::PROPAGATE;
  });

  /* activate panel widget tooltip */
  g_object_set (G_OBJECT (cpuFreq->button), "has-tooltip", true, NULL);
  xfce4::connect_query_tooltip (cpuFreq->button, [](GtkWidget*, gint x, gint y, bool keyboard, GtkTooltip *tooltip) {
      return cpufreq_update_tooltip (tooltip);
  });

  gtk_widget_show_all (cpuFreq->button);

  cpufreq_update_plugin (true);
}



static void
cpufreq_read_config ()
{
  auto options = cpuFreq->options;

  gchar *file = xfce_panel_plugin_lookup_rc_file (cpuFreq->plugin);
  if (G_UNLIKELY (!file))
    return;

  const auto rc = xfce4::Rc::simple_open(file, true);
  g_free (file);

  if (rc)
  {
    const CpuFreqPluginOptions defaults;

    options->timeout             = rc->read_float_entry("timeout", defaults.timeout);
    options->show_cpu            = rc->read_int_entry  ("show_cpu", defaults.show_cpu);
    options->show_icon           = rc->read_bool_entry ("show_icon", defaults.show_icon);
    options->show_label_freq     = rc->read_bool_entry ("show_label_freq", defaults.show_label_freq);
    options->show_label_governor = rc->read_bool_entry ("show_label_governor", defaults.show_label_governor);
    options->show_warning        = rc->read_bool_entry ("show_warning", defaults.show_warning);
    options->keep_compact        = rc->read_bool_entry ("keep_compact", defaults.keep_compact);
    options->one_line            = rc->read_bool_entry ("one_line", defaults.one_line);
    options->icon_color_freq     = rc->read_bool_entry ("icon_color_freq", defaults.icon_color_freq);
    options->fontcolor           = rc->read_entry      ("fontcolor", defaults.fontcolor);
    options->unit                = (CpuFreqUnit) rc->read_int_entry ("freq_unit", defaults.unit);

    auto fontname = rc->read_entry ("fontname", defaults.fontname);
    cpuFreq->set_font (fontname);

    rc->close ();
  }

  options->validate();
}



void
cpufreq_write_config (XfcePanelPlugin *plugin)
{
  auto options = cpuFreq->options;

  gchar *file = xfce_panel_plugin_save_location (plugin, true);
  if (G_UNLIKELY (!file))
    return;

  auto rc = xfce4::Rc::simple_open (file, false);
  g_free(file);

  if (rc)
  {
    const CpuFreqPluginOptions defaults;

    rc->write_default_float_entry("timeout",             options->timeout, defaults.timeout, 0.001);
    rc->write_default_int_entry  ("show_cpu",            options->show_cpu, defaults.show_cpu);
    rc->write_default_bool_entry ("show_icon",           options->show_icon, defaults.show_icon);
    rc->write_default_bool_entry ("show_label_freq",     options->show_label_freq, defaults.show_label_freq);
    rc->write_default_bool_entry ("show_label_governor", options->show_label_governor, defaults.show_label_governor);
    rc->write_default_bool_entry ("show_warning",        options->show_warning, defaults.show_warning);
    rc->write_default_bool_entry ("keep_compact",        options->keep_compact, defaults.keep_compact);
    rc->write_default_bool_entry ("one_line",            options->one_line, defaults.one_line);
    rc->write_default_bool_entry ("icon_color_freq",     options->icon_color_freq, defaults.icon_color_freq);
    rc->write_default_int_entry  ("freq_unit",           options->unit, defaults.unit);
    rc->write_default_entry      ("fontname",            options->fontname, defaults.fontname);
    rc->write_default_entry      ("fontcolor",           options->fontcolor, defaults.fontcolor);

    rc->close ();
  }
}



static void
cpufreq_free (XfcePanelPlugin*)
{
  if (cpuFreq->timeoutHandle)
  {
    g_source_remove (cpuFreq->timeoutHandle);
    cpuFreq->timeoutHandle = 0;
  }

  cpuFreq = nullptr;
}



static xfce4::PluginSize
cpufreq_set_size (XfcePanelPlugin *plugin, gint size)
{
  cpuFreq->panel_size = size;
  cpuFreq->panel_rows = xfce_panel_plugin_get_nrows (plugin);

  cpufreq_update_icon ();
  cpufreq_update_plugin (true);

  return xfce4::RECTANGLE;
}

static void
cpufreq_show_about(XfcePanelPlugin*)
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

  cpuFreq = xfce4::make<CpuFreqPlugin>(plugin);

  cpufreq_read_config ();
  cpuFreq->label.reset_size = true;
  cpuFreq->layout_changed = true;

#ifdef __linux__
  if (!cpufreq_linux_init ())
    xfce_dialog_show_error (NULL, NULL,
      _("Your system is not configured correctly to support CPU frequency scaling!"));

  gtk_widget_set_size_request (GTK_WIDGET (plugin), -1, -1);
  cpufreq_widgets ();

  guint timeout_ms = int(1000 * cpuFreq->options->timeout);
  if (G_LIKELY (timeout_ms >= 10))
  {
    cpuFreq->timeoutHandle = xfce4::timeout_add (timeout_ms, []() {
        cpufreq_update_cpus ();
        return xfce4::TIMEOUT_AGAIN;
    });
  }
#else
  xfce_dialog_show_error (NULL, NULL, _("Your system is not supported yet!"));
#endif /* __linux__ */

  xfce4::connect_free_data (plugin, cpufreq_free);
  xfce4::connect_save (plugin, cpufreq_write_config);
  xfce4::connect_size_changed (plugin, cpufreq_set_size);
  xfce4::connect_mode_changed (plugin, cpufreq_mode_changed);

  /* Enable the configure and about menu items (they are hidden by default) */
  xfce_panel_plugin_menu_show_configure (plugin);
  xfce_panel_plugin_menu_show_about (plugin);
  xfce4::connect_configure_plugin (plugin, cpufreq_configure);
  xfce4::connect_about (plugin, cpufreq_show_about);
}



std::string CpuInfo::get_cur_governor() const
{
    std::lock_guard<std::mutex> guard(mutex);
    return shared.cur_governor;
}



CpuFreqPlugin::CpuFreqPlugin(XfcePanelPlugin *_plugin) : plugin(_plugin)
{
  panel_mode = xfce_panel_plugin_get_mode (plugin);
  panel_rows = xfce_panel_plugin_get_nrows (plugin);
  panel_size = xfce_panel_plugin_get_size (plugin);
}

CpuFreqPlugin::~CpuFreqPlugin()
{
  g_info ("%s", __PRETTY_FUNCTION__);

  if (G_UNLIKELY (timeoutHandle))
    g_source_remove (timeoutHandle);

  if (label.font_desc)
    pango_font_description_free (label.font_desc);

  destroy_icons();
}

void CpuFreqPlugin::destroy_icons()
{
  if (icon)
  {
    gtk_widget_destroy (icon);
    icon = NULL;
  }

  if (base_icon)
  {
    g_object_unref (G_OBJECT (base_icon));
    base_icon = NULL;
  }

  for (gsize i = 0; i < G_N_ELEMENTS (icon_pixmaps); i++)
    if (icon_pixmaps[i])
    {
      g_object_unref (G_OBJECT (icon_pixmaps[i]));
      icon_pixmaps[i] = NULL;
    }

  current_icon_pixmap = NULL;
}

void CpuFreqPlugin::set_font(const std::string &fontname_orEmpty)
{
  if (label.font_desc)
  {
    pango_font_description_free (label.font_desc);
    label.font_desc = NULL;
  }

  if (!fontname_orEmpty.empty())
  {
    options->fontname = fontname_orEmpty;
    label.font_desc = pango_font_description_from_string (fontname_orEmpty.c_str());
  }
  else
  {
    options->fontname.clear();
  }
}



void CpuFreqPluginOptions::validate()
{
  if (timeout < TIMEOUT_MIN)
    timeout = TIMEOUT_MIN;
  else if (timeout > TIMEOUT_MAX)
    timeout = TIMEOUT_MAX;

  if (!show_label_freq && !show_label_governor)
    show_icon = true;

  switch (unit)
  {
  case UNIT_AUTO:
  case UNIT_GHZ:
  case UNIT_MHZ:
    break;
  default:
    unit = UNIT_DEFAULT;
  }
}
