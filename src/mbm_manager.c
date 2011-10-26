/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * Copyright (C) 2009 Ericsson AB
 *
 * Authors: Torgny Johansson <torgny.johansson@ericsson.com>
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
#define __USE_XOPEN
#define _XOPEN_SOURCE
#define _GNU_SOURCE
#include <stdlib.h>
#undef __USE_XOPEN

#include <string.h>
#include <sys/inotify.h>
#include <gmodule.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <errno.h>
#include <termios.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <libudev.h>
#include <dbus/dbus.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <poll.h>
#include <glib-object.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include "mbm_manager.h"
#include "mbm_errors.h"
#include "mbm_serial.h"
#include "mbm_options.h"
#include "mbm_callback_info.h"
#include "gps_settings.h"
#include "mbm_supl.h"
#include "mbm_modem.h"

/*
 * Missing prototypes
 */
extern int getpt (void);
extern int chmod (__const char *__file, __mode_t __mode);
extern int grantpt (int fd);
extern int unlockpt (int fildes);

enum {
	GPS_NMEA_CAP=0,
	GPS_CTRL_CAP=1
};

const char *gps_capabilities[2] = {
	"NMEA",
	"CTRL"
};

void
impl_mbm_manager_enable (MBMManager * manager, gboolean enable,
						 DBusGMethodInvocation * context);

static gboolean probe_gps_ctrl (MBMManager * manager);
static gboolean recover_stalled_ctrl_device (MBMManager * manager);

#include "mbm-manager-glue.h"

G_DEFINE_TYPE (MBMManager, mbm_manager, G_TYPE_OBJECT)

