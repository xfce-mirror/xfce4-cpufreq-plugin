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

#include <stdbool.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <gio/gio.h>

GMainLoop *mainloop;
guint conn_id;
guint bus_id;

void set_frequency (const gchar *frequency, gint cpu, gboolean all);
void set_governor (const gchar *governor, gint cpu, gboolean all);

#define SYSFS_BASE  "/sys/devices/system/cpu"

/*
 *
 */
void
set_frequency (const gchar *frequency, gint cpu, gboolean all)
{
  gchar filename[100];
  GError *error = NULL;

  for (gint n = all ? 0 : cpu; n <= cpu; n++)
  {
    g_snprintf (filename, sizeof (filename),
      SYSFS_BASE "/cpu%d/cpufreq/scaling_max_freq", n);

    if (!g_file_set_contents_full (filename, frequency,
      -1, G_FILE_SET_CONTENTS_NONE, 0666, &error))
    {
      g_warning ("Failed to write to %s: %s", filename, error->message);
      g_clear_error (&error);
    }
  }
}

/*
 *
 */
void
set_governor (const gchar *governor, gint cpu, gboolean all)
{
  gchar filename[100];
  GError *error = NULL;

  for (gint n = all ? 0 : cpu; n <= cpu; n++)
  {
    g_snprintf (filename, sizeof(filename),
      SYSFS_BASE "/cpu%d/cpufreq/scaling_governor", n);

    if (!g_file_set_contents_full (filename, governor,
      -1, G_FILE_SET_CONTENTS_NONE, 0666, &error))
    {
      g_warning ("Failed to write to %s: %s", filename, error->message);
      g_clear_error (&error);
    }
  }
}

static void
server_message_handler (GDBusConnection *conn,
      const gchar *sender,
      const gchar *object_path,
      const gchar *interface_name,
      const gchar *method_name,
      GVariant *parameters,
      GDBusMethodInvocation *invocation,
      gpointer user_data)
{
  if (!g_strcmp0 (method_name, "set_governor"))
  {
    gchar *governor;
    gint cpu;
    gboolean all;
    g_variant_get (parameters, "(sib)", &governor, &cpu, &all);
    set_governor (governor, cpu, all);
    g_free (governor);
  }
  else if (!g_strcmp0 (method_name, "set_frequency"))
  {
    gchar *frequency;
    gint cpu;
    gboolean all;
    g_variant_get (parameters, "(sib)", &frequency, &cpu, &all);
    set_frequency (frequency, cpu, all);
    g_free (frequency);
  }
  g_dbus_method_invocation_return_value (invocation, NULL);
  g_bus_unown_name (bus_id);
  g_dbus_connection_unregister_object (conn, conn_id);
  g_dbus_connection_close (conn, NULL, NULL, NULL);
  g_main_loop_quit (mainloop);
}

static const GDBusInterfaceVTable interface_vtable = {
  &server_message_handler
};

static const gchar introspection_xml[] =
  "<node>"
  "  <interface name='org.xfce.cpufreq.CPUInterface'>"
  "    <method name='set_governor'>"
  "      <arg type='s' name='governor' direction='in'/>"
  "      <arg type='i' name='cpu' direction='in'/>"
  "      <arg type='b' name='all' direction='in'/>"
  "    </method>"
  "    <method name='set_frequency'>"
  "      <arg type='s' name='frequency' direction='in'/>"
  "      <arg type='i' name='cpu' direction='in'/>"
  "      <arg type='b' name='all' direction='in'/>"
  "    </method>"
  "  </interface>"
  "</node>";

int
main (void)
{
  GError *err = NULL;
  GDBusConnection *conn;
  GDBusNodeInfo *introspection_data;
  GDBusInterfaceInfo *interface_info;

  conn = g_bus_get_sync (G_BUS_TYPE_SYSTEM, NULL, &err);
  if (err != NULL)
  {
    g_critical ("Failed to get a system DBus connection: %s\n", err->message);
    g_clear_error (&err);
    return EXIT_FAILURE;
  }

  mainloop = g_main_loop_new (NULL, false);
  introspection_data = g_dbus_node_info_new_for_xml (introspection_xml, NULL);
  interface_info = g_dbus_node_info_lookup_interface (introspection_data,
               "org.xfce.cpufreq.CPUInterface");
  conn_id = g_dbus_connection_register_object (conn, "/org/xfce/cpufreq/CPUObject",
             interface_info, &interface_vtable, NULL, NULL, NULL);
  bus_id = g_bus_own_name (G_BUS_TYPE_SYSTEM, "org.xfce.cpufreq.CPUChanger",
      G_BUS_NAME_OWNER_FLAGS_NONE, NULL, NULL, NULL, NULL, NULL);
  g_main_loop_run (mainloop);

  g_object_unref (conn);
  g_main_loop_unref (mainloop);

  return EXIT_SUCCESS;
}
