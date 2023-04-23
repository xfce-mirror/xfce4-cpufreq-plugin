#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h> /* for glib main loop */

GMainLoop *mainloop;

void set_frequency(const char* frequency, int cpu, bool all);
void set_governor(const char* governor, int cpu, bool all);
DBusHandlerResult server_message_handler(DBusConnection *conn, DBusMessage *message, void *data);

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

/*
 * The main logic of CPUFreq daemon. Should call appropriate function upon message.
 */
DBusHandlerResult server_message_handler(DBusConnection *conn, DBusMessage *message, void *data)
{
	DBusHandlerResult result;
        DBusMessage *reply = NULL;
	DBusError err;
	bool quit = true;

	dbus_error_init(&err);

	if (dbus_message_is_method_call(message, "org.xfce.cpufreq.CPUInterface", "set_governor")) {
		const char *governor;
		dbus_int32_t cpu;
		dbus_bool_t all;

		if (!dbus_message_get_args(message, &err,
					   DBUS_TYPE_STRING, &governor,
					   DBUS_TYPE_INT32, &cpu,
					   DBUS_TYPE_BOOLEAN, &all,
					   DBUS_TYPE_INVALID))
			goto fail;

		set_governor(governor, cpu, all);

		if (!(reply = dbus_message_new_method_return(message)))
			goto fail;

		dbus_message_append_args(reply,
					 DBUS_TYPE_STRING, &governor,
					 DBUS_TYPE_INVALID);

	} else if (dbus_message_is_method_call(message, "org.xfce.cpufreq.CPUInterface", "set_frequency")) {
		const char *frequency;
		dbus_int32_t cpu;
		dbus_bool_t all;

		if (!dbus_message_get_args(message, &err,
					   DBUS_TYPE_STRING, &frequency,
					   DBUS_TYPE_INT32, &cpu,
					   DBUS_TYPE_BOOLEAN, &all,
					   DBUS_TYPE_INVALID))
			goto fail;

		set_frequency(frequency, cpu, all);

		if (!(reply = dbus_message_new_method_return(message)))
			goto fail;

		dbus_message_append_args(reply,
					 DBUS_TYPE_STRING, &frequency,
					 DBUS_TYPE_INVALID);
	} else
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

fail:
	if (dbus_error_is_set(&err)) {
		if (reply)
			dbus_message_unref(reply);
		reply = dbus_message_new_error(message, err.name, err.message);
		dbus_error_free(&err);
	}

	/*
	 * In any cases we should have allocated a reply otherwise it
	 * means that we failed to allocate one.
	 */
	if (!reply)
		return DBUS_HANDLER_RESULT_NEED_MEMORY;

	/* Send the reply which might be an error one too. */
	result = DBUS_HANDLER_RESULT_HANDLED;
	if (!dbus_connection_send(conn, reply, NULL))
		result = DBUS_HANDLER_RESULT_NEED_MEMORY;
	dbus_message_unref(reply);

	if (quit) {
		g_main_loop_quit(mainloop);
	}
	return result;
}


const DBusObjectPathVTable server_vtable = {
	.message_function = server_message_handler
};


int main(void)
{
	DBusConnection *conn;
	DBusError err;
	int rv;

    dbus_error_init(&err);

	/* connect to the daemon bus */
	conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
	if (!conn) {
		fprintf(stderr, "Failed to get a system DBus connection: %s\n", err.message);
		goto fail;
	}

	rv = dbus_bus_request_name(conn, "org.xfce.cpufreq.CPUChanger", DBUS_NAME_FLAG_REPLACE_EXISTING , &err);
	if (rv != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
		fprintf(stderr, "Failed to request name on bus: %s\n", err.message);
		goto fail;
	}

	if (!dbus_connection_register_object_path(conn, "/org/xfce/cpufreq/CPUObject", &server_vtable, NULL)) {
		fprintf(stderr, "Failed to register a object path for 'CPUObject'\n");
		goto fail;
	}

	mainloop = g_main_loop_new(NULL, false);
	/* Set up the DBus connection to work in a GLib event loop */
	dbus_connection_setup_with_g_main(conn, NULL);
	/* Start the glib event loop */
	g_main_loop_run(mainloop);

	return EXIT_SUCCESS;
fail:
	dbus_error_free(&err);
	return EXIT_FAILURE;
}

