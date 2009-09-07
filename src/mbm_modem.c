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
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <dbus/dbus-glib.h>
#include <glib.h>
#include <termios.h>
#include <time.h>
#include "mbm_manager.h"
#include "mbm_errors.h"
#include "mbm_serial.h"
#include "mbm_options.h"
#include "mbm_callback_info.h"
#include "gps_settings.h"
#include "mbm_supl.h"
#include "mbm_modem.h"
#include "mbm_errors.h"
#include "mbm_parse.h"

void modem_error_device_stalled (MBMManager * manager)
{
	g_error
		("No response from the module's control device. This is due to a bug in the device kernel/driver/firmware.\nPlease reset the module by executing the script mbm_reset and then restart mbm-gpsd.\nExiting.\n");
	exit (1);
}

void modem_send_supl_ni_reply (MBMManager * manager, int msg_id, int allow)
{
	char *response;
	char buf[32];
	int len;

	if (allow) {
		len = sprintf (buf, "AT*E2GPSSUPLNIREPLY=%d,%d\r\n", msg_id, 1);
		response = serial_send_AT_cmd (manager, buf, len);
	} else {
		len = sprintf (buf, "AT*E2GPSSUPLNIREPLY=%d,%d\r\n", msg_id, 0);
		response = serial_send_AT_cmd (manager, buf, len);
	}
	free (response);
}

void modem_send_supl_certun_reply (MBMManager * manager, int msg_id, int allow)
{
	char *response;
	char buf[32];
	int len;

	if (allow) {
		len = sprintf (buf, "AT*E2CERTUNREPLY=%d,%d\r\n", msg_id, 1);
		response = serial_send_AT_cmd (manager, buf, len);
	} else {
		len = sprintf (buf, "AT*E2CERTUNREPLY=%d,%d\r\n", msg_id, 0);
		response = serial_send_AT_cmd (manager, buf, len);
	}
	free (response);
}

int modem_nmea_beutify (char *buf)
{
	char *chsum;

	chsum = strrchr (buf, '*');

	/* end string after chsum */
	*(chsum + 3) = '\r';
	*(chsum + 4) = '\n';
	*(chsum + 5) = '\0';

    return strlen (buf);
}

int modem_check_AT_cmd (char *at_cmd)
{
	int is_AT_cmd;

	if (strncasecmp (at_cmd, "AT", 2)) {
		is_AT_cmd = 0;
		/* if it does not contain AT but do contain \r return -1 to indicate a faulty AT command */
		if (strstr (at_cmd, "\r"))
			return -1;
	} else if (strstr (at_cmd, "\r")) {
		is_AT_cmd = 1;
	} else {
		is_AT_cmd = 0;
	}

	if (is_AT_cmd) {
		if (mbm_options_debug ())
			g_debug ("%s is a complete at command.\n", at_cmd);
		return 1;
	} else {
		if (mbm_options_debug ())
			g_debug ("%s is not yet a complete at command.\n", at_cmd);
		return 0;
	}
}

void modem_check_pin (MBMManager * manager)
{
	char *response;
	char buf[40];
	int len;

	if (mbm_options_debug ())
		g_debug ("Checking if the sim card is locked.");

	len = sprintf (buf, "AT+CPIN?\r\n");
	response = serial_send_AT_cmd (manager, buf, len);

	if (response && strstr (response, "SIM PIN")) {
		free (response);
		if (mbm_options_debug ())
			g_debug ("Sim card is pin locked.");

		/* try to unlock once but don't care if it works or not */
		len = sprintf (buf, "AT+CPIN=\"%s\"\r\n", mbm_pin_code ());
		response = serial_send_AT_cmd (manager, buf, len);
		usleep (500);

	} else if (response && strstr (response, "SIM PUK")) {
		free (response);
		if (mbm_options_debug ())
			g_debug ("Sim card is puk locked.");

		/* try to unlock once but don't care if it works or not */
		len =
			sprintf (buf, "AT+CPIN=\"%s\",\"%s\"\r\n", mbm_puk_code (),
					 mbm_pin_code ());
		response = serial_send_AT_cmd (manager, buf, len);
		usleep (500);

	} else if (response && strstr (response, "READY")) {
		if (mbm_options_debug ())
			g_debug ("Sim card is not locked.");
    } else if (response && strstr (response, "ERROR")) {
        if (mbm_options_debug())
            g_debug ("Got an error. Probably no SIM inserted.");
	} else if (response == NULL) {
		modem_error_device_stalled (manager);
	} else {
		if (mbm_options_debug ()) {
			g_debug ("Sim card is in unknown state.");
            g_debug ("State: %s", response);
        }
	}

	free (response);
}

