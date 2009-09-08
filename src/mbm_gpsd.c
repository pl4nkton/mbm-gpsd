/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * mbm_gpsd - Enables user to take advantage of the builtin gps in MBM modules
 *
 * Copyright (C) 2008 Ericsson MBM
 *
 * Authors: Per Hallsmark <per.hallsmark@ericsson.com>
 *          Torgny Johansson <torgny.johansson@ericsson.com>
 *          Bjorn Runaker <bjorn.runaker@ericsson.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <errno.h>
#include <termios.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <libhal.h>
#include <dbus/dbus.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <poll.h>
#include <glib-object.h>
#include <dbus/dbus-glib.h>

#include "version.h"
#include "gps_settings.h"
#include "mbm_options.h"
#include "mbm_manager.h"

/* make sure we get the POSIX version of ptsname */
#define __USE_XOPEN
#define _XOPEN_SOURCE
#define _GNU_SOURCE
#include <stdlib.h>
#undef __USE_XOPEN

#define DBUS_FDO_NAME "org.freedesktop.DBus"
#define DBUS_FDO_PATH "/org/freedesktop/DBus"
#define DBUS_FDO_INTERFACE "org.freedesktop.DBus"

void sig_quit (int sig);
GMainLoop *g_loop;

static void destroy_cb (DBusGProxy * proxy, gpointer user_data)
{
	GMainLoop *loop = (GMainLoop *) user_data;

	g_message ("disconnected from the system bus, exiting.");
	g_main_loop_quit (loop);
}

static gboolean dbus_init (GMainLoop * loop)
{
	DBusGConnection *connection;
	DBusGProxy *proxy;
	GError *err = NULL;
	int request_name_result;

	connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, &err);
	if (!connection) {
		g_warning ("Could not get the system bus. Make sure "
				   "the message bus daemon is running! Message: %s",
				   err->message);
		g_error_free (err);
		return FALSE;
	}

	proxy = dbus_g_proxy_new_for_name (connection, DBUS_FDO_NAME,
									   DBUS_FDO_PATH,
									   DBUS_FDO_INTERFACE);

	if (!dbus_g_proxy_call (proxy, "RequestName", &err, G_TYPE_STRING,
							MBM_DBUS_SERVICE, G_TYPE_UINT,
							DBUS_NAME_FLAG_DO_NOT_QUEUE, G_TYPE_INVALID,
							G_TYPE_UINT, &request_name_result,
							G_TYPE_INVALID)) {
		g_warning ("Could not acquire the %s service.\n" "  Message: '%s'",
				   MBM_DBUS_SERVICE, err->message);
		g_error_free (err);
		goto err;
	}

	if (request_name_result != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
		g_warning ("Could not acquire the MBM GPS service "
				   "as it is already taken. Return: %d", request_name_result);
		goto err;
	}

	g_signal_connect (proxy, "destroy", G_CALLBACK (destroy_cb), loop);

	return TRUE;

  err:dbus_g_connection_unref (connection);
	g_object_unref (proxy);

	return FALSE;
}

static MBMManager *g_manager;

void sig_quit (int sig)
{
    MBMManagerPrivate *priv = MBM_MANAGER_GET_PRIVATE (g_manager);
    if (!priv->system_terminating) {
        if (mbm_options_debug ())
            g_debug ("Shutting down.");
        priv->system_terminating = 1;
        save_settings ();
        mbm_manager_quit (g_manager);
        g_main_loop_quit (g_loop);
    } else {
        if (mbm_options_debug ())
            g_debug ("The system is already shutting down. Please wait.");
    }
}

int main (int argc, char *argv[])
{
	struct sigaction sig_action, old_action;
	MBMManager *manager;
	MBMManagerPrivate *priv;

	sig_action.sa_handler = sig_quit;
	sigemptyset (&sig_action.sa_mask);
	sigaddset (&sig_action.sa_mask, SIGINT);
	sigaddset (&sig_action.sa_mask, SIGTERM);
    sigaddset (&sig_action.sa_mask, SIGHUP);
	sig_action.sa_flags = 0;
	sigaction (SIGINT, &sig_action, &old_action);
	sigaction (SIGTERM, &sig_action, NULL);
    sigaction (SIGHUP, &sig_action, NULL);

	mbm_options_parse (argc, argv);
	g_type_init ();

	if (!g_thread_supported ())
		g_thread_init (NULL);

	dbus_g_thread_init ();

	g_debug ("Started version: %s", VERSION);

	init_settings_table ();
	load_settings ();

	save_settings ();

	g_loop = g_main_loop_new (NULL, FALSE);

	if (!dbus_init (g_loop))
		return -1;

	g_manager = manager = mbm_manager_new ();

	priv = MBM_MANAGER_GET_PRIVATE (manager);

	g_main_loop_run (g_loop);

	g_object_unref (manager);
}
