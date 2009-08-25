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
#define _GNU_SOURCE				/* for strcasestr() */

#include <stdio.h>
#include <stdlib.h>
#include <termio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <poll.h>

#include "mbm_manager.h"
#include "mbm_serial.h"
#include "mbm_errors.h"
#include "mbm_options.h"

static pthread_mutex_t nmea_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Function that serializes command sending.
 The result is put in fresh buffer, which the calling
 function needs to free when not longer needed.
 */
char *serial_send_AT_cmd (MBMManager * manager, char *buf, int len)
{
	int status = 0;
	char *response;
	struct pollfd fds[1];
	int send_nr;
	MBMManagerPrivate *priv = MBM_MANAGER_GET_PRIVATE (manager);

	pthread_mutex_lock (&nmea_mutex);

	response = malloc (MAX_RESPONSE);
	g_debug ("MAX_RESPONSE: %d", MAX_RESPONSE);
	response[0] = 0;

	/* send command */
	/* only send max 256 bytes at a time since the module doesn't receive otherwise (fw or driver issue?) */
	while (status < len) {
		send_nr = len - status;
		if (send_nr > 256)
			send_nr = 256;
		status += write (priv->ctrl_fd, buf + status, send_nr);
	}
	if (mbm_options_debug ())
		g_debug ("Control: --> %d, \"%s\"", status, buf);
	if (status != len) {
		g_error ("Control: error (%m)");
		strcat (response, "ERROR");
		pthread_mutex_unlock (&nmea_mutex);
		return response;
	}

	/* wait for command to complete */
	len = 0;
	fds[0].fd = priv->ctrl_fd;
	fds[0].events = POLLIN;
	while (1) {
		status = poll (fds, 1, 5000);	/* wait up to 5 seconds */
		g_debug ("status=%d", status);
		if (!status) {
            if (len > 0) {
                pthread_mutex_unlock (&nmea_mutex);
                return response;
            } else {
                if (mbm_options_debug () > 1)
                    g_debug ("No response within 5 sec");
                free (response);
                pthread_mutex_unlock (&nmea_mutex);
                return NULL;
            }
		} else if (status < 0) {
			if (mbm_options_debug () > 1)
				g_debug ("Response error (%m)");
			free (response);
			pthread_mutex_unlock (&nmea_mutex);
			return NULL;
		}
		len += read (priv->ctrl_fd, response + len, MAX_RESPONSE - len);
		if (len > MAX_RESPONSE) {
			g_error ("buffer underrun");
			free (response);
			pthread_mutex_unlock (&nmea_mutex);
			return NULL;
		}
		response[len] = 0;
		if (mbm_options_debug () > 1)
			g_debug ("response: <-- \"%s\"", response);
		if (strstr (response, "OK"))
			break;
		if (strstr (response, "ERROR"))
			break;
		if (strstr (response, "> "))
			break;
	}

	pthread_mutex_unlock (&nmea_mutex);
	return response;
}