static void modem_check_registration_status (MBMManager *manager)
{
    char *response;
    char buf[15];
    int len;
    MBMManagerPrivate *priv = MBM_MANAGER_GET_PRIVATE (manager);
    
    if (mbm_options_debug ())
        g_debug ("Checking registration status");

    len = sprintf (buf, "AT+CREG?\r\n");
    response = serial_send_AT_cmd (manager, buf, len);

    if (response) {
        if (strstr (response, "+CREG: 1,1"))
            priv->registration_status = MBM_REGISTRATION_STATUS_HOME_NETWORK;
        if (strstr (response, "+CREG: 1,2"))
            priv->registration_status = MBM_REGISTRATION_STATUS_SEARCHING;
        if (strstr (response, "+CREG: 1,3"))
            priv->registration_status = MBM_REGISTRATION_STATUS_DENIED;
        if (strstr (response, "+CREG: 1,4"))
            priv->registration_status = MBM_REGISTRATION_STATUS_UNKNOWN;
        if (strstr (response, "+CREG: 1,5"))
            priv->registration_status = MBM_REGISTRATION_STATUS_ROAMING;
        if (mbm_options_debug ())
            g_debug ("Registration status: %d", priv->registration_status);
        free (response);
    } else {
        if (mbm_options_debug ())
            g_debug ("Unable to check registration status.");
    }
}

void modem_check_radio (MBMManager * manager)
{
	char *response;
	char buf[40];
	int len;

	if (mbm_options_debug ())
		g_debug ("Checking if radio is on.");

	len = sprintf (buf, "AT+CFUN?\r\n");
	response = serial_send_AT_cmd (manager, buf, len);

	if (response) {
		if (strstr (response, "+CFUN: 1") || strstr (response, "+CFUN: 5")) {
			if (mbm_options_debug ())
				g_debug ("Radio is already on.");
            modem_check_registration_status (manager);
		} else {
			if (mbm_options_debug ())
				g_debug ("Radio is off, trying to turn it on.");
			free (response);

			/* try to turn radio on but don't care if it works or not */
			len = sprintf (buf, "AT+CFUN=1\r\n");
			response = serial_send_AT_cmd (manager, buf, len);
			usleep (500);
		}
	}

	free (response);
}

void modem_check_gps_customization (MBMManager * manager)
{
	char *response;
	char buf[40];
	int len;
	char *eercust = "AT*EERCUST=2";

	/* check if gps is customized */
	len = sprintf (buf, "%s,\"MBM-GPS-Enable\"\r\n", eercust);
	response = serial_send_AT_cmd (manager, buf, len);
	if (response && strstr (response, "01")) {
		if (mbm_options_debug ())
			g_debug ("MBM-GPS-Enable is set in the customization.\n");
		mbm_set_gps_customization (STAND_ALONE_MODE, 1);
		free (response);
	} else if (response && strstr (response, "ERROR"))  {
        if (mbm_options_debug ())
            g_debug ("Unable to check GPS customization. Assuming F3507G module.");
        mbm_set_gps_customization (STAND_ALONE_MODE, 1);
        free (response);
    } else {
		if (mbm_options_debug ())
			g_debug ("MBM-GPS-Enable is not set in the customization.\n");
		mbm_set_gps_customization (STAND_ALONE_MODE, 0);
		free (response);
	}

	/* check if gps-supl is customized */
	len = sprintf (buf, "%s,\"MBM-GPS-SUPL-Enable\"\r\n", eercust);
	response = serial_send_AT_cmd (manager, buf, len);
	if (response && strstr (response, "01")) {
		if (mbm_options_debug ())
			g_debug ("MBM-GPS-SUPL-Enable is set in the customization.\n");
		mbm_set_gps_customization (SUPL_MODE, 1);
		free (response);
	} else {
		if (mbm_options_debug ())
			g_debug ("MBM-GPS-SUPL-Enable is not set in the customization.\n");
		mbm_set_gps_customization (SUPL_MODE, 0);
		free (response);
	}
}

