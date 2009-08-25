/*
 * gps_settings - Read/write the settings for the gps
 *
 * Copyright (C) 2008 Ericsson AB
 * Author: Torgny Johansson <torgny.johansson@ericsson.com>
 *         Bjorn Runaker <bjorn.runaker@ericsson.com>
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
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "mbm_errors.h"
#include "gps_settings.h"
#include "mbm_gpsd_type.h"
#include "mbm_options.h"
#include "mbm_manager.h"
#include "mbm_modem.h"

int nmea_max_interval = 60;
int nmea_min_interval = 1;
int nmea_default_interval = 1;

struct setting {
	char key[512];
	char value[512];
};

static struct setting **settings;
static int nr_of_entries;

void init_settings_table ()
{
	nr_of_entries = 0;
	settings =
		(struct setting **)malloc (sizeof (struct setting *) * MAX_SETTINGS);
	memset (settings, 0, sizeof (struct setting *) * MAX_SETTINGS);
}

void deinit_settings_table ()
{
	int i;
	for (i = 0; i < nr_of_entries; i++) {
		free (settings[i]);
	}
	free (settings);
}

/*
 * Trim leading and trailing whitespaces
 * The string pointed to by str will be modified,
 * but only on the right side so the returned pointer
 * should be used.
 */
static char *trim (char *str)
{
	int len;

	//trim left side from spaces and tabs
	while (str[0] == ' ' || str[0] == '\t') {
		str++;
	}

	//trim right side from spaces and tabs
	len = strlen (str);
	while (str[len - 1] == ' ' || str[len - 1] == '\t') {
		str[len - 1] = 0;
		len = strlen (str);
	}
	return str;
}

/*
 * Parse a line read from the config file.
 * The result is placed in the k and v buffers.
 * If the line represents a valid setting 1 is returned, otherwise 0 is returned
 */
static int parse_line (char *line, char *k, char *v)
{
	int idx;
	char *tmp = strstr (line, "=");
	if (tmp == NULL) {
		return 0;
	}
	idx = tmp - line;
	strcpy (k, trim (strndup (line, idx)));
	strcpy (v, trim (strndup (line + idx + 1, strlen (line) - idx - 1)));
	return 1;
}

/*
 * Read the given file. Lookup and store any valid settings in the settings array.
 * Returns the number of settings found.
 */
extern int read_settings (FILE * fd)
{
	char *trim_line;
	char line[LINE_MAX];
	int len;

	while (fgets (line, LINE_MAX, fd) != NULL) {
		//remove newline
		len = strlen (line);
		if (line[len - 1] == '\n')
			line[len - 1] = 0;
		//printf("Read:%s.\n", line);

		trim_line = trim (line);
		//check if the line is a comment
		if (trim_line[0] == '#') {
			//printf("Found a comment. Skipping\n");
			continue;
		}
		//check if the line is long enough to be a valid setting, e.g. a=b
		if (strlen (trim_line) < 3) {
			//printf("Found a line which is too short to be a setting. Skipping.\n");
			continue;
		}

		settings[nr_of_entries] = malloc (sizeof (struct setting));
		memset (settings[nr_of_entries]->key, 0,
				sizeof (settings[nr_of_entries]->key));
		if (!parse_line (trim_line, settings[nr_of_entries]->key,
						 settings[nr_of_entries]->value)) {
			//printf("Malformed setting. Removing.\n");
			free (settings[nr_of_entries]);
			continue;
		}
		nr_of_entries++;
	}
	return nr_of_entries;
}

/*
 * Write settings to the given file
 */
