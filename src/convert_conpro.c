/*
 * convert_conpro
 *
 * Copyright (C) 2009 Ericsson AB
 * Author: Bjorn Runaker <bjorn.runaker@ericsson.com>
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
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main (int argc, char *argv[])
{
	FILE *in, *out;
	char line[2048], *pStr;
	int mcc;
	int bThreeDigit, mnc;
	int current_line = 0;

	if (argc < 2) {
		printf ("Usage %s <ConnectionProfile.csv>\nGenerates mnc_list.h",
				argv[0]);
		exit (1);
	}
	in = fopen (argv[1], "r");
	out = fopen ("mnc_list.h", "w");
	if (in == NULL || out == NULL) {
		printf ("can't open files\n");
		exit (1);
	}
	fprintf (out,
			 "typedef struct {\n    int mcc;\n    int bT;\n    int mnc;\n} MNC_LIST;\nMNC_LIST mnc_list[] = {\n");
	while (fgets (line, 2048, in) != NULL) {
		current_line++;
		if (*line == ';' || *line == ' ' || *line == '\r' || *line == '\n')
			continue;
		mcc = strtol (line, &pStr, 10);
		pStr++;
		if (*pStr != 'T' && *pStr != 'F') {
			printf ("Error in file - field 2 line %d\n", current_line);
			exit (1);
		}
		if (*pStr == 'T')
			bThreeDigit = 1;
		else
			bThreeDigit = 0;
		pStr += 2;
		while (*pStr != ',') {
			if (*pStr == ';')
				pStr++;
			mnc = strtol (pStr, &pStr, 10);
			fprintf (out, "{%d,%d,%d},\n", mcc, bThreeDigit, mnc);
		}
	}
	fprintf (out, "{0, 0, 0}\n};\n");
	fclose (out);
	fclose (in);
}