void modem_open_gps_nmea (MBMManager * manager)
{
	int status;
	struct termios settings;
	MBMManagerPrivate *priv = MBM_MANAGER_GET_PRIVATE (manager);

	if (mbm_options_debug ())
		g_debug ("Opening %s\n", priv->nmea_dev);

	priv->nmea_fd = open (priv->nmea_dev, O_RDWR| O_EXCL | O_NONBLOCK | O_NOCTTY);
	if (priv->nmea_fd == -1) {
		g_error ("Open of %s failed: %m", priv->nmea_dev);
		exit (1);
	}

	FD_SET (priv->nmea_fd, &priv->fdsread);
	if (priv->nmea_fd >= priv->maxfd)
		priv->maxfd = priv->nmea_fd + 1;

	tcflush (priv->nmea_fd, TCIOFLUSH);
	status = tcgetattr (priv->nmea_fd, &settings);
	if (status) {
		g_error ("getting settings failed: %m");
		exit (1);
	}
	settings.c_lflag = ISIG | ICANON | IEXTEN | ECHOE | ECHOK;
	cfsetispeed (&settings, B4800);
	cfsetospeed (&settings, B4800);
	status = tcsetattr (priv->nmea_fd, TCSANOW, &settings);
	if (status) {
		g_error ("setting baudrate failed: %m");
		exit (1);
	}
}

void modem_open_gps_ctrl (MBMManager * manager)
{
	MBMManagerPrivate *priv = MBM_MANAGER_GET_PRIVATE (manager);

	if (mbm_options_debug ())
		g_debug ("Opening %s\n", priv->ctrl_dev);

	priv->ctrl_fd = open (priv->ctrl_dev, O_RDWR);
	if (priv->ctrl_fd == -1) {
		g_error ("Open of %s failed: %m", priv->ctrl_dev);
		exit (1);
	}

	FD_SET (priv->ctrl_fd, &priv->fdsread);
	if (priv->ctrl_fd >= priv->maxfd)
		priv->maxfd = priv->ctrl_fd + 1;
}

void modem_enable_unsolicited_responses (MBMManager * manager)
{
	char buf[32];
	char *response;
	int len, i;
	MBMManagerPrivate *priv = MBM_MANAGER_GET_PRIVATE (manager);

	modem_disable_unsolicited_responses (manager);

	if (mbm_options_debug ())
		g_debug ("Enabling unsolicited responses.");

	/* enable unsolicited responses */
	len = sprintf (buf, "AT*E2GPSSTAT=1\r\n");
	response = serial_send_AT_cmd (manager, buf, len);
	if (response == NULL)
		modem_error_device_stalled (manager);
    
	for (i = 0; strstr (response, "ERROR") && i < 5; i++) {
		if (mbm_options_debug ())
			g_debug ("Error enabling E2GPSSTAT, trying again.\n");
		sleep (2);
        free (response);
		response = serial_send_AT_cmd (manager, buf, len);
	}
	free (response);

	len = sprintf (buf, "AT*E2CFUN=1\r\n");
	response = serial_send_AT_cmd (manager, buf, len);
	if (response == NULL)
		modem_error_device_stalled (manager);
    
	for (i = 0; strstr (response, "ERROR") && i < 5; i++) {
		if (mbm_options_debug ())
			g_debug ("Error enabling E2CFUN, trying again.\n");
		sleep (2);
        free (response);
		response = serial_send_AT_cmd (manager, buf, len);
	}
	free (response);

    len = sprintf (buf, "AT+CREG=1\r\n");
	response = serial_send_AT_cmd (manager, buf, len);
	if (response == NULL)
		modem_error_device_stalled (manager);
    
	for (i = 0; strstr (response, "ERROR") && i < 5; i++) {
		if (mbm_options_debug ())
			g_debug ("Error enabling CREG, trying again.\n");
		sleep (2);
        free (response);
		response = serial_send_AT_cmd (manager, buf, len);
	}
	free (response);

	priv->unsolicited_responses_enabled = 1;
}

void modem_disable_unsolicited_responses (MBMManager * manager)
{
	char buf[32];
	char *response;
	int len;
	MBMManagerPrivate *priv = MBM_MANAGER_GET_PRIVATE (manager);

	if (mbm_options_debug ())
		g_debug ("Disabling unsolicited responses.");
	if (!priv->unsolicited_responses_enabled) {
		if (mbm_options_debug ())
			g_debug ("Unsolicited responses have not been enabled.\n");
		return;
	}
	/* enable unsolicited responses */
	len = sprintf (buf, "AT*E2GPSSTAT=0\r\n");
	response = serial_send_AT_cmd (manager, buf, len);
	free (response);

	len = sprintf (buf, "AT*E2CFUN=0\r\n");
	response = serial_send_AT_cmd (manager, buf, len);
	free (response);

    len = sprintf (buf, "AT+CREG=0\r\n");
	response = serial_send_AT_cmd (manager, buf, len);
	free (response);

	priv->unsolicited_responses_enabled = 0;
}

