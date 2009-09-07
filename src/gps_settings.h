/*
 * gps_settings - Read/write the settings for the gps
 *
 * Copyright (C) 2008 Ericsson AB
 * Author: Torgny Johansson <torgny.johansson@ericsson.com>
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

#ifndef GPS_SETTINGS_H
#define GPS_SETTINGS_H

#include <stdio.h>
#include <stdlib.h>
#include <gmodule.h>
#include <libhal.h>
#include <glib/gtypes.h>
#include <glib-object.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "mbm_manager.h"

#define FIX_INTERVAL_STR "FIX_INTERVAL"
#define PREF_GPS_MODE_STR "PREF_GPS_MODE"
#define SUPL_APN_STR "SUPL_APN"
#define SUPL_AUTO_CONFIGURE_ADDRESS_STR  "SUPL_AUTO_CONFIGURE_ADDRESS"
#define SUPL_ADDRESS_STR  "SUPL_ADDRESS"
#define SUPL_USER_STR "SUPL_USER"
#define SUPL_PASSWORD_STR "SUPL_PASSWORD"
#define SUPL_ENABLE_NI_STR "SUPL_ENABLE_NI"

#define SIM_PIN_STR "SIM_PIN"
#define SIM_PUK_STR "SIM_PUK"

#define MAX_SETTINGS 100

extern void init_settings_table ();
extern void deinit_settings_table ();
extern int read_settings (FILE * fd);
extern int write_settings (FILE * fd);
extern int get_setting (char *key, char *value);
extern int change_setting (char *key, char *new_value);
extern void print_all_settings ();

extern int nmea_max_interval;
extern int nmea_min_interval;
extern int nmea_default_interval;

extern int debug;

void init_settings_table (void);
void deinit_settings_table (void);
void print_all_settings (void);

void load_settings (void);
void save_settings (void);

void quit (void);

int set_nmea_interval (int interval);

typedef enum {
	OFF = 0, STAND_ALONE_MODE = 1, SUPL_MODE = 3,
	GPS_WAKING_UP = 10, GPS_RECOVERING = 11, SUPL_TRYING_MODE =
	30, SUPL_WAITING_FOR_REGISTRATION = 31, STAND_ALONE_MODE_SUPL_FAILED = 40,
} EGPS_MODE;

void
impl_mbm_set_fix_interval (MBMManager * gpsd, int interval,
						   DBusGMethodInvocation * context);
void
impl_mbm_set_single_shot (MBMManager * gpsd, int single_shot,
						  DBusGMethodInvocation * context);
void
impl_mbm_set_pref_gps_mode (MBMManager * gpsd, int mode,
							DBusGMethodInvocation * context);
void
impl_mbm_set_supl_apn (MBMManager * gpsd, char *supl_apn_new,
					   DBusGMethodInvocation * context);
void
impl_mbm_set_supl_auto_configure_address (MBMManager * gpsd,
										  gint auto_configure,
										  DBusGMethodInvocation * context);
void impl_mbm_set_supl_address (MBMManager * gpsd, char *supl_address_new,
								DBusGMethodInvocation * context);
void impl_mbm_set_supl_user (MBMManager * gpsd, char *supl_user_new,
							 DBusGMethodInvocation * context);
void impl_mbm_set_supl_password (MBMManager * gpsd, char *supl_password_new,
								 DBusGMethodInvocation * context);
void impl_mbm_set_supl_enable_ni (MBMManager * gpsd, int supl_enable_ni_new,
								  DBusGMethodInvocation * context);
void impl_mbm_supl_install_certificate (MBMManager * gpsd, char *file_path,
										DBusGMethodInvocation * context);
void impl_mbm_get_supl_install_certificate_status (MBMManager * gpsd,
												   DBusGMethodInvocation *
												   context);

void
impl_mbm_get_gps_customization (MBMManager * gpsd,
								DBusGMethodInvocation * context);
void
impl_mbm_get_gps_supl_customization (MBMManager * gpsd,
										  DBusGMethodInvocation * context);

void
impl_mbm_get_fix_interval (MBMManager * gpsd, DBusGMethodInvocation * context);
void
impl_mbm_get_single_shot (MBMManager * gpsd, DBusGMethodInvocation * context);
void
impl_mbm_get_pref_gps_mode (MBMManager * gpsd, DBusGMethodInvocation * context);
void
impl_mbm_get_time_without_fix (MBMManager * gpsd, DBusGMethodInvocation * context);
void
impl_mbm_get_supl_apn (MBMManager * gpsd, DBusGMethodInvocation * context);
void
impl_mbm_get_supl_auto_configure_address (MBMManager * gpsd,
										  DBusGMethodInvocation * context);
void
impl_mbm_get_supl_address (MBMManager * gpsd, DBusGMethodInvocation * context);
void
impl_mbm_get_supl_user (MBMManager * gpsd, DBusGMethodInvocation * context);
void
impl_mbm_get_supl_password (MBMManager * gpsd, DBusGMethodInvocation * context);
void
impl_mbm_get_supl_enable_ni (MBMManager * gpsd,
							 DBusGMethodInvocation * context);
void impl_mbm_get_gps_enabled (MBMManager * gpsd,
							   DBusGMethodInvocation * context);
void impl_mbm_set_gps_enabled (MBMManager * gpsd, int enabled,
							   DBusGMethodInvocation * context);
void impl_mbm_get_enabled_gps_mode (MBMManager * gpsd,
									DBusGMethodInvocation * context);
void impl_mbm_set_suspend (MBMManager * gpsd, int suspended,
						   DBusGMethodInvocation * context);
void impl_mbm_get_suspend (MBMManager * gpsd, DBusGMethodInvocation * context);

#endif
