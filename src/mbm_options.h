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
#ifndef MBM_OPTIONS_H
#define MBM_OPTIONS_H

void mbm_options_parse (int argc, char *argv[]);

gint mbm_options_debug (void);

gint mbm_gps_customization (gint key);

void mbm_set_gps_customization (gint key, gint value);

gboolean mbm_foreground (void);

gint mbm_nmea_interval (void);

void mbm_set_nmea_interval (gint interval);

gint mbm_single_shot (void);

void mbm_set_single_shot (gint _single_shot);

gint mbm_nmea_mode (void);

void mbm_set_nmea_mode (gint mode);

gchar *mbm_pin_code (void);

void mbm_set_pin_code (gchar * pin);

gchar *mbm_puk_code (void);

void mbm_set_puk_code (gchar * puk);

/* SUPL */
gint mbm_supl_settings_changed (void);

void mbm_set_supl_settings_changed (gint settings_changed);

gchar *mbm_supl_apn (void);

void mbm_set_supl_apn (gchar *);

gchar *mbm_supl_address (void);

void mbm_set_supl_auto_configure_address (gint _supl_auto_configure_address);

gint mbm_supl_auto_configure_address (void);

void mbm_set_supl_address (gchar *);

gchar *mbm_supl_user (void);

void mbm_set_supl_user (gchar *);

gchar *mbm_supl_password (void);

void mbm_set_supl_password (gchar *);

gint mbm_supl_enable_ni (void);

void mbm_set_supl_enable_ni (gint interval);

void mbm_set_user_disabled_gps (gint disable_gps);

gint mbm_user_disabled_gps (void);

#endif /* MBM_OPTIONS_H */