void modem_set_gps_port (MBMManager * manager)
{
	char buf[32];
	int len;
	int status;
	MBMManagerPrivate *priv = MBM_MANAGER_GET_PRIVATE (manager);

	if (mbm_options_debug ())
		g_debug ("Setting gps port.\n");

	len = sprintf (buf, "AT*E2GPSNPD\r\n");
	status = write (priv->nmea_fd, buf, len);
	if (mbm_options_debug () > 1)
		g_debug ("NMEA: --> %d, \"%s\"", status, buf);
	if (status != len) {
		g_debug ("NMEA: error setting GPS port (%m)");
		exit (1);
	}
}

int
modem_enable_gps (MBMManager * manager, unsigned int mode,
				  const unsigned int interval)
{
	char buf[32];
	char *response;
	int len;
    time_t rawtime;
	MBMManagerPrivate *priv = MBM_MANAGER_GET_PRIVATE (manager);

	if (mbm_options_debug ())
		g_debug ("trying to enable gps to %d, %d\n", mode, interval);

	if ((interval < 0) || (interval > 60)) {
		g_error ("invalid interval %d", interval);
		return -1;
	}

    time (&rawtime);
    priv->gps_start_time = mktime (localtime (&rawtime));

	if (mode == SUPL_MODE) {
        if (setup_supl (manager)) {
			if (mbm_options_debug ())
				g_debug ("Trying to enable GPS with SUPL.\n");
			len = sprintf (buf, "AT*E2GPSCTL=%d,%d\r\n", mode, interval);
			response = serial_send_AT_cmd (manager, buf, len);

			if (response && strstr (response, "OK")) {
				if (mbm_options_debug ())
					g_debug ("Gps enabled in SUPL mode.\n");
				if (priv->gps_port_not_defined) {
					if (mbm_options_debug ())
						g_debug ("GPS port not set, setting it.\n");
					modem_set_gps_port (manager);
					priv->gps_port_not_defined = 0;
				}
				priv->gps_enabled = SUPL_MODE;
				priv->new_gps_state.gps_enabled_mode = SUPL_MODE;
				modem_parse_gps_ctrl (manager, response, strlen (response));
				free (response);
				return SUPL_MODE;
			} else {
				if (mbm_options_debug ())
					g_debug
						("Failed to enable the GPS with SUPL. Falling back to STAND_ALONE.\n");
				mode = STAND_ALONE_MODE;
			}
			free (response);
		} else {
			if (mbm_options_debug ())
				g_debug
					("Supl was not setup properly. Falling back to STAND_ALONE.\n");
			mode = STAND_ALONE_MODE;
		}
	} else if (mode == OFF) {
		len = sprintf (buf, "AT*E2GPSCTL=0\r\n");
		response = serial_send_AT_cmd (manager, buf, len);
		g_debug ("GPS off. Response: %s", response);
		priv->gps_enabled = OFF;
		priv->new_gps_state.gps_enabled_mode = OFF;
        free (response);
		return OFF;
	}

	/* if we reached this point, SUPL failed or weren't enabled by the user */
	len = sprintf (buf, "AT*E2GPSCTL=%d,%d\r\n", mode, interval);
	response = serial_send_AT_cmd (manager, buf, len);

	if (response && strstr (response, "OK")) {
		if (mbm_options_debug ())
			g_debug ("Gps enabled in STAND_ALONE mode.\n");
		if (priv->gps_port_not_defined) {
			if (mbm_options_debug ())
				g_debug ("GPS port not set, setting it.\n");
			modem_set_gps_port (manager);
			priv->gps_port_not_defined = 0;
		}
		priv->gps_enabled = STAND_ALONE_MODE;
		priv->new_gps_state.gps_enabled_mode = STAND_ALONE_MODE;
		modem_parse_gps_ctrl (manager, response, strlen (response));
		free (response);
		return STAND_ALONE_MODE;
	}
	free (response);
	sleep (2);
	return 0;
}

