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
#include "mbm_callback_info.h"
#include "mbm_errors.h"

static void callback_info_done (gpointer user_data)
{
	MBMCallbackInfo *info = (MBMCallbackInfo *) user_data;
	gpointer result;

	info->pending_id = 0;

	result = mbm_callback_info_get_data (info, "callback-info-result");

	if (info->async_callback)
		info->async_callback (info->manager, info->error, info->user_data);
	else if (info->uint_callback)
		info->uint_callback (info->manager, GPOINTER_TO_UINT (result),
							 info->error, info->user_data);
	else if (info->str_callback)
		info->str_callback (info->manager, (const char *)result, info->error,
							info->user_data);

	if (info->error)
		g_error_free (info->error);

	if (info->manager)
		g_object_unref (info->manager);

	g_datalist_clear (&info->qdata);
	g_slice_free (MBMCallbackInfo, info);
}

static gboolean callback_info_do (gpointer user_data)
{
	/* Nothing here, everything is done in callback_info_done to make sure the info->callback
	   always gets called, even if the pending call gets cancelled. */
	return FALSE;
}

void mbm_callback_info_schedule (MBMCallbackInfo * info)
{
	info->pending_id =
		g_idle_add_full (G_PRIORITY_DEFAULT_IDLE, callback_info_do, info,
						 callback_info_done);
}

MBMCallbackInfo *mbm_callback_info_new (MBMManager * manager,
										MBMManagerFn callback,
										gpointer user_data)
{
	MBMCallbackInfo *info;

	info = g_slice_new0 (MBMCallbackInfo);
	g_datalist_init (&info->qdata);
	info->manager = g_object_ref (manager);
	info->async_callback = callback;
	info->user_data = user_data;

	return info;
}

MBMCallbackInfo *mbm_callback_info_uint_new (MBMManager * manager,
											 MBMManagerUIntFn callback,
											 gpointer user_data)
{
	MBMCallbackInfo *info;

	info = g_slice_new0 (MBMCallbackInfo);
	g_datalist_init (&info->qdata);
	info->manager = g_object_ref (manager);
	info->uint_callback = callback;
	info->user_data = user_data;

	return info;
}

MBMCallbackInfo *mbm_callback_info_string_new (MBMManager * manager,
											   MBMManagerStringFn callback,
											   gpointer user_data)
{
	MBMCallbackInfo *info;

	info = g_slice_new0 (MBMCallbackInfo);
	g_datalist_init (&info->qdata);
	info->manager = g_object_ref (manager);
	info->str_callback = callback;
	info->user_data = user_data;

	return info;
}

void
mbm_callback_info_set_result (MBMCallbackInfo * info,
							  gpointer data, GDestroyNotify destroy)
{
	mbm_callback_info_set_data (info, "callback-info-result", data, destroy);
}

void
mbm_callback_info_set_data (MBMCallbackInfo * info,
							const char *key,
							gpointer data, GDestroyNotify destroy)
{
	g_datalist_id_set_data_full (&info->qdata, g_quark_from_string (key), data,
								 data ? destroy : (GDestroyNotify) NULL);
}

gpointer mbm_callback_info_get_data (MBMCallbackInfo * info, const char *key)
{
	GQuark quark;

	quark = g_quark_try_string (key);

	return quark ? g_datalist_id_get_data (&info->qdata, quark) : NULL;
}