extern int write_settings (FILE * fd)
{
	int i = 0;
	fprintf (fd, "# mbm-gpsd.conf\n");
	fprintf (fd, "# This is the configuration file for the mbm_gpsd daemon.\n");
	fprintf (fd, "# It's possible to change the values in this file\n");
	fprintf (fd, "# but any invalid settings or comments are discarded.\n");
	fprintf (fd, "#\n");
	fprintf (fd, "# Allowed settings and values:\n");
	fprintf (fd, "#\n");
	fprintf (fd, "#\tFIX_INTERVAL=<interval>\n");
	fprintf (fd, "#\tAllowed values are 1-60.\n");
	fprintf (fd, "#\n");
	fprintf (fd, "#\tPREF_GPS_MODE=<mode>\n");
	fprintf (fd, "#\tAllowed values are:\n");
	fprintf (fd, "#\t\tSTAND_ALONE\n");
	fprintf (fd, "#\t\tSUPL\n");
	fprintf (fd, "#\n");
	fprintf (fd, "#\tSUPL_APN=<apn>\n");
	fprintf (fd, "#\tThe APN to be used with SUPL.\n");
	fprintf (fd, "#\n");
	fprintf (fd, "#\tSUPL_AUTO_CONFIGURE_ADDRESS=<auto configure>\n");
	fprintf (fd, "#\tAllowed values are:\n");
	fprintf (fd, "#\t\t1 - try to auto configure address\n");
	fprintf (fd, "#\t\t0 - don't try to auto configure address\n");
	fprintf (fd, "#\n");
	fprintf (fd, "#\tSUPL_ADDRESS=<supl server address>\n");
	fprintf (fd, "#\tThe address to the supl server.\n");
	fprintf (fd, "#\n");
	fprintf (fd, "#\tSUPL_USER=<username>\n");
	fprintf (fd, "#\tThe username to be used with SUPL.\n");
	fprintf (fd, "#\n");
	fprintf (fd, "#\tSUPL_PASSWORD=<password>\n");
	fprintf (fd, "#\tThe password to be used with SUPL.\n");
	fprintf (fd, "#\n");
	fprintf (fd, "#\tSUPL_ENABLE_NI=<enable_ni>\n");
	fprintf (fd, "#\tAllowed values are:\n");
	fprintf (fd, "#\t\t1 - allow network initiated requests.\n");
	fprintf (fd, "#\t\t0 - do not allow network initiated requests.\n");
	fprintf (fd, "#\n");
	fprintf (fd, "#\tSIM_PIN=<pin code>\n");
	fprintf (fd, "#\tThe pin code of the sim card\n");
	fprintf (fd, "#\n");
	fprintf (fd, "#\tSIM_PUK=<puk code>\n");
	fprintf (fd, "#\tThe puk code of the sim card\n");
	fprintf (fd, "#\n");

	for (i = 0; i < nr_of_entries; i++) {
		fprintf (fd, "%s=%s\n", settings[i]->key, settings[i]->value);
	}
	return 1;
}

/*
 * Searches for the given key and places it's value in the value buffer if found.
 * Returns 1 if found, 0 otherwise.
 */
extern int get_setting (char *key, char *value)
{
	int i = 0;
	for (i = 0; i < nr_of_entries; i++) {
		if (!strcmp (key, settings[i]->key)) {
			//printf("Found a matching key!\n");
			strcpy (value, settings[i]->value);
			return 1;
		}
	}
	printf ("No matching key found\n");
	return 0;
}

/*
 * Change the given setting to the new value supplied
 * Returns 1 if successful and 0 otherwise
 */
extern int change_setting (char *key, char *new_value)
{
	int i = 0;
	for (i = 0; i < nr_of_entries; i++) {
		if (!strcmp (key, settings[i]->key)) {
			//printf("Found a matching key!\n");
			strcpy (settings[i]->value, new_value);
			return 1;
		}
	}
	printf ("No matching key found\n");
	return 0;
}

/*
 * Print all settings to stdout
 */
void print_all_settings ()
{
	int i = 0;
	printf ("Printing all settings\n");
	for (i = 0; i < nr_of_entries; i++) {
		printf ("Key: %s, Value: %s.\n", settings[i]->key, settings[i]->value);
	}
}

/*
 * Set the nmea interval to the given value.
 * If the value is out of range eithe NMEA_MAX_INTERVAL or NMEA_MIN_INTERVAL is set
 * The setting is not activated until the next enable_gps call
 * Returns the value that has been set
 */
