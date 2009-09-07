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
#ifndef MBM_MODEM_H
#define MBM_MODEM_H

#define UR_GPS          "*E2GPSSTAT:"
#define UR_E2CFUN       "*E2CFUN:"
#define UR_E2GPSSUPLNI  "*E2GPSSUPLNI:"
#define UR_E2CERTUN     "*E2CERTUN:"
#define UR_CREG         "+CREG:"
#define UR_ERINFO        "*ERINFO: "

#define AT_CIMI     "AT+CIMI"
#define AT_CGSN     "AT+CGSN"
#define AT_ERINFO   "AT*ERINFO?"
#define AT_CREG     "AT+CREG?"

#define MBM_REGISTRATION_STATUS_NOT_REGISTERED 0
#define MBM_REGISTRATION_STATUS_HOME_NETWORK 1
#define MBM_REGISTRATION_STATUS_SEARCHING 2
#define MBM_REGISTRATION_STATUS_DENIED 3
#define MBM_REGISTRATION_STATUS_UNKNOWN 4
#define MBM_REGISTRATION_STATUS_ROAMING 5

void modem_error_device_stalled (MBMManager * manager);

void modem_check_pin (MBMManager * manager);

void modem_check_radio (MBMManager * manager);

void modem_send_supl_ni_reply (MBMManager * manager, int msg_id, int allow);

void modem_send_supl_certun_reply (MBMManager * manager, int msg_id, int allow);

int modem_nmea_beutify (char *buf);

int modem_check_AT_cmd (char *at_cmd);

void modem_open_gps_nmea (MBMManager * manager);

void modem_open_gps_ctrl (MBMManager * manager);

void modem_enable_unsolicited_responses (MBMManager * manager);

void modem_disable_unsolicited_responses (MBMManager * manager);

void modem_check_gps_customization (MBMManager * manager);

void modem_set_gps_port (MBMManager * manager);

void modem_disable_gps (MBMManager * manager);

int
modem_enable_gps (MBMManager * manager, unsigned int mode,
				  const unsigned int interval);

void modem_parse_gps_nmea (MBMManager * manager, char *buf, int len);

void modem_parse_client (MBMManager * manager, int i, char *buf, int len);

void modem_parse_gps_ctrl (MBMManager * manager, char *buf, int len);

void modem_check_and_handle_gps_states (MBMManager * manager);

void modem_get_imsi (MBMManager * manager);

void modem_get_imei (MBMManager * manager);

void modem_get_erinfo (MBMManager * manager);

void modem_get_creg (MBMManager * manager);

void modem_install_supl_certificate (MBMManager * manager, char *file_path);

#endif /* MM_MODEM_H */
