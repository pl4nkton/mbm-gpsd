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
#include "mbm_errors.h"

#define ENUM_ENTRY(NAME, DESC) { NAME, "" #NAME "", DESC }

GQuark mbm_serial_error_quark (void)
{
	static GQuark ret = 0;

	if (ret == 0)
		ret = g_quark_from_static_string ("mbm_serial_error");

	return ret;
}

GType mbm_serial_error_get_type (void)
{
	static GType etype = 0;

	if (etype == 0) {
		static const GEnumValue values[] = {
			ENUM_ENTRY (MBM_SERIAL_OPEN_FAILED, "SerialOpenFailed"),
			ENUM_ENTRY (MBM_SERIAL_SEND_FAILED, "SerialSendfailed"),
			ENUM_ENTRY (MBM_SERIAL_RESPONSE_TIMEOUT, "SerialResponseTimeout"),
			{0, 0, 0}
		};

		etype = g_enum_register_static ("MBMSerialError", values);
	}

	return etype;
}

GQuark mbm_modem_error_quark (void)
{
	static GQuark ret = 0;

	if (ret == 0)
		ret = g_quark_from_static_string ("mbm_modem_error");

	return ret;
}

GType mbm_modem_error_get_type (void)
{
	static GType etype = 0;

	if (etype == 0) {
		static const GEnumValue values[] = {
			ENUM_ENTRY (MBM_MODEM_ERROR_GENERAL, "General"),
			ENUM_ENTRY (MBM_MODEM_ERROR_OPERATION_NOT_SUPPORTED,
						"OperationNotSupported"),
			{0, 0, 0}
		};

		etype = g_enum_register_static ("MBMModemError", values);
	}

	return etype;
}
