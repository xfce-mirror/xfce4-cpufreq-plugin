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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <string>
#include <string.h>
#include <vector>

#include <gio/gio.h>

#include "xfce4-cpufreq-plugin.h"
#include "xfce4-cpufreq-linux-dbus.h"

static gboolean
call_dbus_func (const gchar* func, GVariant *params, GError **error)
{
  GDBusConnection *conn;
  GVariant *call;

  conn = g_bus_get_sync (G_BUS_TYPE_SYSTEM, NULL, error);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  call = g_dbus_connection_call_sync (conn,
            "org.xfce.cpufreq.CPUChanger",    /* name */
            "/org/xfce/cpufreq/CPUObject",  /* object path */
            "org.xfce.cpufreq.CPUInterface",  /* interface */
            func,
            params,
            NULL,
            G_DBUS_CALL_FLAGS_NONE,
            -1,
            NULL,
            error);

  if (call != NULL)
    g_variant_unref (call);
  g_object_unref (conn);

  return ( (error == NULL) || (*error ==NULL));
}

gboolean
cpufreq_dbus_set_governor (const gchar* governor, gint cpu, gboolean all, GError **error)
{
  return call_dbus_func ("set_governor", g_variant_new ("(sib)", governor, cpu, all), error);
}

gboolean
cpufreq_dbus_set_preference (const gchar* preference, gint cpu, gboolean all, GError **error)
{
  return call_dbus_func ("set_preference", g_variant_new ("(sib)", preference, cpu, all), error);
}

gboolean
cpufreq_dbus_set_max_freq (const gchar* frequency, gint cpu, gboolean all, GError **error)
{
  return call_dbus_func ("set_max_freq", g_variant_new ("(sib)", frequency, cpu, all), error);
}

gboolean
cpufreq_dbus_set_min_freq (const gchar* frequency, gint cpu, gboolean all, GError **error)
{
  return call_dbus_func ("set_min_freq", g_variant_new ("(sib)", frequency, cpu, all), error);
}

gboolean
cpufreq_dbus_set_hwp_dynamic_boost (const gchar *boost, GError **error)
{
  return call_dbus_func ("set_hwp_dynamic_boost", g_variant_new ("(s)", boost), error);
}

gboolean
cpufreq_dbus_set_max_perf_pct (const gchar *pct, GError **error)
{
  return call_dbus_func ("set_max_perf_pct", g_variant_new ("(s)", pct), error);
}

gboolean
cpufreq_dbus_set_min_perf_pct (const gchar *pct, GError **error)
{
  return call_dbus_func ("set_min_perf_pct", g_variant_new ("(s)", pct), error);
}

gboolean
cpufreq_dbus_set_no_turbo (const gchar *turbo, GError **error)
{
  return call_dbus_func ("set_no_turbo", g_variant_new ("(s)", turbo), error);
}