enum {
	DEVICE_ADDED,
	DEVICE_REMOVED,

	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

MBMManager *mbm_manager_new (void)
{
	return g_object_new (MBM_TYPE_MANAGER, NULL);
}

/*
 * udev lookup of the given capability.
 * A string to the device file e.g. /dev/ttyACM0 is returned if successful
 * otherwise an empty string is returned.
 */

static char *find_device_file (int capability)
{
	struct udev *udev;
	struct udev_enumerate *enumerate;
	struct udev_list_entry *devices, *dev_list_entry;
	struct udev_device *dev, *parent_dev, *pparent_dev;
	const char *iface, *path;
	const char *idvendor, *idproduct;
	char *device = NULL;

	udev = udev_new();
	if (!udev) return NULL;

	enumerate = udev_enumerate_new(udev);
	udev_enumerate_add_match_subsystem(enumerate, "tty");
	udev_enumerate_add_match_subsystem(enumerate, "usb");
	udev_enumerate_scan_devices(enumerate);

	devices = udev_enumerate_get_list_entry(enumerate);
	udev_list_entry_foreach(dev_list_entry, devices) {
		path = udev_list_entry_get_name(dev_list_entry);
		dev = udev_device_new_from_syspath(udev, path);
		parent_dev = udev_device_get_parent(dev);
		pparent_dev = udev_device_get_parent(parent_dev);
		idvendor = udev_device_get_sysattr_value(pparent_dev,
			"idVendor");
		idproduct = udev_device_get_sysattr_value(pparent_dev,
			"idProduct");
		iface = udev_device_get_sysattr_value(parent_dev,
			"bInterfaceNumber");
		if (!idvendor || !idproduct || !iface) {
			udev_device_unref (dev);
			continue;
		}

		/* all the information from the legacy hal fdi file 
		 * related to gps_{ctrl,nmea}_capability is integrated
		 * in the following test
		 */
		if ((!strcmp(idvendor, "0bdb") &&
		     (!strcmp(idproduct, "1900") ||
		      !strcmp(idproduct, "1904") ||
		      !strcmp(idproduct, "1905") ||
		      !strcmp(idproduct, "1906") ||
		      !strcmp(idproduct, "1907") ||
		      !strcmp(idproduct, "1911"))) ||
		    (!strcmp(idvendor, "413c") &&
		     (!strcmp(idproduct, "8183") ||
		      !strcmp(idproduct, "8184") ||
		      !strcmp(idproduct, "818d"))) ||
		    (!strcmp(idvendor, "0930") &&
		     (!strcmp(idproduct, "130c") ||
		      !strcmp(idproduct, "1311")))) {
			if ((capability == GPS_NMEA_CAP && !strcmp(iface,"09")) ||
			    (capability == GPS_CTRL_CAP && !strcmp(iface,"05")))
			{
				device = strdup(udev_device_get_devnode(dev));
				udev_device_unref (dev);
				if (mbm_options_debug ()) {
					g_debug ("found device %s for capability %s",
					device, gps_capabilities[capability]);
				}
				break;
			}
		}
		udev_device_unref (dev);
	}
	udev_enumerate_unref(enumerate);
	udev_unref(udev);
	if (!device)
		g_debug ("Couldn't find any device with capability %s.",
			gps_capabilities[capability]);
	return device;
}

static void init_mbm_modem (MBMManager * manager)
{
	MBMManagerPrivate *priv = MBM_MANAGER_GET_PRIVATE (manager);
    
	modem_open_gps_ctrl (manager);
	/* check if module has stalled */
	if (!probe_gps_ctrl (manager)) {
		g_debug ("The gps ctrl device has stalled. Will try to recover.\n");
		if (!recover_stalled_ctrl_device (manager))
			modem_error_device_stalled (manager);
	}

    modem_check_gps_customization (manager);

	modem_check_pin (manager);
    modem_enable_unsolicited_responses (manager);
	modem_check_radio (manager);

    if (!mbm_gps_customization (STAND_ALONE_MODE)) {
        g_debug ("The module does not have GPS functionality. Exiting.");
        save_settings ();
        mbm_manager_quit (manager);
        exit (0);
    }
    
	modem_open_gps_nmea (manager);

	priv->gps_port_not_defined = 1;
}

static void *create_mbm_modem (MBMManager * manager)
{
	MBMManagerPrivate *priv = MBM_MANAGER_GET_PRIVATE (manager);

	/* set the ports */
	priv->nmea_dev = find_device_file (GPS_NMEA_CAP);
	priv->ctrl_dev = find_device_file (GPS_CTRL_CAP);

	if (!priv->nmea_dev || !priv->ctrl_dev) {
		g_warning ("No GPS devices found. Exiting.");
		mbm_manager_quit (manager);
		exit (0);
	}

	init_mbm_modem (manager);

	return NULL;
}

void
impl_mbm_manager_enable (MBMManager * manager, gboolean enable,
						 DBusGMethodInvocation * context)
{
	dbus_g_method_return (context);
}

static void create_initial_modems (MBMManager * manager)
{
	create_mbm_modem (manager);
}

static void set_pty_attributes (int fd)
{
	int status;
	struct termios settings;

	tcflush (fd, TCIOFLUSH);
	status = tcgetattr (fd, &settings);
	if (status) {
		syslog (LOG_ERR, "getting settings failed: %m");
		return;
	}
	settings.c_lflag = ISIG | ICANON | IEXTEN;
	cfsetispeed (&settings, B4800);
	cfsetospeed (&settings, B4800);
	status = tcsetattr (fd, TCSANOW, &settings);
	if (status) {
		syslog (LOG_ERR, "setting baudrate failed: %m");
		return;
	}
}

static void client_add (MBMManager * manager, int id)
{
	char user_dev[512];
	int status;
	MBMManagerPrivate *priv = MBM_MANAGER_GET_PRIVATE (manager);

	if (mbm_options_debug ())
		g_debug ("adding client %d\n", id);

	priv->client[id].master_fd = getpt ();
	if (priv->client[id].master_fd == -1) {
		g_error ("Open of ptmx failed: %m");
		exit (1);
	}
	FD_SET (priv->client[id].master_fd, &priv->fdsread);
	if (priv->client[id].master_fd >= priv->maxfd)
		priv->maxfd = priv->client[id].master_fd + 1;
	status = grantpt (priv->client[id].master_fd);
	if (status == -1) {
		g_error ("grantpt ptmx failed: %m");
		exit (1);
	}
	status = unlockpt (priv->client[id].master_fd);
	if (status == -1) {
		g_error ("unlock ptmx failed: %m");
		exit (1);
	}
	set_pty_attributes (priv->client[id].master_fd);
	priv->client[id].slave_dev = strdup (ptsname (priv->client[id].master_fd));
	sprintf (user_dev, "%s%d", CLIENT_DEV, id);
	unlink (user_dev);
	status = symlink (priv->client[id].slave_dev, user_dev);
	if (status) {
		g_error ("linking %s to %s failed: %m", user_dev,
				 priv->client[id].slave_dev);
		exit (1);
	}
	if (mbm_options_debug ())
		g_debug ("linking %s to %s OK\n", user_dev, priv->client[id].slave_dev);
	status = chmod (priv->client[id].slave_dev, 0666);
	if (status) {
		g_error ("correcting rights for %s failed: %m",
				 priv->client[id].slave_dev);
		exit (1);
	}
	if (mbm_options_debug ())
		g_debug ("rights set to 0666 for %s\n", priv->client[id].slave_dev);
	priv->client[id].slave_ifd = inotify_add_watch (priv->ifd, user_dev, IN_OPEN
													| IN_CLOSE_WRITE |
													IN_CLOSE_NOWRITE |
													IN_ONESHOT);
	if (priv->client[id].slave_ifd == -1) {
		g_error ("monitoring of %s failed: %m", user_dev);
		exit (1);
	}
	priv->client[id].at_cmd = malloc (MAX_RESPONSE);
	memset (priv->client[id].at_cmd, 0, MAX_RESPONSE);
	priv->client[id].active = 0;
}

static void client_remove (MBMManager * manager, int id)
{
	char user_dev[512];
	MBMManagerPrivate *priv = MBM_MANAGER_GET_PRIVATE (manager);

	if (mbm_options_debug ())
		g_debug ("removing client %d\n", id);

	free (priv->client[id].at_cmd);
	priv->client[id].active = 0;
	close (priv->client[id].master_fd);
	free (priv->client[id].slave_dev);
	sprintf (user_dev, "%s%d", CLIENT_DEV, id);
	unlink (user_dev);
}

static gboolean probe_gps_ctrl (MBMManager * manager)
{
	char *response, buf[50];
	int len;

	len = sprintf (buf, "AT\r\n");
	response = serial_send_AT_cmd (manager, buf, len);
	if (response) {
		free (response);
		return TRUE;
	} else {
		return FALSE;
	}
}

static gboolean recover_stalled_ctrl_device (MBMManager * manager)
{
	MBMManagerPrivate *priv = MBM_MANAGER_GET_PRIVATE (manager);
	char buf[15];
	int len, result, count, gps_enabled_current, probe_attempts;

	gps_enabled_current = priv->gps_enabled;
	priv->gps_enabled = GPS_RECOVERING;

	g_debug ("Resetting device");
	len = sprintf (buf, "AT*E2RESET\r\n");
	result = write (priv->ctrl_fd, buf, len);
    if (result == -1)
        g_debug ("Bad result from RESET write. Recovery proabably will not work.");

	g_debug ("Closing file handles");
	tcflush (priv->nmea_fd, TCIOFLUSH);
	tcflush (priv->ctrl_fd, TCIOFLUSH);
	FD_CLR (priv->nmea_fd, &priv->fdsread);
	FD_CLR (priv->ctrl_fd, &priv->fdsread);
	close (priv->nmea_fd);
	close (priv->ctrl_fd);

	g_debug ("Sleeping for 10 seconds to wait for the module to reappear");
	sleep (10);
	g_debug ("Trying to restore the file handles");

	if (priv->nmea_dev)
		g_free (priv->nmea_dev);
	if (priv->ctrl_dev)
		g_free (priv->ctrl_dev);

	priv->nmea_dev = find_device_file (GPS_NMEA_CAP);
	priv->ctrl_dev = find_device_file (GPS_CTRL_CAP);

	count = 0;
	while (count < 5) {
		if ((!priv->nmea_dev || !priv->ctrl_dev)) {
			g_debug ("All devices weren't found. Trying again in 5 seconds.");
			sleep (5);
			if (priv->nmea_dev)
				g_free (priv->nmea_dev);
			if (priv->ctrl_dev)
				g_free (priv->ctrl_dev);
			priv->nmea_dev = find_device_file (GPS_NMEA_CAP);
			priv->ctrl_dev = find_device_file (GPS_CTRL_CAP);
			count++;
		} else {
			g_debug ("Finally found all devices! Continuing.");
			break;
		}
	}
    
    if (count >= 5)
        g_error ("Unable to find all devices. Giving up.");

	modem_open_gps_ctrl (manager);

	g_debug ("Probing the device again to see if the reset worked");
	probe_attempts = 0;
	while (probe_attempts < 5) {
		if (!probe_gps_ctrl (manager)) {
			probe_attempts++;
			g_debug ("Probe attempt %d failed. Trying again.", probe_attempts);
			sleep (3);
		} else {
			g_debug ("The recovery was successful! :)\n");
			priv->gps_enabled = gps_enabled_current;
			return TRUE;
		}
	}
    
    g_debug ("The recover didn't help! :(\n");
    g_debug ("Giving up...");
    return FALSE;
}

static void pm_prepare_for_sleep (MBMManager * manager)
{
	MBMManagerPrivate *priv = MBM_MANAGER_GET_PRIVATE (manager);

	if (mbm_options_debug ())
		g_debug ("Preparing to sleep\n");

	remove_supl_setup (manager);
	modem_disable_unsolicited_responses (manager);
    priv->registration_status = MBM_REGISTRATION_STATUS_NOT_REGISTERED;
	modem_disable_gps (manager);
	tcflush (priv->nmea_fd, TCIOFLUSH);
	tcflush (priv->ctrl_fd, TCIOFLUSH);
	FD_CLR (priv->nmea_fd, &priv->fdsread);
	FD_CLR (priv->ctrl_fd, &priv->fdsread);
	close (priv->nmea_fd);
	close (priv->ctrl_fd);
}

static void pm_wake_up (MBMManager * manager)
{
	MBMManagerPrivate *priv = MBM_MANAGER_GET_PRIVATE (manager);
	int gps_enabled_current;
	gps_enabled_current = priv->gps_enabled;

	priv->gps_enabled = GPS_WAKING_UP;
	if (mbm_options_debug ())
		g_debug ("Waking up in 5 seconds");
	sleep (10);

	/* set the ports */
	if (priv->nmea_dev)
		g_free (priv->nmea_dev);
	if (priv->ctrl_dev)
		g_free (priv->ctrl_dev);

	priv->nmea_dev = find_device_file (GPS_NMEA_CAP);
	priv->ctrl_dev = find_device_file (GPS_CTRL_CAP);

	init_mbm_modem (manager);

	priv->gps_enabled = gps_enabled_current;
}

gint main_loop (gpointer data)
{
	char buf[512];
	int len;
	int i, j;
	MBMManager *manager = (MBMManager *) data;
	MBMManagerPrivate *priv = MBM_MANAGER_GET_PRIVATE (manager);

	/* main loop
	 *
	 * check
	 *   notifier - have a client port been opened?
	 *   NMEA port - new NMEA sentences arrived from gps?
	 *   client ports - perhaps a client want to change interval?
	 *   control - hmmm... perhaps some AT unsolicited responses..
	 */
	int select_res;

	while (1) {
		/* Going down? Uninstall idle task if so */
		if (priv->system_terminating) {
			g_thread_exit (NULL);
			return FALSE;
		}

		if (priv->suspended) {
			if (priv->wait_for_sleep)
				continue;
			pm_prepare_for_sleep (manager);
			if (mbm_options_debug ())
				g_debug ("Entering suspend state");
			priv->wait_for_sleep = 1;
            dbus_g_method_return (priv->dbus_suspend_context);
			continue;
		} else if (priv->wait_for_sleep) {
			priv->wait_for_sleep = 0;
			pm_wake_up (manager);
			if (mbm_options_debug ())
				g_debug ("Resuming from suspend state");
		}

		modem_check_and_handle_gps_states (manager);

		/* Wait for any data before doing anything */
		priv->fdsreaduse = priv->fdsread;
		if (mbm_options_debug () > 2)
			g_debug ("MAIN: Waiting for select to return.\n");
		priv->timeout.tv_sec = 0;
		priv->timeout.tv_nsec = 200000000;
		select_res = pselect (priv->maxfd, &priv->fdsreaduse, NULL, NULL,
							  &priv->timeout, &priv->sigmask);
		if (select_res == -1) {
			g_debug ("select returns %m");
			/* maybe the files have been closed because we're going to sleep */
			/* just continue and the wait_for_sleep block will catch that case */
			continue;
		} else if (select_res == 0) {
			/* the timeout occured, just continue */
			if (mbm_options_debug () > 2)
				g_debug ("MAIN: Timeout on select\n");
			priv->fdsreaduse = priv->fdsread;
			continue;
		} else {
			/* we actually have some data, so break the while loop */
			if (mbm_options_debug () > 2)
				g_debug ("MAIN: Have some data from SELECT, continue.");
		}

		/* check the notifier (for open events on pty slave usage) */
		if (mbm_options_debug () > 2)
			g_debug ("MAIN: FD_ISSET on ifd\n");
		if (FD_ISSET (priv->ifd, &priv->fdsreaduse)) {
			struct inotify_event *ievent;
			if (mbm_options_debug () > 2)
				g_debug ("MAIN: FD_ISSET was set on ifd\n");
			memset (buf, 0, sizeof (buf));
			len = read (priv->ifd, buf, sizeof (buf));
			if (mbm_options_debug () > 1)
				g_debug ("notifier: <-- %d", len);
			i = 0;
			while (i < len) {
				ievent = (struct inotify_event *)&buf[i];
				if (ievent->mask & IN_OPEN) {
					if (mbm_options_debug () > 1)
						g_debug ("notifier: port %d opened", ievent->wd);
					/* only enable gps when first client attach */
					if (!priv->client_connections) {
						modem_enable_gps (manager, mbm_nmea_mode (),
										  mbm_nmea_interval ());
					}
					priv->client_connections++;
					if (mbm_options_debug ())
						g_debug ("client_connections %d",
								 priv->client_connections);
					for (j = 0; j < MAX_CLIENTS; j++) {
						if (priv->client[j].slave_ifd == ievent->wd) {
							if (mbm_options_debug ())
								g_debug ("The ievent->wd matches client %d.",
										 j);
							priv->client[j].active = 1;
						}
					}
				} else if ((ievent->mask & IN_CLOSE_WRITE) || (ievent->mask
															   &
															   IN_CLOSE_NOWRITE))
				{
					if (mbm_options_debug () > 1)
						g_debug ("notifier: port %d closed", ievent->wd);
				}
				if (mbm_options_debug () > 1)
					g_debug ("notifier: name %s\n", ievent->name);
				/* check for more events */
				i += sizeof (struct inotify_event) + ievent->len;
			}
		}

		/* check NMEA port */
		if (mbm_options_debug () > 2)
			g_debug ("MAIN: FD_ISSET on nmea_fd\n");
		if (FD_ISSET (priv->nmea_fd, &priv->fdsreaduse)) {
			if (mbm_options_debug () > 2)
				g_debug ("MAIN: FD_ISSET was true on nmea_fd.\n");
			memset (buf, 0, sizeof (buf));
			len = read (priv->nmea_fd, buf, sizeof (buf));
			if (mbm_options_debug () > 1)
				g_debug ("NMEA: <-- %d, \"%s\"", len, buf);

			if (len == 0) {
				if (mbm_options_debug ())
					g_debug ("NMEA read 0");
			} else if (len < 0) {
				if (mbm_options_debug ())
					g_debug ("Error: NMEA read %d", len);
			} else
				modem_parse_gps_nmea (manager, buf, len);
		}

		/* check client ports */
		for (i = 0; i < MAX_CLIENTS; i++) {
			if (mbm_options_debug () > 2)
				g_debug ("MAIN: FD_ISSET on master_fd on client %d\n", i);
			if (FD_ISSET (priv->client[i].master_fd, &priv->fdsreaduse)) {
				if (mbm_options_debug () > 2)
					g_debug
						("MAIN: FD_ISSET was set on master_fd on client %d\n",
						 i);
				memset (buf, 0, sizeof (buf));
				len = read (priv->client[i].master_fd, buf, sizeof (buf));
				if (mbm_options_debug ())
					g_debug ("Client: <-- %d, \"%s\"", len, buf);

                if (len < 0) {
					if (mbm_options_debug ())
						g_debug ("Error: client read %d, probably closed", len);
					/* shutdown client port */
					client_remove (manager, i);
					/* recreate client port */
					client_add (manager, i);
					/* adjust client accounts */
					if (priv->client_connections)
						priv->client_connections--;
					if (mbm_options_debug ())
						g_debug ("client_connections %d",
								 priv->client_connections);
					/* if no more clients, close gps */
					if (!priv->client_connections)
						modem_disable_gps (manager);
				}
                
                if (len > 0) {
					int written;
					if (mbm_options_debug ())
						g_debug ("Client: --> \"%s\"", buf);
					/* first echo back to client */
					written = write (priv->client[i].master_fd, buf, len);
					/* then check for a complete command */
					modem_parse_client (manager, i, buf, len);
				}
			}
		}

		/* check control port */
		if (mbm_options_debug () > 2)
			g_debug ("MAIN: FD_ISSET on ctrl_fd\n");
		if (FD_ISSET (priv->ctrl_fd, &priv->fdsreaduse)) {
			if (mbm_options_debug () > 2)
				g_debug ("MAIN: FD_ISSET was set on ctrl_fd\n");
			memset (buf, 0, sizeof (buf));
			len = read (priv->ctrl_fd, buf, sizeof (buf));
			if (mbm_options_debug ())
				g_debug ("control: <-- %d, \"%s\"", len, buf);
			if (len == 0) {
				if (mbm_options_debug ())
					g_debug ("control read 0");
			} else if (len < 0) {
				if (mbm_options_debug ()) {
					g_debug ("Error: control read %d", len);
					g_debug
						("The control device was lost. Will try to recover in 5 seconds.");
				}
				tcflush (priv->nmea_fd, TCIOFLUSH);
				tcflush (priv->ctrl_fd, TCIOFLUSH);
				FD_CLR (priv->nmea_fd, &priv->fdsread);
				FD_CLR (priv->ctrl_fd, &priv->fdsread);
				close (priv->nmea_fd);
				close (priv->ctrl_fd);

				pm_wake_up (manager);
			} else
				modem_parse_gps_ctrl (manager, buf, len);
		}
	}
}

void cleanup_func (MBMManager * manager)
{
	MBMManagerPrivate *priv = MBM_MANAGER_GET_PRIVATE (manager);

	remove_supl_setup (manager);
	modem_disable_unsolicited_responses (manager);
    priv->registration_status = MBM_REGISTRATION_STATUS_NOT_REGISTERED;
	modem_disable_gps (manager);
	tcflush (priv->nmea_fd, TCIOFLUSH);
	tcflush (priv->ctrl_fd, TCIOFLUSH);
	FD_CLR (priv->nmea_fd, &priv->fdsread);
	FD_CLR (priv->ctrl_fd, &priv->fdsread);
	close (priv->nmea_fd);
	close (priv->ctrl_fd);
}

/*
 * Gracefully shuts down and release all handles
 */
void mbm_manager_quit (MBMManager * manager)
{
	int i = 0;

	for (i = 0; i < MAX_CLIENTS; i++) {
		client_remove (manager, i);
	}
	cleanup_func (manager);
}

void _mbm_manager_init (MBMManager * manager)
{
	MBMManagerPrivate *priv = MBM_MANAGER_GET_PRIVATE (manager);
	GError *err = NULL;
	DBusError dbus_error;
	int i;

	priv->modem = NULL;

	priv->connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, &err);
	if (!priv->connection)
		g_error ("Could not connect to system bus.");

