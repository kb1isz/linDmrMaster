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

/*Source code inspired on the work of Paul Griffiths
  www.paulgriffiths.net
*/

#include "master_server.h"

bool * convertToBits(unsigned char packet[33]){
	static bool bits[264];
	unsigned char convPacket[33];
	unsigned char dmrPacket[100];
	int i,x;
	
	memset(dmrPacket,0,100);
	//bit invert
	for (i=0;i<33;i=i+2){
		convPacket[i] = *(packet + i + 1);
		convPacket[i+1] = *(packet +i);
		//sprintf(dmrPacket,"%s%02x:%02x:",dmrPacket,convPacket[i],convPacket[i+1]);
	}
	//syslog(LOG_NOTICE,"DMR packet:%s",dmrPacket);
	
	//convert to bits
	for (i=0;i<33;i++){
		for(x=0;x<8;x++){
			bits[(7-x) + (8 * i)] = (convPacket[i] >> x) & 1;
		}
	}
	
	return bits;
}