void modem_disable_gps (MBMManager * manager)
{
	char buf[32];
	char *response;
	int len;
	MBMManagerPrivate *priv = MBM_MANAGER_GET_PRIVATE (manager);

	if (mbm_options_debug ())
		g_debug ("disabling gps\n");
	if (!priv->gps_enabled) {
		if (mbm_options_debug ())
			g_debug ("Gps has not been enabled.\n");
		return;
	}

	len = sprintf (buf, "AT*E2GPSCTL=0\r\n");

	response = serial_send_AT_cmd (manager, buf, len);

	if ( !(response && strstr (response, "OK")))
		g_debug ("Couldn't disable gps. Assuming it's ok.\n");

    free (response);
	priv->new_gps_state.gps_enabled_mode = 0;
	priv->gps_enabled = 0;
}

void modem_parse_gps_nmea (MBMManager * manager, char *buf, int len)
{
	char *str;
	int i;
	int written;
	gchar **nmea_infos;
	int valid_fix;
	MBMManagerPrivate *priv = MBM_MANAGER_GET_PRIVATE (manager);

	/* look for NMEA strings, write to active clients */
	str = strstr (buf, "$GP");
	/* split the nmea data to be used later */
	if (str)
		nmea_infos = g_strsplit (str, ",", 15);
	else
		nmea_infos = g_strsplit (" , , ", ",", 15);

	if (str) {
		for (i = 0; i < MAX_CLIENTS; i++) {
			if (priv->client[i].active) {
				len = modem_nmea_beutify (str);
				if (mbm_options_debug () > 1)
					g_debug ("Client: --> %d, \"%s\"", len, str);
				written = write (priv->client[i].master_fd, str, len);

                if (written == -1)
                    g_debug ("Unable to write to client. Ignoring.");
			}
		}
	}

	/* look for "+PACSP0", on resume we need to "reshoot" the at command
	 * that sets the NMEA port correctly
	 */
	str = strstr (buf, "+PACSP0");
	if (str)
		modem_set_gps_port (manager);

	/* finally parse the location if any */
	/* start with GPGGA */
	if (strstr (nmea_infos[0], "$GPGGA")) {
		valid_fix = atoi (nmea_infos[6]);
		if (valid_fix) {
			priv->location_data.fix = 1;
			g_stpcpy (priv->location_data.latitude, nmea_infos[2]);
			priv->location_data.latitude_hemi = nmea_infos[3][0];

			g_stpcpy (priv->location_data.longitude, nmea_infos[4]);
			priv->location_data.longitude_hemi = nmea_infos[5][0];

			g_stpcpy (priv->location_data.altitude, nmea_infos[9]);
		} else
			priv->location_data.fix = 0;
	}
}

void modem_parse_client (MBMManager * manager, int i, char *buf, int len)
{
	char *response;
	char *cmd;
	int cmd_len;
	int response_len;
	int written;
	int at_ok;
	MBMManagerPrivate *priv = MBM_MANAGER_GET_PRIVATE (manager);

	/* protection from buffer overflow */
	if (len + strlen (priv->client[i].at_cmd) > MAX_RESPONSE) {
		if (mbm_options_debug ())
			g_debug
				("The client sent too much nonsense. Resetting it's buffer.\n");
		memset (priv->client[i].at_cmd, 0, MAX_RESPONSE);
		return;
	}

	/* build the at-command */
	buf[len] = 0;
	strcat (priv->client[i].at_cmd, buf);

	at_ok = modem_check_AT_cmd (priv->client[i].at_cmd);
	if (at_ok == 1) {
		/* ok, command is ready. Send to module */
		response =
			serial_send_AT_cmd (manager, priv->client[i].at_cmd,
								strlen (priv->client[i].at_cmd));

		/* send response first to parse_gps_ctrl in order to
		   act on any unsolicited response that might be in this
		   buffer */
		modem_parse_gps_ctrl (manager, response, strlen (response));

		/* remove the, by module echoed, command in response */
		cmd = strstr (response, priv->client[i].at_cmd);
		if (cmd) {
			cmd_len = strlen (priv->client[i].at_cmd);
			response_len = strlen (response);
			memmove (cmd, cmd + cmd_len, strlen (cmd + cmd_len));
			cmd[response_len - cmd_len] = 0;
		}

		/* send response to client */
		strcat (response, "\r\n");
		written =
			write (priv->client[i].master_fd, response, strlen (response));
		if (mbm_options_debug () > 1)
			g_debug ("Client: --> \"%s\"", response);

        if (written == -1)
            g_debug ("Unable to write to client. Ignoring.");
        
		free (response);

		/* reset the at command */
		memset (priv->client[i].at_cmd, 0, MAX_RESPONSE);
	} else if (at_ok == -1)
		/* reset the at command */
		memset (priv->client[i].at_cmd, 0, MAX_RESPONSE);
}