int set_nmea_interval (int interval)
{
	if (interval < nmea_min_interval) {
		if (mbm_options_debug ())
			g_debug
				("Value of interval is too small. Using minimum value of %d",
				 nmea_min_interval);
		interval = nmea_min_interval;
	} else if (interval > nmea_max_interval) {
		if (mbm_options_debug ())
			g_debug
				("Value of interval is too large. Using maximum value of %d",
				 nmea_max_interval);
		interval = nmea_max_interval;
	} else {
		if (mbm_options_debug ())
			g_debug ("Interval is %d", interval);
	}
	mbm_set_nmea_interval (interval);
	return mbm_nmea_interval ();
}

/*
 * Load settings from config file
 * Settings can be overidden with cmdline arguments
 */
void load_settings ()
{
	FILE *fd;
	char value[512];
	int interval = nmea_default_interval;
	if (mbm_options_debug ())
		g_debug ("Loading settings from config file.\n");
	if ((fd = fopen (MBM_SETTING_FPATH, "r")) == NULL) {
		if (mbm_options_debug ())
			g_debug ("Unable to open ordinary config file.\n");
		if ((fd = fopen (MBM_SETTING_FPATH_FALLBACK, "r")) == NULL) {
			if (mbm_options_debug ())
				g_debug ("Unable to open fallback config file.\n");
			return;
		} else
			g_debug ("Reading config from %s", MBM_SETTING_FPATH_FALLBACK);
	} else
		g_debug ("Reading config from %s", MBM_SETTING_FPATH);

	read_settings (fd);
	fclose (fd);

	if (mbm_options_debug ())
		g_debug ("Looking for key FIX_INTERVAL\n");
	if (get_setting (FIX_INTERVAL_STR, value)) {
		if (mbm_options_debug ())
			g_debug ("The value of key FIX_INTERVAL is %s\n", value);
		interval = atoi (value);
	}
	set_nmea_interval (interval);

	if (mbm_options_debug ())
		g_debug ("Looking for key PREF_GPS_MODE\n");
	if (get_setting (PREF_GPS_MODE_STR, value)) {
		if (!strcmp (value, "STAND_ALONE")) {
			if (mbm_options_debug ())
				g_debug ("Using STAND_ALONE mode.\n");
			mbm_set_nmea_mode (STAND_ALONE_MODE);
		} else if (!strcmp (value, "SUPL")) {
			if (mbm_options_debug ())
				g_debug ("Using SUPL mode.\n");
			mbm_set_nmea_mode (SUPL_MODE);
		}
	} else {
		//go stand alone
		if (mbm_options_debug ())
			g_debug ("Unable to read mode setting. Using STAND_ALONE.\n");
		mbm_set_nmea_mode (STAND_ALONE_MODE);
	}

	if (mbm_options_debug ())
		g_debug ("Looking for key SUPL_APN\n");
	if (get_setting (SUPL_APN_STR, value)) {
		if (mbm_options_debug ())
			g_debug ("Using supl apn: %s\n", value);
		mbm_set_supl_apn (value);
	} else {
		if (mbm_options_debug ())
			g_debug ("Unable to read supl apn setting.\n");
		mbm_set_supl_apn ("");
	}

	if (mbm_options_debug ())
		g_debug ("Looking for key SUPL_AUTO_CONFIGURE_ADDRESS\n");
	if (get_setting (SUPL_AUTO_CONFIGURE_ADDRESS_STR, value)) {
		if (mbm_options_debug ())
			g_debug ("Using supl auto configure address: %s\n", value);
		mbm_set_supl_auto_configure_address (atoi (value));
	} else {
		if (mbm_options_debug ())
			g_debug ("Unable to read supl auto configure address setting.\n");
		mbm_set_supl_auto_configure_address (0);
	}

	if (mbm_options_debug ())
		g_debug ("Looking for key SUPL_ADDRESS\n");
	if (get_setting (SUPL_ADDRESS_STR, value)) {
		if (mbm_options_debug ())
			g_debug ("Using supl address: %s\n", value);
		mbm_set_supl_address (value);
	} else {
		if (mbm_options_debug ())
			g_debug ("Unable to read supl address setting.\n");
		mbm_set_supl_address ("");
	}

	if (mbm_options_debug ())
		g_debug ("Looking for key SUPL_USER\n");
	if (get_setting (SUPL_USER_STR, value)) {
		if (mbm_options_debug ())
			g_debug ("Using supl user: %s\n", value);
		if (value == NULL)
			mbm_set_supl_user ("");
		else
			mbm_set_supl_user (value);
	} else {
		if (mbm_options_debug ())
			g_debug ("Unable to read supl user setting.\n");
		mbm_set_supl_user ("");
	}

	if (mbm_options_debug ())
		g_debug ("Looking for key SUPL_PASSWORD\n");
	if (get_setting (SUPL_PASSWORD_STR, value)) {
		if (mbm_options_debug ())
			g_debug ("Using supl password: <secret>.\n");
		if (value == NULL)
			mbm_set_supl_password ("");
		else
			mbm_set_supl_password (value);
	} else {
		if (mbm_options_debug ())
			g_debug ("Unable to read supl password setting.\n");
		mbm_set_supl_password ("");
	}

	if (mbm_options_debug ())
		g_debug ("Looking for key SUPL_ENABLE_NI\n");
	if (get_setting (SUPL_ENABLE_NI_STR, value)) {
		if (mbm_options_debug ())
			g_debug ("The value of key SUPL_ENABLE_NI is %s\n", value);
		mbm_set_supl_enable_ni (atoi (value));
	} else {
		if (mbm_options_debug ())
			g_debug ("Unable to read supl enable ni setting.\n");
		mbm_set_supl_enable_ni (1);
	}

	if (mbm_options_debug ())
		g_debug ("Looking for key SIM_PIN\n");
	if (get_setting (SIM_PIN_STR, value)) {
		if (mbm_options_debug ())
			g_debug ("The value of key SIM_PIN is secret...\n");
		mbm_set_pin_code (value);
	} else {
		if (mbm_options_debug ())
			g_debug ("Unable to read sim pin setting.\n");
		mbm_set_pin_code ("");
	}

	if (mbm_options_debug ())
		g_debug ("Looking for key SIM_PUK\n");
	if (get_setting (SIM_PUK_STR, value)) {
		if (mbm_options_debug ())
			g_debug ("The value of key SIM_PUK is secret...\n");
		mbm_set_puk_code (value);
	} else {
		if (mbm_options_debug ())
			g_debug ("Unable to read sim puk setting.\n");
		mbm_set_puk_code ("");
	}

	return;
}

