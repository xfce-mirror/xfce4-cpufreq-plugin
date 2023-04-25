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

static void
call_dbus_func(const char* func, const char* param, int cpu, bool all)
{
	GDBusProxy *proxy;
	GDBusConnection *conn;
	GError *error = NULL;

	conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
	g_assert_no_error(error);

	proxy = g_dbus_proxy_new_sync(conn,
				      G_DBUS_PROXY_FLAGS_NONE,
				      NULL,				/* GDBusInterfaceInfo */
				      "org.xfce.cpufreq.CPUChanger",		/* name */
				      "/org/xfce/cpufreq/CPUObject",	/* object path */
				      "org.xfce.cpufreq.CPUInterface",	/* interface */
				      NULL,				/* GCancellable */
				      &error);
	g_assert_no_error(error);

	g_dbus_proxy_call_sync(proxy,
					func,
					g_variant_new ("(sib)", param, cpu, all),
					G_DBUS_CALL_FLAGS_NONE,
					-1,
					NULL,
					&error);
	g_assert_no_error(error);

	g_object_unref(proxy);
	g_object_unref(conn);
}

void
cpufreq_dbus_set_governor(const char* governor, int cpu, bool all)
{
	call_dbus_func("set_governor", governor, cpu, all);
}

void
cpufreq_dbus_set_frequency(const char* frequency, int cpu, bool all)
{
	call_dbus_func("set_frequency", frequency, cpu, all);
}

