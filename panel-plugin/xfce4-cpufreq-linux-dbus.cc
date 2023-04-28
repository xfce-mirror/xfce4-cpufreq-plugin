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
call_dbus_func (const gchar* func, const gchar* param, gint cpu, gboolean all, GError **error)
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
            g_variant_new ("(sib)", param, cpu, all),
            NULL,
            G_DBUS_CALL_FLAGS_NONE,
            -1,
            NULL,
            error);

  if (call != NULL)
    g_variant_unref (call);
  g_object_unref (conn);

  return call != NULL;
}

gboolean
cpufreq_dbus_set_governor (const gchar* governor, gint cpu, gboolean all, GError **error)
{
  return call_dbus_func ("set_governor", governor, cpu, all, error);
}

gboolean
cpufreq_dbus_set_frequency (const gchar* frequency, gint cpu, gboolean all, GError **error)
{
  return call_dbus_func ("set_frequency", frequency, cpu, all, error);
}