void save_settings ()
{
	FILE *fd;
	if (mbm_options_debug ())
		g_debug ("Saving settings to file.\n");
	if ((fd = fopen (MBM_SETTING_FPATH, "w")) == NULL) {
		if (mbm_options_debug ())
			g_debug ("Unable to open ordinary config file for writing.\n");
		if ((fd = fopen (MBM_SETTING_FPATH_FALLBACK, "w")) == NULL) {
			if (mbm_options_debug ())
				g_debug ("Unable to open fallback config file for writing.\n");
		} else
			g_debug ("Saving config to %s", MBM_SETTING_FPATH_FALLBACK);
		return;
	} else
		g_debug ("Saving config to %s", MBM_SETTING_FPATH);
	if (write_settings (fd)) {
		if (mbm_options_debug ())
			g_debug ("Settings saved succesfully.\n");
	}
	fclose (fd);
}

void
impl_mbm_set_fix_interval (MBMManager * gpsd, int interval,
						   DBusGMethodInvocation * context)
{
	char buf[20];
	g_debug ("impl_mbm_set_fix_interval interval=%d", interval);
	sprintf (buf, "%d", interval);
	if (change_setting (FIX_INTERVAL_STR, buf)) {
		mbm_set_nmea_interval (interval);
	}
	dbus_g_method_return (context);
}

void
impl_mbm_set_single_shot (MBMManager * gpsd, int single_shot,
						  DBusGMethodInvocation * context)
{
	g_debug ("impl_mbm_set_single_shot=%d", single_shot);
	mbm_set_single_shot (single_shot);
	dbus_g_method_return (context);
}