static void send_unsolicited_to_client (MBMManager *manager, char *str, int len)
{
    int i, written;
    MBMManagerPrivate *priv = MBM_MANAGER_GET_PRIVATE (manager);
    
    for (i = 0; i < MAX_CLIENTS; i++) {
        if (priv->client[i].active)
            written = write (priv->client[i].master_fd, str, len);
    }
}

static void parse_e2gpsstat (MBMManager *manager, char *buf)
{
    char *str, *str2;
    int a, b, c, d, e;
    MBMManagerPrivate *priv = MBM_MANAGER_GET_PRIVATE (manager);
    
    str = g_strrstr (buf, UR_GPS);
	str2 = strstr (buf, UR_GPS);
	if (str) {
		if (str2 != str) {
			/* this is done since sometimes two unsolicited messages can come in the same buffer
			   in that case we need to handle both messages otherwise we may not discover that supl has
			   failed for example
            */
			a = str2[12] - '0';	/* 0 unsolicited responses off, 1 on */
			b = str2[14] - '0';	/* 0 gps off, 1 gps searching, 2 gps ok */
			c = str2[16] - '0';	/* 0 temp ok, 1 gps search limited, */
			d = str2[18] - '0';	/* gps mode, 0 none, 1 stand alone, 3 supl */
			e = str2[20] - '0';	/* supl status, 0 ok, 1-7 failure */
			priv->current_gps_state.gps_status = b;
			priv->current_gps_state.gps_overheated = c;
			priv->current_gps_state.gps_enabled_mode = d;
			priv->current_gps_state.supl_status = e;
			str2[21] = '\r';
			str2[22] = '\n';
			/* send unsolicited messages to all clients */
            send_unsolicited_to_client (manager, str2, 23);
		}

		a = str[12] - '0';		/* 0 unsolicited responses off, 1 on */
		b = str[14] - '0';		/* 0 gps off, 1 gps searching, 2 gps ok */
		c = str[16] - '0';		/* 0 temp ok, 1 gps search limited, */
		d = str[18] - '0';		/* gps mode, 0 none, 1 stand alone, 3 supl */
		e = str[20] - '0';		/* supl status, 0 ok, 1-7 failure */
		priv->new_gps_state.gps_status = b;
        priv->current_gps_state.gps_status = b;
        if (b == 2)
            priv->gps_fix_obtained = 1;
        else
            priv->gps_fix_obtained = 0;
		priv->new_gps_state.gps_overheated = c;
		priv->new_gps_state.gps_enabled_mode = d;
		priv->new_gps_state.supl_status = e;
		str[21] = '\r';
		str[22] = '\n';

		/* send unsolicited messages to all clients */
        send_unsolicited_to_client (manager, str, 23);
	}
}

static void parse_e2cfun (MBMManager *manager, char *buf)
{
    char *str;
    int a, b, c;
    MBMManagerPrivate *priv = MBM_MANAGER_GET_PRIVATE (manager);
    
    str = strstr (buf, UR_E2CFUN);
	if (str) {
		a = str[9] - '0';		/* mode */
		b = str[11] - '0';		/* fun */
		c = str[13] - '0';		/* w_disable */
		priv->new_gps_state.cfun = b;
		priv->new_gps_state.w_disable = c;
		str[14] = '\r';
		str[15] = '\n';
        
		/* send unsolicited messages to all clients */
        send_unsolicited_to_client (manager, str, 16);
	}
}

