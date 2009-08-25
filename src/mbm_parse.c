/*
 * mbm_parse - parse utilities
 *
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

#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>

#include "mbm_errors.h"
#include "gps_settings.h"
#include "mbm_gpsd_type.h"
#include "mbm_options.h"
#include "mbm_manager.h"
#include "mbm_modem.h"
#include "mbm_parse.h"

/**
 *  @brief Parses an Int from string.
 *
 *  @param ppData - IN OUT ptr string ptr, updated to end of int, if found.
 *  @param len - IN OUT total length of string ptr. updated.
 *  @param nOut - parsed integer.
 *
 *  @return true if int was found.
 */
int mbm_parse_int (char **ppData, int *len, int *nOut)
{
	int i = 0;
	int tmpVal;
	char *pEnd;

	// Skip all ' '
	while (i < *len && (*ppData)[i] == ' ')
		i++;

	// Skip _one_ ',' if present
	if (i < *len && (*ppData)[i] == ',')
		i++;

	if (i == *len) {
		return 0;
	}
	// Skip _one_ ',' if present and return, empty field found
	if (i < *len && (*ppData)[i] == ',') {
		i++;
		*len -= 1;
		*ppData += 1;
		return 0;
	}

	tmpVal = strtol ((*ppData) + i, &pEnd, 10);

	if ((*ppData) + i == pEnd) {
		return 0;
	}

	*nOut = tmpVal;
	*len -= pEnd - *ppData;
	*ppData += pEnd - *ppData;

	return 1;
}

/**
 *  @brief Parses a "String" from ptr string ptr.
 *
 *  @param ppData - IN OUT ptr string ptr, updated to end of string, if found.
 *  @param len - IN OUT total length of string ptr. updated.
 *  @param pszOutStr - OUT parsed string ptr.
 *  @param nAllocatedStrLen - IN Length of pszOutStr.
 *
 *  @return true if string was found.
 */
int
mbm_parse_str (char **ppData, int *len, char *pszOutStr, int nAllocatedStrLen)
{
	int i = 0;
	int strLen = 0;
	const char *pStart;

	// Skip ',' and ' '
	//while(i < *len && ((*ppData)[i]==',' || (*ppData)[i]==' ')) i++;

	// Skip all ' '
	while (i < *len && (*ppData)[i] == ' ')
		i++;

	// Skip _one_ ',' if present
	if (i < *len && (*ppData)[i] == ',')
		i++;

	//if(i == *len)
	//{
	//    return false;
	//}

	if (i == *len || (*ppData)[i] != '"')
		return 0;

	i++;
	pStart = (*ppData) + i;

	while (i < *len && (*ppData)[i] != '"') {
		strLen++;
		i++;
	}

	if (i == *len || (*ppData)[i] != '"' || (nAllocatedStrLen > 0 && strLen
											 >= nAllocatedStrLen))
		return 0;

	if (nAllocatedStrLen > 0) {
		memcpy (pszOutStr, pStart, strLen);
		pszOutStr[strLen] = 0;
	}
	// Skip the enclosing '"'
	i++;

	*len -= i;
	*ppData += i;

	return 1;
}
