/*
 * mbm_supl.c - supl functions
 *
 * Copyright (C) 2009 Ericsson AB
 *
 * Authors: Torgny Johansson <torgny.johansson@ericsson.com>
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
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <time.h>
#include "mbm_manager.h"
#include "mbm_supl.h"
#include "mbm_errors.h"
#include "mbm_serial.h"
#include "mbm_options.h"
#include "mbm_callback_info.h"
#include "gps_settings.h"

void supl_get_current_date_string (char *date_string)
{
	time_t rawtime;
	struct tm *timeinfo;
	int diff;
	char time_string[35];
	char diff_string[4];

	time (&rawtime);
	timeinfo = localtime (&rawtime);
	diff = ((timeinfo->tm_gmtoff) / 3600);
	strftime (time_string, 30, "%Y/%m/%d,%H:%M:%S", timeinfo);

	if (diff >= 0)
		sprintf (diff_string, "+%.2d", diff * 4);
	else
		sprintf (diff_string, "%.2d", diff * 4);

	strcat (time_string, diff_string);
	strcpy (date_string, time_string);
}

/*
 * Removes the account created by setup_supl
 * Returns 1 if successful, 0 otherwise (currently will always return 1 no matter what)
 */
int remove_supl_setup (MBMManager * manager)
{
	MBMManagerPrivate *priv = MBM_MANAGER_GET_PRIVATE (manager);

	if (mbm_options_debug ())
		g_debug ("Removing SUPL setup.\n");

	if (priv->supl_account_idx != -1) {
		char buf[32];
		int len = sprintf (buf, "AT*E2GPSSUPLNI=0\r\n");
		char *response = serial_send_AT_cmd (manager, buf, len);
		if (mbm_options_debug ())
			g_debug ("Disabled supl ni unsolicited responses.\n");

		free (response);
		len = sprintf (buf, "AT*EIAD=%d,1\r\n", priv->supl_account_idx);
		response = serial_send_AT_cmd (manager, buf, len);
		if (response && strstr (response, "OK")) {
			if (mbm_options_debug ())
				g_debug ("SUPL account successfully removed.\n");
			priv->supl_account_idx = -1;
			priv->supl_setup_done = 0;
			free (response);
			return 1;
		} else {
			if (mbm_options_debug ())
				g_debug
					("Unable to remove SUPL account. It's assumed to be faulty.\n");
			priv->supl_account_idx = -1;
			priv->supl_setup_done = 0;
			free (response);
			return 1;
		}

	} else {
		if (mbm_options_debug ())
			g_debug ("SUPL has not yet been setup so nothing is removed\n");
		return 1;
	}
}

static gboolean supl_setup_clock (MBMManager *manager)
{
	int len;
	char buf[512];
	char *response;
	char date[35];

	supl_get_current_date_string (date);
	len = sprintf (buf, "AT+CCLK=\"%s\"\r\n", date);
	response = serial_send_AT_cmd (manager, buf, len);
	if (response && strstr (response, "OK")) {
		free (response);
		if (mbm_options_debug ())
			g_debug ("Clock setup: %s\n", date);
		return TRUE;
	} 

	if (mbm_options_debug ())
		g_debug ("Unable to setup clock.\n");
	return FALSE;
}

static gboolean supl_create_account (MBMManager *manager)
{
	int len;
	char buf[512];
	char *response;
	MBMManagerPrivate *priv = MBM_MANAGER_GET_PRIVATE (manager);
	
	len = sprintf (buf, "AT*EIAC=1,\"supl\"\r\n");
	response = serial_send_AT_cmd (manager, buf, len);
	if (response && strstr (response, "OK")) {
		char *idx = strstr (response, "*EIAC: ");
		priv->supl_account_idx = strtol (idx + 7, NULL, 10);
		free (response);
		if (mbm_options_debug ())
			g_debug ("The account id for SUPL is %d.\n",
					 priv->supl_account_idx);
		return TRUE;
	} 

	if (mbm_options_debug ())
		g_debug ("Unable to setup supl account.\n");
	return FALSE;
}

static gboolean supl_send_cmd (MBMManager *manager, char *buf, int len)
{
	char *response;

	response = serial_send_AT_cmd (manager, buf, len);
	
	if (response && strstr (response, "OK")) {
		free (response);
		if (mbm_options_debug())
			g_debug ("%s %s OK", __FUNCTION__, buf);
		return TRUE;
	}

	if (mbm_options_debug())
		g_debug ("%s %s FAILED", __FUNCTION__, buf);
	return FALSE;
}

/*
 * Setup supl
 * returns 1 if setup was successful, 0 otherwise
 */
