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

void decodeHyteraGps(int radioId,int repeaterId, unsigned char data[300]){

	struct gpsCoordinates gpsData = {0};


	memcpy(gpsData.latitude,data+32,4);
	memcpy(gpsData.latitude+4,data+38,4);
	memcpy(gpsData.longitude,data+44,9);
	memcpy(gpsData.speed,data+57,3);
	memcpy(gpsData.heading,data+60,3);

	syslog(LOG_NOTICE,"Decoded GPS data(Hytera): LAT(%s) LONG(%s) SPEED(%s) HEADING(%s)",gpsData.latitude,gpsData.longitude,gpsData.speed,gpsData.heading);
}

void decodeHyteraGpsCompressed(int radioId,int repeaterId, unsigned char data[300]){

	syslog(LOG_NOTICE,"Compressed GPS data(Hytera) not decoding");
}