void
impl_mbm_set_pref_gps_mode (MBMManager * gpsd, int mode,
							DBusGMethodInvocation * context)
{
	char buf[20];
	switch (mode) {
	case STAND_ALONE_MODE:
		sprintf (buf, "%s", "STAND_ALONE");
		break;
	case SUPL_MODE:
		sprintf (buf, "%s", "SUPL");
		break;
	default:
		g_debug ("Invalid gps mode. Not changing it.\n");
		break;
	}
	if (change_setting (PREF_GPS_MODE_STR, buf)) {
		mbm_set_nmea_mode (mode);
	}
	dbus_g_method_return (context);
	g_debug ("gps mode %d", mode);
}

void
impl_mbm_set_supl_apn (MBMManager * gpsd, char *supl_apn_new,
					   DBusGMethodInvocation * context)
{
	if (change_setting (SUPL_APN_STR, supl_apn_new)) {
		mbm_set_supl_apn (supl_apn_new);
	}
	dbus_g_method_return (context);
}

void
impl_mbm_set_supl_auto_configure_address (MBMManager * gpsd,
										  gint auto_configure,
										  DBusGMethodInvocation * context)
{
	char buf[2];
	sprintf (buf, "%d", auto_configure);
	if (change_setting (SUPL_AUTO_CONFIGURE_ADDRESS_STR, buf)) {
		mbm_set_supl_auto_configure_address (auto_configure);
	}
	dbus_g_method_return (context);
}

void
impl_mbm_set_supl_address (MBMManager * gpsd, char *supl_address_new,
						   DBusGMethodInvocation * context)
{
	if (change_setting (SUPL_ADDRESS_STR, supl_address_new)) {
		mbm_set_supl_address (supl_address_new);
	}
	dbus_g_method_return (context);
}

void
impl_mbm_set_supl_user (MBMManager * gpsd, char *supl_user_new,
						DBusGMethodInvocation * context)
{
	if (change_setting (SUPL_USER_STR, supl_user_new)) {
		mbm_set_supl_user (supl_user_new);
	}
	dbus_g_method_return (context);
}

void
impl_mbm_set_supl_password (MBMManager * gpsd, char *supl_password_new,
							DBusGMethodInvocation * context)
{
	if (change_setting (SUPL_PASSWORD_STR, supl_password_new)) {
		mbm_set_supl_password (supl_password_new);
	}
	dbus_g_method_return (context);
}

void
impl_mbm_set_supl_enable_ni (MBMManager * gpsd, int supl_enable_ni_new,
							 DBusGMethodInvocation * context)
{
	char buf[20];
	sprintf (buf, "%d", supl_enable_ni_new);
	if (change_setting (SUPL_ENABLE_NI_STR, buf)) {
		mbm_set_supl_enable_ni (supl_enable_ni_new);
	}
	dbus_g_method_return (context);
}

void
impl_mbm_supl_install_certificate (MBMManager * gpsd, char *file_path,
								   DBusGMethodInvocation * context)
{
	modem_install_supl_certificate (gpsd, strdup (file_path));
	dbus_g_method_return (context);
}

void
impl_mbm_get_supl_install_certificate_status (MBMManager * gpsd,
											  DBusGMethodInvocation * context)
{
	MBMManagerPrivate *priv = MBM_MANAGER_GET_PRIVATE (gpsd);

	dbus_g_method_return (context, priv->install_supl_cert_status);
}

void
impl_mbm_get_gps_customization (MBMManager * gpsd,
								DBusGMethodInvocation * context)
{
	dbus_g_method_return (context, mbm_gps_customization (STAND_ALONE_MODE));
}

void
impl_mbm_get_gps_supl_customization (MBMManager * gpsd,
									 DBusGMethodInvocation * context)
{
	dbus_g_method_return (context, mbm_gps_customization (SUPL_MODE));
}

void
impl_mbm_get_fix_interval (MBMManager * gpsd, DBusGMethodInvocation * context)
{
	dbus_g_method_return (context, mbm_nmea_interval ());
}

void
impl_mbm_get_single_shot (MBMManager * gpsd, DBusGMethodInvocation * context)
{
	dbus_g_method_return (context, mbm_single_shot ());
}

void
impl_mbm_get_pref_gps_mode (MBMManager * gpsd, DBusGMethodInvocation * context)
{
	dbus_g_method_return (context, mbm_nmea_mode ());
}

