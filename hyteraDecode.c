/*
 *  Linux DMR Master server
    Copyright (C) 2014 Wim Hofman

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "master_server.h"

void decodeHyteraGps(unsigned char data[300]){

	unsigned char latitude[9],longitude[9];
	unsigned char speed[4],heading[4];

	memset(latitude,0,9);
	memset(longitude,0,10);
	memset(speed,0,4);
	memset(heading,0,4);

	memcpy(latitude,data+32,4);
	memcpy(latitude+4,data+38,4);
	memcpy(longitude,data+44,9);
	memcpy(speed,data+57,3);
	memcpy(heading,data+60,3);

	syslog(LOG_NOTICE,"Decoded GPS data: LAT(%s) LONG(%s) SPEED(%s) HEADING(%s)",latitude,longitude,speed,heading);
}