int setup_supl (MBMManager * manager)
{
	char buf[512];
	int len;
	char *supl_address;
	gboolean supl_setup_failed;
	MBMManagerPrivate *priv = MBM_MANAGER_GET_PRIVATE (manager);

	supl_setup_failed = FALSE;

	if (mbm_supl_settings_changed ()) {
		if (mbm_options_debug ())
			g_debug
				("Supl settings have changed. Removing them before readding them.\n");
		remove_supl_setup (manager);
		mbm_set_supl_settings_changed (0);
	}

	if (priv->supl_setup_done) {
		if (mbm_options_debug ())
			g_debug ("SUPL is already setup successfully.\n");
		return priv->supl_setup_done;
	}
	if (mbm_options_debug ())
		g_debug ("Setting up SUPL.\n");

	//setup the clock
	supl_setup_clock (manager);

	//setup additional data account and supl info
	if (!supl_create_account (manager)) {
		return 0;
	}

	if (strlen (mbm_supl_apn ()) > 0) {
		len = sprintf (buf, "AT*EIAPSW=%d,1,\"%s\"\r\n", priv->supl_account_idx,
					   mbm_supl_apn ());
		if (!supl_send_cmd (manager, buf, len))
			supl_setup_failed = TRUE;
			
		/* TODO maybe add check if username or password == "" */
		len =
			sprintf (buf, "AT*EIAAUW=%d,1,\"%s\",\"%s\"\r\n",
					 priv->supl_account_idx, mbm_supl_user (),
					 mbm_supl_password ());
		if (!supl_send_cmd (manager, buf, len))
			supl_setup_failed = TRUE;
		
		if (mbm_supl_auto_configure_address ())
			supl_address = "";
		else
			supl_address = mbm_supl_address ();

		len = sprintf (buf,
					   "AT*E2GPSSUPL=1,%d,\"%s\",%d\r\n",
					   priv->supl_account_idx, supl_address,
					   mbm_supl_enable_ni ());
		if (!supl_send_cmd (manager, buf, len))
			supl_setup_failed = TRUE;

		if (!supl_setup_failed) {
			if (mbm_options_debug ())
				g_debug ("SUPL setup successful.\n");
			
			/* now supl has been setup successfully */
			priv->supl_setup_done = 1;
			
			/* finally enable unsolicited responses for supl ni */
			len = sprintf (buf, "AT*E2GPSSUPLNI=1\r\n");
			if (!supl_send_cmd (manager, buf, len)) {
				if (mbm_options_debug ())
					g_debug
						("Unable to enable supl network initiated unsolicited responses.\n");
			}
			
			len = sprintf (buf, "AT*E2CERTUN=1\r\n");
			if (!supl_send_cmd (manager, buf, len)) {
				if (mbm_options_debug ())
					g_debug
						("Unable to enable certificate unknown unsolicited responses.\n");
			}
		}
	} else {
		if (mbm_options_debug ())
			g_debug ("Supl apn is not set. Unable to setup supl.\n");
	}
	
/* check if the account has been setup but we failed later
 * if so we need to remove the account again
 */
	if (!priv->supl_setup_done && (priv->supl_account_idx != -1)) {
		remove_supl_setup (manager);
	}
	
	return priv->supl_setup_done;
}

static char u_char_to_ascii_char (unsigned char c)
{
	switch (c) {
	case 0x0:
		return '0';
	case 0x1:
		return '1';
	case 0x2:
		return '2';
	case 0x3:
		return '3';
	case 0x4:
		return '4';
	case 0x5:
		return '5';
	case 0x6:
		return '6';
	case 0x7:
		return '7';
	case 0x8:
		return '8';
	case 0x9:
		return '9';
	case 0xA:
		return 'A';
	case 0xB:
		return 'B';
	case 0xC:
		return 'C';
	case 0xD:
		return 'D';
	case 0xE:
		return 'E';
	case 0xF:
		return 'F';
	default:
		return -1;
	}
}

/* convert the data read from certificate file to a format the module
 * understands.
 * Every byte should be converted to two ascii characters.
 * For example, if the first source byte is 11110001 the first two characters
 * in the destination will be 'F' and '1'
 * the dest string must be at least double the length of the src string
 * returns the length of the destination string
 */
static int convert_certificate_data (char *dest, char *src, int src_len)
{
	int i, j, dest_len;
	unsigned char lsb;
	unsigned char msb;
	j = 0;
	dest_len = 0;
	for (i = 0; i < src_len; i++) {
		lsb = (*(src + i)) & 0x0F;
		msb = ((*(src + i)) >> 4) & 0x0F;

		*(dest + j++) = u_char_to_ascii_char (msb);
		*(dest + j++) = u_char_to_ascii_char (lsb);
		dest_len += 2;
	}
	return dest_len;
}

int supl_install_certificate (MBMManager * manager, char *file_path)
{
	FILE *file;
	long size;
	char *buffer;
	char *converted_buf;
	size_t result;
	int tot_len;
	int len;
	char buf[2048];
	char *response;

	if (mbm_options_debug ())
		g_debug ("Installing supl certificate: %s\n", file_path);

	file = fopen (file_path, "r");
	if (file == NULL) {
		g_debug ("Unable to read certificate file.\n");
		return 0;
	}

	/* get size of file */
	fseek (file, 0, SEEK_END);
	size = ftell (file);
	rewind (file);

	/* allocate enough memory to read entire file */
	buffer = (char *)malloc (sizeof (char) * size);
	if (buffer == NULL) {
		g_debug ("Unable to allocate memory.\n");
		fclose (file);
		return 0;
	}

	/* copy file into buffer */
	result = fread (buffer, 1, size, file);
	if (result != size) {
		g_debug ("Error reading file, mismatch in characters read.\n");
		fclose (file);
		free (buffer);
		return 0;
	}

	converted_buf = (char *)malloc (sizeof (char) * (2 * size + 1));
	tot_len = convert_certificate_data (converted_buf, buffer, result);
	*(converted_buf + tot_len++) = 26;

	/* send the certificate to the module */
	len = sprintf (buf, "AT*E2CERT=%d\r\n", tot_len - 1);
	response = serial_send_AT_cmd (manager, buf, len);

	if (response && strstr (response, ">")) {
		free (response);
		response = serial_send_AT_cmd (manager, converted_buf, tot_len);

		if (response && strstr (response, "OK")) {
			free (response);
		} else {
			g_debug
				("Unable to install supl certificate. NOK response from module.\n");
			free (response);
			fclose (file);
			free (buffer);
			return 0;
		}
	} else {
		g_debug
			("Unable to install supl certificate. > not received from module.\n");
		fclose (file);
		free (buffer);
		return 0;
	}

	/* free memory and stuff */
	fclose (file);
	free (buffer);

	if (mbm_options_debug ())
		g_debug ("Installation of supl certificate successful.\n");

	return 1;

}
