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
#include <fcntl.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <gio/gio.h>

GMainLoop *mainloop;

void set_frequency (const gchar *frequency, gint cpu, gboolean all);
void set_governor (const gchar *governor, gint cpu, gboolean all);

#define SYSFS_BASE  "/sys/devices/system/cpu"

/*
 *
 */
void
set_frequency (const gchar *frequency, gint cpu, gboolean all)
{
  FILE *fp;
  for (gint n = all ? 0 : cpu; n <= cpu; n++)
  {
    gchar filename[100];
    g_snprintf (filename, sizeof (filename),
      SYSFS_BASE "/cpu%d/cpufreq/scaling_min_freq", n);
    if ((fp = g_fopen (filename, "w")) != NULL)
    {
      g_printerr ("Writing '%s' to %s\n", frequency, filename);
      g_fprintf (fp, "%s", frequency);
      fclose (fp);
    }
  }
}

/*
 *
 */
void
set_governor (const gchar *governor, gint cpu, gboolean all)
{
  FILE *fp;
  for (int n = all ? 0 : cpu; n <= cpu; n++)
  {
    gchar filename[100];
    g_snprintf (filename, sizeof(filename),
      SYSFS_BASE "/cpu%d/cpufreq/scaling_governor", n);
    if ((fp = g_fopen (filename, "w")) != NULL)
    {
      g_printerr ("Writing '%s' to %s\n", governor, filename);
      g_fprintf (fp, "%s", governor);
      fclose (fp);
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
    g_dbus_method_invocation_return_value (invocation, g_variant_new ("(s)", "Done"));
    g_free (governor);
  }
  else if (!g_strcmp0 (method_name, "set_frequency"))
  {
    gchar *frequency;
    gint cpu;
    gboolean all;
    g_variant_get (parameters, "(sib)", &frequency, &cpu, &all);
    set_frequency (frequency, cpu, all);
    g_dbus_method_invocation_return_value (invocation, g_variant_new ("(s)", "Done"));
    g_free (frequency);
  }
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
  "      <arg type='s' name='response' direction='out'/>"
  "    </method>"
  "    <method name='set_frequency'>"
  "      <arg type='s' name='frequency' direction='in'/>"
  "      <arg type='i' name='cpu' direction='in'/>"
  "      <arg type='b' name='all' direction='in'/>"
  "      <arg type='s' name='response' direction='out'/>"
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
    g_fprintf (stderr, "Failed to get a system DBus connection: %s\n",
       err->message);
    g_error_free (err);
    return EXIT_FAILURE;
  }

  mainloop = g_main_loop_new (NULL, false);
  introspection_data = g_dbus_node_info_new_for_xml (introspection_xml, NULL);
  interface_info = g_dbus_node_info_lookup_interface (introspection_data,
               "org.xfce.cpufreq.CPUInterface");
  g_dbus_connection_register_object (conn, "/org/xfce/cpufreq/CPUObject",
             interface_info, &interface_vtable, NULL,
             NULL, NULL);
  g_bus_own_name (G_BUS_TYPE_SYSTEM, "org.xfce.cpufreq.CPUChanger",
      G_BUS_NAME_OWNER_FLAGS_NONE, NULL, NULL, NULL, NULL, NULL);
  g_main_loop_run (mainloop);

  return EXIT_SUCCESS;
}