void
impl_mbm_get_time_without_fix (MBMManager * gpsd, DBusGMethodInvocation * context)
{
	time_t rawtime;
	struct tm *current_time;
	int diff;
	MBMManagerPrivate *priv = MBM_MANAGER_GET_PRIVATE (gpsd);

	time (&rawtime);
	
	if (!priv->gps_fix_obtained && priv->gps_enabled) {
		current_time = localtime (&rawtime);
		diff = (int) difftime (mktime (current_time), priv->gps_start_time);
		dbus_g_method_return (context, diff);
	} else {
		priv->gps_start_time = mktime (localtime (&rawtime));
		dbus_g_method_return (context, 0);
	}
}


void impl_mbm_get_supl_apn (MBMManager * gpsd, DBusGMethodInvocation * context)
{
	dbus_g_method_return (context, g_strdup (mbm_supl_apn ()));
}

void
impl_mbm_get_supl_auto_configure_address (MBMManager * gpsd,
										  DBusGMethodInvocation * context)
{
	dbus_g_method_return (context, mbm_supl_auto_configure_address ());
}

void
impl_mbm_get_supl_address (MBMManager * gpsd, DBusGMethodInvocation * context)
{
	dbus_g_method_return (context, g_strdup (mbm_supl_address ()));
}

void impl_mbm_get_supl_user (MBMManager * gpsd, DBusGMethodInvocation * context)
{

	dbus_g_method_return (context, g_strdup (mbm_supl_user ()));
}

void
impl_mbm_get_supl_password (MBMManager * gpsd, DBusGMethodInvocation * context)
{

	dbus_g_method_return (context, g_strdup (mbm_supl_password ()));
}

void
impl_mbm_get_supl_enable_ni (MBMManager * gpsd, DBusGMethodInvocation * context)
{

	dbus_g_method_return (context, mbm_supl_enable_ni ());

}

void
impl_mbm_get_gps_enabled (MBMManager * gpsd, DBusGMethodInvocation * context)
{
	MBMManagerPrivate *priv = MBM_MANAGER_GET_PRIVATE (gpsd);

	dbus_g_method_return (context, priv->gps_enabled);
}

void
impl_mbm_set_gps_enabled (MBMManager * gpsd, int enabled,
						  DBusGMethodInvocation * context)
{
	if (enabled)
		mbm_set_user_disabled_gps (0);
	else {
		mbm_set_user_disabled_gps (1);
	}
	dbus_g_method_return (context);
}

void
impl_mbm_get_enabled_gps_mode (MBMManager * gpsd,
							   DBusGMethodInvocation * context)
{
	MBMManagerPrivate *priv = MBM_MANAGER_GET_PRIVATE (gpsd);

	if (priv->supl_failed)
		dbus_g_method_return (context, STAND_ALONE_MODE_SUPL_FAILED);
	else
		dbus_g_method_return (context,
							  priv->current_gps_state.gps_enabled_mode);
}

void
impl_mbm_set_suspend (MBMManager * gpsd, int suspended,
					  DBusGMethodInvocation * context)
{
	MBMManagerPrivate *priv = MBM_MANAGER_GET_PRIVATE (gpsd);
	if (suspended) {
		priv->suspended = 1;
		/* don't send reply here, instead send reply after preparing for suspend / waking up */
		priv->dbus_suspend_context = context;
	} else {
		priv->suspended = 0;
		dbus_g_method_return (context);
	}
}

void impl_mbm_get_suspend (MBMManager * gpsd, DBusGMethodInvocation * context)
{
	MBMManagerPrivate *priv = MBM_MANAGER_GET_PRIVATE (gpsd);

	dbus_g_method_return (context, priv->suspended);
}

/*
 int main(int argc, char *argv[])
 {
 FILE *fd;
 if((fd = fopen("mbm-gpsd.conf", "r")) == NULL){
 printf("Unable to open file. Exiting.\n");
 return 1;
 }
 init_table();
 read_settings(fd);
 fclose(fd);
 print_all_settings();
 printf("Looking for key apn\n");
 char value[512];
 if(get_setting("SUPL_APN", value)){
 printf("The value of key apn is %s\n", value);
 }
 deinit_table();
 return 0;
 }
 */
