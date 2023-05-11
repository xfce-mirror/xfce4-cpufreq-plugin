/*  xfce4-cpu-freq-plugin - panel plugin for cpu informations
 *
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

#ifndef XFCE4_CPUFREQ_LINUX_DBUS_H
#define XFCE4_CPUFREQ_LINUX_DBUS_H

#include <glib.h>

gboolean
cpufreq_dbus_set_governor (const gchar *governor, gint cpu, gboolean all, GError **error);

gboolean
cpufreq_dbus_set_preference (const gchar *preference, gint cpu, gboolean all, GError **error);

gboolean
cpufreq_dbus_set_max_freq (const gchar *frequency, gint cpu, gboolean all, GError **error);

gboolean
cpufreq_dbus_set_min_freq (const gchar *frequency, gint cpu, gboolean all, GError **error);

gboolean
cpufreq_dbus_set_hwp_dynamic_boost (const gchar *boost, GError **error);

gboolean
cpufreq_dbus_set_max_perf_pct (const gchar *pct, GError **error);

gboolean
cpufreq_dbus_set_min_perf_pct (const gchar *pct, GError **error);

gboolean
cpufreq_dbus_set_no_turbo (const gchar *turbo, GError **error);

gboolean
cpufreq_dbus_set_status (const gchar *status, GError **error);

#endif /* XFCE4_CPUFREQ_LINUX_DBUS_H */
