/*  xfce4-cpu-freq-plugin - panel plugin for cpu informations
 *
 *  Copyright (c) 2018 Andre Miranda <andreldm@xfce.org>
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

#ifndef XFCE4_CPUFREQ_LINUX_SYSFS_H
#define XFCE4_CPUFREQ_LINUX_SYSFS_H

#include <glib.h>

G_BEGIN_DECLS

gboolean cpufreq_sysfs_is_available (void);

gboolean cpufreq_sysfs_read (void);

void cpufreq_sysfs_read_current (gint cpu_number);

void cpufreq_sysfs_read_int (const gchar *file, guint *intval);

G_END_DECLS

#endif /* XFCE4_CPUFREQ_LINUX_SYSFS_H */