static void parse_e2gpssuplni (MBMManager *manager, char *buf)
{
    int a, b, c;
	char *str, *msg_id_end;
    GString *to_client;
    gchar **tmp;

    str = strstr (buf, UR_E2GPSSUPLNI);
	if (str) {
		if (mbm_options_debug ())
			g_debug ("Supl Network Initiated unsolicited response received.\n");
		a = str[14] - '0';		/* enable ni unsol */
		msg_id_end = strchr (&str[14], ',');
		b = atoi (msg_id_end + 1);	/* message id, 0-255 */
		msg_id_end = strchr (msg_id_end + 1, ',');
		c = msg_id_end[1] - '0';	/* message type, 0 - Ver, allow on timeout
									 * 1 - Ver, deny on timeout
									 * 2 - Notification
									 */

		/* TODO use other "optional" info that can come in the message as well */
		if (mbm_options_debug ())
			g_debug ("Supl NI message id: %d, message type: %d\n", b, c);
		if (c == 2) {
			/* notification */
			/* No need for this anymore. Handled in the gui. */
			//modem_send_supl_ni_reply(manager, b, 1);
		} else if (c == 1) {
			/* verification, deny on timeout */
		} else if (c == 0) {
			/* verification, allow on timeout */
		} else {
			if (mbm_options_debug ())
				g_debug ("Message type for %s unknwon: %d\n", UR_E2GPSSUPLNI,
						 c);
		}
		tmp = g_strsplit (str, "\r\n", 1);

		to_client = g_string_sized_new (strlen (tmp[0]) + 2);
		g_string_assign (to_client, tmp[0]);
		g_string_append (to_client, "\r\n");

		/* send unsolicited messages to all clients */
        send_unsolicited_to_client (manager, to_client->str, to_client->len);
        
		g_string_free (to_client, TRUE);
	}
}

static void parse_e2certun (MBMManager *manager, char *buf)
{
    int a, b, c;
	char *str, *msg_id_end;
    GString *to_client;
    gchar **tmp;
    
    str = strstr (buf, UR_E2CERTUN);
	if (str) {
		if (mbm_options_debug ())
			g_debug
				("Supl certificate unknown unsolicited response received.\n");
		a = str[11] - '0';		/* Unsolicited responses enabled */
		msg_id_end = strchr (&str[11], ',');
		b = atoi (msg_id_end + 1);	/* message id, 0-255 */
		msg_id_end = strchr (msg_id_end + 1, ',');
		c = msg_id_end[1] - '0';	/* application, 1 for supl */
        
		tmp = g_strsplit (str, "\r\n", 1);
        
		to_client = g_string_sized_new (strlen (tmp[0]) + 2);
		g_string_assign (to_client, tmp[0]);
		g_string_append (to_client, "\r\n");
        
		/* send unsolicited messages to all clients */
        send_unsolicited_to_client (manager, to_client->str, to_client->len);
 
		g_string_free (to_client, TRUE);
	}
}

static void parse_creg (MBMManager *manager, char *buf)
{
    char *str;
    int status;
    MBMManagerPrivate *priv = MBM_MANAGER_GET_PRIVATE (manager);
    
    str = strstr (buf, UR_CREG);

    if (str) {
        status = str[7] - '0';
        if (mbm_options_debug ())
            g_debug ("CREG unsolicited response received, status: %d", status);
        priv->registration_status = status;
    }
}

void modem_parse_gps_ctrl (MBMManager * manager, char *buf, int len)
{
	if (mbm_options_debug () > 1)
		g_debug ("%s: <-- %d, \"%s\"", __FUNCTION__, len, buf);

	g_debug ("PARSE_GPS_CTRL: %s", buf);

    parse_e2gpsstat (manager, buf);

    parse_e2cfun (manager, buf);

    parse_e2gpssuplni (manager, buf);

    parse_e2certun (manager, buf);

    parse_creg (manager, buf);
}

