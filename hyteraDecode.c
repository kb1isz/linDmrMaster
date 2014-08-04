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
void sendAprs();

void decodeHyteraGps(int radioId,struct repeater repeater, unsigned char data[300]){

	struct gpsCoordinates gpsData = {0};
	regex_t regex;
	int reti;

	memcpy(gpsData.latitude,data+33,3);
	memcpy(gpsData.latitude+3,data+38,4);
	memcpy(gpsData.latitude+7,data+32,1);
	memcpy(gpsData.longitude,data+45,8);
	memcpy(gpsData.longitude+8,data+44,1);
	memcpy(gpsData.speed,data+57,3);
	memcpy(gpsData.heading,data+60,3);

	syslog(LOG_NOTICE,"[%s]Decoded GPS data(Hytera): LAT(%s) LONG(%s) SPEED(%s) HEADING(%s)",repeater.callsign,gpsData.latitude,gpsData.longitude,gpsData.speed,gpsData.heading);
	
	reti = regcomp(&regex, "^[0-9][0-9][0-9][0-9][.][0-9][0-9][NZ]$", 0);
	if(reti){
		syslog(LOG_NOTICE,"[%s]Hyt GPS decode,could not compile regex latitude",repeater.callsign);
		return;
	}
	reti = regexec(&regex,gpsData.latitude,0,NULL,0);
	if(reti == REG_NOMATCH){
		syslog(LOG_NOTICE,"[%s]Corrupt latitude received",repeater.callsign);
		regfree(&regex);
		return;
	}
	regfree(&regex);

	reti = regcomp(&regex, "^[0-9][0-9][0-9][0-9][0-9][.][0-9][0-9][EW]$", 0);
	if(reti){
		syslog(LOG_NOTICE,"[%s]Hyt GPS decode,could not compile regex longitude",repeater.callsign);
		regfree(&regex);
		return;
	}
	reti = regexec(&regex,gpsData.longitude,0,NULL,0);
	if(reti == REG_NOMATCH){
		syslog(LOG_NOTICE,"[%s]Corrupt longitude received",repeater.callsign);
		regfree(&regex);
		return;
	}
	regfree(&regex);

	sendAprs(gpsData,radioId,repeater);
}

void decodeHyteraGpsCompressed(int radioId,struct repeater repeater, unsigned char data[300]){

	syslog(LOG_NOTICE,"[%s]Compressed GPS data(Hytera) not decoding",repeater.callsign);
}
