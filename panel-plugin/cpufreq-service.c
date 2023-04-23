#include <stdio.h>
#include <stdbool.h>
#include <gio/gio.h>

GMainLoop *mainloop;

void set_frequency(const char* frequency, int cpu, bool all);
void set_governor(const char* governor, int cpu, bool all);

/*
 *
 */
void set_frequency(const char* frequency, int cpu, bool all)
{
	FILE *fp;
	for (int n=all?0:cpu; n <= cpu; n++) {
		char filename[100];
		sprintf(filename,"/sys/devices/system/cpu/cpu%d/cpufreq/scaling_min_freq", n);
		if ((fp = fopen(filename, "w")) != NULL) {
			fprintf(stderr, "Writing '%s' to %s\n", frequency, filename);
			fprintf(fp, "%s", frequency);
			fclose(fp);
		}
	}
}

/*
 *
 */
void set_governor(const char* governor, int cpu, bool all)
{
	FILE *fp;
	for (int n=all?0:cpu; n <= cpu; n++) {
		char filename[100];
		sprintf(filename,"/sys/devices/system/cpu/cpu%d/cpufreq/scaling_governor", n);
		if ((fp = fopen(filename, "w")) != NULL) {
			fprintf(stderr, "Writing '%s' to %s\n", governor, filename);
			fprintf(fp, "%s", governor);
			fclose(fp);
		}
	}
}

static void server_message_handler(GDBusConnection *conn,
                               const gchar *sender,
                               const gchar *object_path,
                               const gchar *interface_name,
                               const gchar *method_name,
                               GVariant *parameters,
                               GDBusMethodInvocation *invocation,
                               gpointer user_data)
{
	if (!g_strcmp0(method_name, "set_governor")) {
		gchar *governor;
		gint cpu;
		gboolean all;
		g_variant_get(parameters, "(sib)", &governor, &cpu, &all);
		set_governor(governor, cpu, all);
		g_dbus_method_invocation_return_value(invocation, g_variant_new("(s)", "Done"));
		g_free(governor);
	} else if (!g_strcmp0(method_name, "set_frequency")) {
		gchar *frequency;
		gint cpu;
		gboolean all;
		g_variant_get(parameters, "(sib)", &frequency, &cpu, &all);
		set_frequency(frequency, cpu, all);
		g_dbus_method_invocation_return_value(invocation, g_variant_new("(s)", "Done"));
		g_free(frequency);
	}
	g_main_loop_quit(mainloop);
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

int main(void)
{
	GError *err = NULL;
	GDBusConnection *conn;
	GDBusNodeInfo *introspection_data;
	GDBusInterfaceInfo *interface_info;

	conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &err);
	if (err != NULL) {
		fprintf(stderr, "Failed to get a system DBus connection: %s\n", err->message);
		g_error_free(err);
		return EXIT_FAILURE;
	}

	mainloop = g_main_loop_new(NULL, false);
	introspection_data = g_dbus_node_info_new_for_xml(introspection_xml, NULL);
	interface_info = g_dbus_node_info_lookup_interface(introspection_data, "org.xfce.cpufreq.CPUInterface");
	g_dbus_connection_register_object(conn, "/org/xfce/cpufreq/CPUObject", interface_info, &interface_vtable, NULL, NULL, NULL);
	g_bus_own_name(G_BUS_TYPE_SYSTEM, "org.xfce.cpufreq.CPUChanger", G_BUS_NAME_OWNER_FLAGS_NONE, NULL, NULL, NULL, NULL, NULL);
	g_main_loop_run(mainloop);

	return EXIT_SUCCESS;
}

