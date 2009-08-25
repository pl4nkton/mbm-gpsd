/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * mbm_gpsd_type - Types used in MBM GPS, for D-Bus wrapper
 *
 * Copyright (C) 2008 Ericsson AB
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

#ifndef MBM_GPSD_TYPE_H
#define MBM_GPSD_TYPE_H

#include <glib-object.h>

typedef struct _MBMGpsd MBMGpsd;

typedef void (*MBMGpsdFn) (MBMGpsd * gpsd, GError * error, gpointer user_data);

typedef void (*MBMGpsdUIntFn) (MBMGpsd * gpsd,
							   guint32 result,
							   GError * error, gpointer user_data);

typedef void (*MBMGpsdStringFn) (MBMGpsd * gpsd,
								 const char *result,
								 GError * error, gpointer user_data);

struct _MBMGpsd {
	GTypeInterface g_iface;

	/* Methods */
	void (*set_fix_interval) (MBMGpsd * self,
							  int interval,
							  MBMGpsdFn callback, gpointer user_data);

	void (*set_pref_gps_mode) (MBMGpsd * self,
							   int mode,
							   MBMGpsdFn callback, gpointer user_data);

};

void mbm_set_fix_interval (MBMGpsd * self,
						   int interval,
						   MBMGpsdFn callback, gpointer user_data);

void mbm_set_pref_gps_mode (MBMGpsd * self,
							int mode, MBMGpsdFn callback, gpointer user_data);

#endif