void modem_check_and_handle_gps_states (MBMManager * manager)
{
	int disable_gps_set = 0;
	int enable_gps_set = 0;
	int gps_mode = mbm_nmea_mode ();
	MBMManagerPrivate *priv = MBM_MANAGER_GET_PRIVATE (manager);
	int interval;
    int waiting_for_registration = 0;

	/* check if we should install a supl certificate */
	if (priv->install_supl_cert) {
		if (mbm_options_debug ())
			g_debug ("Install_supl_cert set.");
		if (supl_install_certificate (manager, priv->certificate_file_path)) {
			priv->install_supl_cert_status = SUPL_INST_CERT_SUCCESS;
		} else {
			priv->install_supl_cert_status = SUPL_INST_CERT_FAILED;
		}
		priv->install_supl_cert = 0;
	}

	/* check which interval to use (single shot or not) */
	if (mbm_single_shot ())
		interval = 0;
	else
		interval = mbm_nmea_interval ();

	/* check if anything has changed that the gui should now about */
	if ((priv->current_gps_state.gps_enabled_mode
		 != priv->new_gps_state.gps_enabled_mode)) {

		if (mbm_options_debug ())
			g_debug ("The gps status has changed to %d\n.",
					 priv->new_gps_state.gps_enabled_mode);
	}

	/* handle the w_disable flag */
	if ((priv->current_gps_state.w_disable == 0)
		&& (priv->new_gps_state.w_disable == 1)) {
		disable_gps_set = 1;
		if (mbm_options_debug ())
			g_debug ("W_DISABLE has been set.\n");
	} else if ((priv->current_gps_state.w_disable == 1)
			   && (priv->new_gps_state.w_disable == 0)) {
		enable_gps_set = 1;
		if (mbm_options_debug ())
			g_debug ("W_DISABLE has been unset.\n");
		sleep (3);

	}

	/* check if supl has failed */
	if ((priv->new_gps_state.gps_enabled_mode == 0)
		&& ((priv->current_gps_state.gps_enabled_mode == SUPL_MODE)
			|| (priv->new_gps_state.gps_enabled_mode == SUPL_MODE))
		&& (priv->new_gps_state.supl_status != 0)
		&& !priv->new_gps_state.w_disable) {
		if (mbm_options_debug ())
			g_debug ("SUPL has failed!!! Error: %d\n",
					 priv->new_gps_state.supl_status);
		priv->supl_failed = 1;
		enable_gps_set = 1;
		gps_mode = STAND_ALONE_MODE;
	}

	/* check if single shot is used and we have already gotten a fix */
	if (mbm_single_shot () && !priv->new_gps_state.gps_enabled_mode
		&& priv->current_gps_state.gps_enabled_mode) {
		if (mbm_options_debug ())
			g_debug
				("Single shot fix received. Disabling gps until user enables it again.\n");
		mbm_set_user_disabled_gps (1);
		priv->gps_enabled = 0;
	}

	/* check if user has disabled gps */
	if (mbm_user_disabled_gps ()) {
		disable_gps_set = 1;
		enable_gps_set = 0;
	}

	if (mbm_options_debug () > 2) {
		g_debug ("current_gps_state.gps_enabled_mode=%d\n",
				 priv->current_gps_state.gps_enabled_mode);
		g_debug ("new_gps_state.gps_enabled_mode=%d\n",
				 priv->new_gps_state.gps_enabled_mode);
	}

    if (gps_mode == SUPL_MODE) {
        switch (priv->registration_status) {
        case MBM_REGISTRATION_STATUS_HOME_NETWORK:
        case MBM_REGISTRATION_STATUS_ROAMING:
        case MBM_REGISTRATION_STATUS_DENIED:
            if (priv->current_gps_state.gps_enabled_mode == SUPL_WAITING_FOR_REGISTRATION)
                enable_gps_set = 1;
            
            waiting_for_registration = 0;
            break;
        case MBM_REGISTRATION_STATUS_NOT_REGISTERED:
        case MBM_REGISTRATION_STATUS_SEARCHING:
        case MBM_REGISTRATION_STATUS_UNKNOWN:
            if (mbm_options_debug () > 1)
                g_debug ("Waiting for network registration");
            waiting_for_registration = 1;
            priv->gps_enabled = SUPL_WAITING_FOR_REGISTRATION;
            priv->new_gps_state.gps_enabled_mode = SUPL_WAITING_FOR_REGISTRATION;
            break;
        default:
            break;
        }
    }
    
	priv->current_gps_state = priv->new_gps_state;

	/* finally enable/disable the gps */
	if (priv->client_connections && !disable_gps_set
		&& !priv->current_gps_state.w_disable) {
		if (priv->current_gps_state.gps_enabled_mode) {
			if (enable_gps_set && !waiting_for_registration) {
				if (mbm_options_debug ())
					g_debug
						("Enabling gps (enable_gps_set) with mode: %d, interval: %d.\n",
						 gps_mode, interval);
				modem_enable_gps (manager, gps_mode, interval);
			}
		} else {
            if (!waiting_for_registration) {
                if (mbm_options_debug ())
                    g_debug
                        ("Enabling gps (client_connections) with mode: %d, interval: %d.\n",
                         gps_mode, interval);
                modem_enable_gps (manager, gps_mode, interval);
            }
		}
	} else {
		if (priv->current_gps_state.gps_enabled_mode) {
			if (mbm_options_debug ())
				g_debug ("Disabling gps.\n");
			priv->supl_failed = 0;
			modem_disable_gps (manager);
		}
	}
}

void modem_install_supl_certificate (MBMManager * manager, char *file_path)
{
	MBMManagerPrivate *priv = MBM_MANAGER_GET_PRIVATE (manager);
	priv->install_supl_cert = 1;
	priv->install_supl_cert_status = SUPL_INST_CERT_RUNNING;
	priv->certificate_file_path = file_path;
}

/*****************************************************************************/
