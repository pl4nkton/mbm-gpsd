/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * mbm_supl.c - supl functions
 *
 * Copyright (C) 2009 Ericsson AB
 *
 * Authors: Torgny Johansson <torgny.johansson@ericsson.com>
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
#ifndef MBM_SUPL_H
#define MBM_SUPL_H

#define SUPL_INST_CERT_NOT_STARTED 0
#define SUPL_INST_CERT_RUNNING 1
#define SUPL_INST_CERT_FAILED 2
#define SUPL_INST_CERT_SUCCESS 3

void supl_get_current_date_string (char *date_string);

int remove_supl_setup (MBMManager * manager);

int setup_supl (MBMManager * manager);

int supl_install_certificate (MBMManager * manager, char *file_path);

#endif
