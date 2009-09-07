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
#ifndef MBM_MANAGER_H
#define MBM_MANAGER_H

#include <glib/gtypes.h>
#include <glib-object.h>
#include <sys/types.h>
#include <time.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>

#define MBM_TYPE_MANAGER            (mbm_manager_get_type ())
#define MBM_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MBM_TYPE_MANAGER, MBMManager))
#define MBM_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MBM_TYPE_MANAGER, MBMManagerClass))
#define MBM_IS_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MBM_TYPE_MANAGER))
#define MBM_IS_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), MBM_TYPE_MANAGER))
#define MBM_MANAGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MBM_TYPE_MANAGER, MBMManagerClass))

#define MBM_DBUS_SERVICE "org.mbm.MbmGps"
#define MBM_DBUS_PATH "/org/mbm/Gps"

#define MBM_SETTING_FPATH "/etc/mbm-gpsd.conf"
// Bjorn: If can't use the standard conf file use an alternative. This should be "~/.mbm-gpsd.conf"
#define MBM_SETTING_FPATH_FALLBACK "./mbm-gpsd.conf"

typedef struct {
	GObject parent;
} MBMManager;

typedef struct {
	GObjectClass parent;

	/* Signals */
	void (*device_added) (MBMManager * manager, const char *device_udi);
	void (*device_removed) (MBMManager * manager, const char *device_udi);
} MBMManagerClass;

typedef void (*MBMManagerFn) (MBMManager * modem,
							  GError * error, gpointer user_data);

typedef void (*MBMManagerUIntFn) (MBMManager * modem,
								  guint32 result,
								  GError * error, gpointer user_data);

typedef void (*MBMManagerStringFn) (MBMManager * modem,
									const char *result,
									GError * error, gpointer user_data);

GType mbm_manager_get_type (void);

MBMManager *mbm_manager_new (void);

#define CLIENT_DEV  "/dev/gps"
#define MAX_CLIENTS 3
#define MAX_RESPONSE 2048

#define MAX_IMSI_LEN 20
#define MAX_IMEI_LEN 20

// MBMClient
typedef struct {
	char *slave_dev;
	char *at_cmd;
	int master_fd;
	int slave_ifd;
	int active;
} MBMClient;

// MBMGpsState
typedef struct {
	int w_disable;
	int cfun;
	int gps_status;
	int gps_enabled_mode;
	int supl_status;
	int gps_overheated;
} MBMGpsState;

//Location data
typedef struct {
	int fix;
	char longitude_hemi;
	char latitude_hemi;
	gchar latitude[20];
	gchar longitude[20];
	gchar altitude[20];
	gchar horiz_dilution[20];
	gchar vert_dilution[20];
} MBMLocationData;

#define SERVICE_RAT_MASK    0xF0
#define SERVICE_RAT_2G      0x10
#define SERVICE_RAT_3G      0x20

typedef enum {
	SERVICE_UNKNOWN = 0,		///< Unknown service type
	SERVICE_GPRS = 0x11,		///< 2G service: GPRS
	SERVICE_EGPRS = 0x12,		///< 2G service: EGPRS
	SERVICE_UMTS = 0x21,		///< 3G service: UMTS
	SERVICE_HSDPA = 0x22		///< 3G service: HSDPA
} SERVICE_TYPE;

typedef struct {
	void *connection;
	void *hal_ctx;
	gchar *nmea_dev;
	gchar *ctrl_dev;
	gchar *nmea_udi;
	gchar *ctrl_udi;
	gchar *driver;
	gint connected;
    int registration_status;

	void *modem;

	int ifd;
	int maxfd;
	fd_set fdsread;
	fd_set fdsreaduse;

	int nmea_fd;
	int ctrl_fd;

	int wait_for_sleep;
	struct timespec timeout;
	sigset_t sigmask;

    DBusGMethodInvocation *dbus_suspend_context;

	/* clients on pseudo tty ports (via symlink...) */
	MBMClient client[MAX_CLIENTS];
	int client_connections;

	int unsolicited_responses_enabled;

	int gps_port_not_defined;

	int supl_setup_done;
	int supl_account_idx;
	int supl_failed;

	int gps_enabled;
	MBMGpsState current_gps_state;
	MBMGpsState new_gps_state;
	MBMLocationData location_data;
	int system_terminating;

	int suspended;

	int install_supl_cert;
	char *certificate_file_path;
	int install_supl_cert_status;

    time_t gps_start_time;
    int gps_fix_obtained;
} MBMManagerPrivate;

#define MBM_MANAGER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), MBM_TYPE_MANAGER, MBMManagerPrivate))

gint main_loop (gpointer data);

void mbm_manager_quit (MBMManager * manager);

void cleanup_func (MBMManager * manager);

void _mbm_manager_init (MBMManager * manager);

void _finalize (MBMManagerPrivate * priv);

#endif /* MM_MANAGER_H */
