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
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include "mbm_options.h"
#include "gps_settings.h"

static gboolean _foreground = FALSE;
static gboolean _debug = FALSE;
static gboolean _verbose = FALSE;
static gint nmea_interval = 1;

static int nmea_mode = 1;

static int user_disabled_gps = 0;
static int single_shot = 0;

static char supl_apn[512];
static char supl_address[512];
static gint supl_enable_ni = 1;
static gchar supl_user[512];
static gchar supl_password[512];
static gint supl_auto_configure_address = 0;

static int supl_settings_changed = 0;

static int gps_customization = 0;
static int gps_supl_customization = 0;

static gchar *pin_code;
static gchar *puk_code;

void mbm_options_parse (int argc, char *argv[])
{
	GOptionContext *opt_ctx;
	GError *error = NULL;
	GOptionEntry entries[] = {
		{"foreground", 'f', 0, G_OPTION_ARG_NONE, &_foreground,
		 "Keep the daemon process in foreground", NULL},
		{"debug", 'N', 0, G_OPTION_ARG_NONE, &_debug,
		 "Output to console rather than syslog", NULL},
		{"interval", 'i', 0, G_OPTION_ARG_INT, &nmea_interval,
		 "Set the nmea sentence interval to N seconds", "N"},
		{"verbose", 'v', 0, G_OPTION_ARG_NONE, &_verbose, "Be verbose", NULL},
		{NULL}
	};

	opt_ctx = g_option_context_new (NULL);
	g_option_context_set_summary (opt_ctx,
								  "DBus system service to communicate with gps.");
	g_option_context_add_main_entries (opt_ctx, entries, NULL);

	if (!g_option_context_parse (opt_ctx, &argc, &argv, &error)) {
		g_warning ("%s\n", error->message);
		g_error_free (error);
		exit (1);
	}

	g_option_context_free (opt_ctx);
}

gint mbm_options_debug (void)
{
	if (_debug && _verbose)
		return 2;
	return _debug ? 1 : 0;
}

gchar *mbm_pin_code ()
{
	return pin_code;
}

void mbm_set_pin_code (gchar * pin)
{
	pin_code = g_strdup (pin);
}

gchar *mbm_puk_code ()
{
	return puk_code;
}

void mbm_set_puk_code (gchar * puk)
{
	puk_code = g_strdup (puk);
}

gint mbm_gps_customization (gint key)
{
	switch (key) {
	case STAND_ALONE_MODE:
		return gps_customization;
	case SUPL_MODE:
		return gps_supl_customization;
	default:
		return -1;
	}
}

void mbm_set_gps_customization (gint key, gint value)
{
	int enabled;
	if (value)
		enabled = 1;
	else
		enabled = 0;

	switch (key) {
	case STAND_ALONE_MODE:
		gps_customization = enabled;
		break;
	case SUPL_MODE:
		gps_supl_customization = enabled;
		break;
	default:
		break;
	}
}

gboolean mbm_foreground (void)
{
	return _foreground;
}

gint mbm_nmea_interval (void)
{
	return nmea_interval;
}

void mbm_set_nmea_interval (gint interval)
{
	nmea_interval = interval;
}

gint mbm_supl_settings_changed (void)
{
	return supl_settings_changed;
}

void mbm_set_supl_settings_changed (gint settings_changed)
{
	if (settings_changed)
		supl_settings_changed = 1;
	else
		supl_settings_changed = 0;
}

gint mbm_supl_enable_ni (void)
{
	return supl_enable_ni;
}

void mbm_set_supl_enable_ni (gint _supl_enable_ni)
{
	supl_enable_ni = _supl_enable_ni;
	mbm_set_supl_settings_changed (1);
}

void mbm_set_single_shot (gint _single_shot)
{
	single_shot = _single_shot;
}

gint mbm_single_shot (void)
{
	return single_shot;
}

gchar *mbm_supl_apn (void)
{
	return supl_apn;
}

void mbm_set_supl_apn (gchar * _supl_apn)
{
	if (_supl_apn && strlen (_supl_apn) < 500)
		g_stpcpy (supl_apn, _supl_apn);
	else
		g_stpcpy (supl_apn, "");
	mbm_set_supl_settings_changed (1);
}

gint mbm_supl_auto_configure_address (void)
{
	return supl_auto_configure_address;
}

void mbm_set_supl_auto_configure_address (gint _supl_auto_configure_address)
{
	supl_auto_configure_address = _supl_auto_configure_address;
	mbm_set_supl_settings_changed (1);
}

gchar *mbm_supl_address (void)
{
	return supl_address;
}

void mbm_set_supl_address (gchar * _supl_address)
{
	if (_supl_address && strlen (_supl_address) < 500)
		g_stpcpy (supl_address, _supl_address);
	else
		g_stpcpy (supl_address, "");
	mbm_set_supl_settings_changed (1);
}

gchar *mbm_supl_user (void)
{
	return supl_user;
}

void mbm_set_supl_user (gchar * _supl_user)
{
	if (_supl_user && strlen (_supl_user) < 500)
		g_stpcpy (supl_user, _supl_user);
	else
		g_stpcpy (supl_user, "");
	mbm_set_supl_settings_changed (1);
}

gchar *mbm_supl_password (void)
{
	return supl_password;
}

void mbm_set_supl_password (gchar * _supl_password)
{
	if (_supl_password && strlen (_supl_password) < 500)
		g_stpcpy (supl_password, _supl_password);
	else
		g_stpcpy (supl_password, "");
	mbm_set_supl_settings_changed (1);
}

gint mbm_nmea_mode (void)
{
	return nmea_mode;
}

void mbm_set_nmea_mode (gint _nmea_mode)
{
	nmea_mode = _nmea_mode;
}

void mbm_set_user_disabled_gps (gint disable_gps)
{
	user_disabled_gps = disable_gps;
}

gint mbm_user_disabled_gps (void)
{
	return user_disabled_gps;
}
