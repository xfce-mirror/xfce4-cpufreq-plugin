/*  xfce4-cpu-freq-plugin - panel plugin for cpu informations
 *
 *  Copyright (c) 2006 Thomas Schreck <shrek@xfce.org>
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

#ifndef XFCE4_CPUFREQ_UTILS_H
#define XFCE4_CPUFREQ_UTILS_H

#include "xfce4-cpufreq-plugin.h"

G_BEGIN_DECLS

gchar*
cpufreq_get_human_readable_freq (guint freq, CpuFreqUnit unit);

guint
cpufreq_get_normal_freq (const gchar *freq);

void
cpufreq_warn_reset (void);

G_END_DECLS

#endif /* XFCE4_CPUFREQ_UTILS_H */
