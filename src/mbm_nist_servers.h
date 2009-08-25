/*
 * mbm_nist_servers.h
 *
 * Copyright (C) 2009 Ericsson AB
 *
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
 * Description: List of official NIST servers supporting daytime protocol
 *
 * NOTE: Hard coding just one server is strictly prohibited and considered as a denial of service attack.
 *
 */

#ifndef MBM_NIST_SERVERS
#define MBM_NIST_SERVERS

typedef struct {
	char url[160];
	char ip[50];
	char desc[256];
} NIST_SERVER;

NIST_SERVER nist_servers[] = {

	{"time-a.nist.gov", "129.6.15.28", "NIST, Gaithersburg, Maryland"},
	{"time-b.nist.gov", "129.6.15.29", "NIST, Gaithersburg, Maryland"},
	{"time-a.timefreq.bldrdoc.gov", "132.163.4.101", "NIST, Boulder, Colorado"},
	{"time-b.timefreq.bldrdoc.gov", "132.163.4.102", "NIST, Boulder, Colorado"},
	{"time-c.timefreq.bldrdoc.gov", "132.163.4.103", "NIST, Boulder, Colorado"},
	{"utcnist.colorado.edu", "128.138.140.44",
	 "University of Colorado, Boulder"},
	{"time.nist.gov", "192.43.244.18", "NCAR, Boulder, Colorado"},
	{"time-nw.nist.gov", "131.107.13.100", "Microsoft, Redmond, Washington"},
	{"nist1.symmetricom.com", "69.25.96.13",
	 "Symmetricom, San Jose, California"},
	{"nist1-dc.WiTime.net", "206.246.118.250", "WiTime, Virginia"},
	{"nist1-ny.WiTime.net", "208.184.49.9", "WiTime, New York City"},
	{"nist1-sj.WiTime.net", "64.125.78.85", "WiTime, San Jose, California"},
	{"nist1.aol-ca.symmetricom.com", "207.200.81.113",
	 "Symmetricom, AOL facility, Sunnyvale, California"},
	{"nist1.aol-va.symmetricom.com", "64.236.96.53",
	 "Symmetricom, AOL facility, Virginia"},
	{"nist1.columbiacountyga.gov", "68.216.79.113", "Columbia County, Georgia"},
	{"nist.expertsmi.com", "71.13.91.122", "Monroe, Michigan"},
	{"nist.netservicesgroup.com", "64.113.32.5", "Southfield, Michigan"},
	{"", "", ""}
};

#endif