	dbus_g_connection_register_g_object ((DBusGConnection *) priv->connection,
										 MBM_DBUS_PATH, G_OBJECT (manager));

	dbus_error_init (&dbus_error);

	priv->ifd = inotify_init ();
	if (priv->ifd == -1) {
		g_error ("Open of inotify handle failed (%m)");
		exit (1);
	}
	FD_SET (priv->ifd, &priv->fdsread);
	priv->maxfd = priv->ifd + 1;

	/* client ports, these are interfaces towards host applications */
	for (i = 0; i < MAX_CLIENTS; i++) {
		client_add (manager, i);
	}

	priv->wait_for_sleep = 0;
	priv->unsolicited_responses_enabled = 0;
    priv->registration_status = MBM_REGISTRATION_STATUS_NOT_REGISTERED;

	priv->supl_setup_done = 0;
	priv->supl_account_idx = -1;

	priv->current_gps_state.w_disable = 0;
	priv->current_gps_state.cfun = 0;
	priv->current_gps_state.gps_status = 0;
	priv->current_gps_state.gps_enabled_mode = 0;
	priv->current_gps_state.supl_status = 0;
	priv->current_gps_state.gps_overheated = 0;
	priv->new_gps_state = priv->current_gps_state;
	priv->supl_failed = 0;

