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
#ifndef MBM_CALLBACK_INFO_H
#define MBM_CALLBACK_INFO_H

#include "mbm_manager.h"

typedef struct {
	GData *qdata;
	MBMManager *manager;

	MBMManagerFn async_callback;
	MBMManagerUIntFn uint_callback;
	MBMManagerStringFn str_callback;

	gpointer user_data;
	GError *error;
	guint pending_id;
} MBMCallbackInfo;

MBMCallbackInfo *mbm_callback_info_new (MBMManager * modem,
										MBMManagerFn callback,
										gpointer user_data);

MBMCallbackInfo *mbm_callback_info_uint_new (MBMManager * modem,
											 MBMManagerUIntFn callback,
											 gpointer user_data);

MBMCallbackInfo *mbm_callback_info_string_new (MBMManager * modem,
											   MBMManagerStringFn callback,
											   gpointer user_data);

void mbm_callback_info_schedule (MBMCallbackInfo * info);
void mbm_callback_info_set_result (MBMCallbackInfo * info,
								   gpointer data, GDestroyNotify destroy);

void mbm_callback_info_set_data (MBMCallbackInfo * info,
								 const char *key,
								 gpointer data, GDestroyNotify destroy);

gpointer mbm_callback_info_get_data (MBMCallbackInfo * info, const char *key);

#endif /* MM_CALLBACK_INFO_H */
