/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * Copyright (C) 2009 Ericsson AB
 *
 * Authors: Bjorn Runaker <bjorn.runaker@ericsson.com>
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
#ifndef MBM_MODEM_ERROR_H
#define MBM_MODEM_ERROR_H

#include <glib-object.h>

enum {
	MBM_SERIAL_OPEN_FAILED = 0,
	MBM_SERIAL_SEND_FAILED = 1,
	MBM_SERIAL_RESPONSE_TIMEOUT = 2
};

#define MBM_SERIAL_ERROR (mbm_serial_error_quark ())
#define MBM_TYPE_SERIAL_ERROR (mbm_serial_error_get_type ())

GQuark mbm_serial_error_quark (void);
GType mbm_serial_error_get_type (void);

enum {
	MBM_MODEM_ERROR_GENERAL = 0,
	MBM_MODEM_ERROR_OPERATION_NOT_SUPPORTED = 1
};

#define MBM_MODEM_ERROR (mbm_modem_error_quark ())
#define MBM_TYPE_MODEM_ERROR (mbm_modem_error_get_type ())

GQuark mbm_modem_error_quark (void);
GType mbm_modem_error_get_type (void);

#endif /* MM_MODEM_ERROR_H */