	priv->gps_port_not_defined = 1;

	priv->system_terminating = 0;

	priv->suspended = 0;

	create_initial_modems (manager);

}

static void mbm_manager_init (MBMManager * manager)
{
	_mbm_manager_init (manager);

	g_thread_create ((GThreadFunc) main_loop, manager, TRUE, NULL);
}

void _finalize (MBMManagerPrivate * priv)
{

	if (priv->connection)
		dbus_g_connection_unref ((DBusGConnection *) priv->connection);
	if (priv->nmea_dev)
		g_free (priv->nmea_dev);
	if (priv->ctrl_dev)
		g_free (priv->ctrl_dev);
}

static void finalize (GObject * object)
{
	MBMManagerPrivate *priv = MBM_MANAGER_GET_PRIVATE (object);

	_finalize (priv);

	G_OBJECT_CLASS (mbm_manager_parent_class)->finalize (object);
}

static void mbm_manager_class_init (MBMManagerClass * manager_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (manager_class);

	g_type_class_add_private (object_class, sizeof (MBMManagerPrivate));

	/* Virtual methods */
	object_class->finalize = finalize;

	/* Signals */
	signals[DEVICE_ADDED] =
		g_signal_new ("device-added", G_OBJECT_CLASS_TYPE (object_class),
					  G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET (MBMManagerClass,
														   device_added), NULL,
					  NULL, g_cclosure_marshal_VOID__STRING, G_TYPE_NONE, 1,
					  G_TYPE_STRING);

	signals[DEVICE_REMOVED] = g_signal_new ("device-removed",
											G_OBJECT_CLASS_TYPE (object_class),
											G_SIGNAL_RUN_FIRST,
											G_STRUCT_OFFSET (MBMManagerClass,
															 device_removed),
											NULL, NULL,
											g_cclosure_marshal_VOID__STRING,
											G_TYPE_NONE, 1, G_TYPE_STRING);

	dbus_g_object_type_install_info (G_TYPE_FROM_CLASS (manager_class),
									 &dbus_glib_mbm_manager_object_info);

	dbus_g_error_domain_register (MBM_MODEM_ERROR, "org.mbm.MbmGps.Modem",
								  MBM_TYPE_MODEM_ERROR);
}
