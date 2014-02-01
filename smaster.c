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

#define NUMSLOTS 2                                        //DMR IS 2 SLOT
#define SLOT1 4369                                        //HEX 1111
#define SLOT2 8738                                        //HEX 2222
#define VCALL 4369                                        //HEX 1111
#define DCALL 26214                                        //HEX 6666
#define ISSYNC 61166										//HEX EEEE
#define VCALLEND 8738										//HEX 2222
//#define CALL 2
//#define CALLEND 3
#define PTYPE_ACTIVE1 2                                        
#define PTYPE_END1 3
#define PTYPE_ACTIVE2 66
#define PTYPE_END2 67
#define VFRAMESIZE 103                                        //UDP PAYLOAD SIZE OF sMaster
#define SYNC_OFFSET1 18                                        //UDP OFFSETS FOR VARIOUS BYTES IN THE DATA STREAM
#define SYNC_OFFSET2 19                                        //
//#define SYNC_OFFSET3 18                                        //Look for EEEE
//#define SYNC_OFFSET4 19                                        //Look for EEEE
#define SLOT_OFFSET1 16                                        //        
#define SLOT_OFFSET2 17
#define PTYPE_OFFSET 8
#define SRC_OFFSET1 68
#define SRC_OFFSET2 69
#define SRC_OFFSET3 70
#define DST_OFFSET1 64
#define DST_OFFSET2 65
#define DST_OFFSET3 66
#define TYP_OFFSET1 62

struct masterData sMaster = {0};

struct allow{
	bool repeater;
	bool sMaster;
	bool isRange;
};

struct allow checkTalkGroup();

void *sMasterThread(){
	int sockfd = 0;
	char ping[50];
	unsigned char buffer[VFRAMESIZE];
	unsigned char repInfo[200];
	int n,rc,i;
	fd_set fdMaster;
	struct timeval timeout;
	time_t timeNow,needPingTime,reportTime,pongTime;
	int packetType = 0;
	unsigned char slot = 0;
	int sync = 0;
	int srcId = 0;
	int dstId = 0;
	int callType = 0;
	unsigned char origC[5][3];
	struct sockaddr_in servaddr,cliaddr;
	struct allow toSend = {0};
	bool block[3];
	
	syslog(LOG_NOTICE,"Starting sMaster thread");
	block[1] = false;
	block[2] = false;
	
	sockfd=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr=INADDR_ANY;
	servaddr.sin_port=htons(atoi(master.sMasterPort));
	bind(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr));
	servaddr.sin_addr.s_addr=inet_addr(master.sMasterIp);
    	
	sMaster.address = servaddr;
	sMaster.sockfd = sockfd;
	sMaster.online = true;
	sprintf(ping,"PING%s%s%s ",master.ownCountryCode,master.ownRegion,version);
	time(&needPingTime);
	sendto(sockfd,ping,strlen(ping),0,(struct sockaddr *)&servaddr,sizeof(servaddr));
	time(&reportTime);
	FD_ZERO(&fdMaster);
	
	for(;;){
		FD_SET(sockfd, &fdMaster);
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		if (rc = select(sockfd+1, &fdMaster, NULL, NULL, &timeout) == -1) { 
			syslog(LOG_NOTICE,"Select error in sMaster thread");
			close(sockfd);
			//need to reconnect here !!!!
        }
		
		if (FD_ISSET(sockfd,&fdMaster)) {
			n = recv(sockfd,buffer,VFRAMESIZE,0);
			if (n<VFRAMESIZE){
				if (strstr(buffer,"RREQ")){
					syslog(LOG_NOTICE,"RREQ from sMaster");
					char rreqResponse[200];
					char myCCInfo[21];
					sprintf(myCCInfo,"%s%s",master.ownCountryCode,master.ownRegion);
					sprintf(rreqResponse,"TS1R%-20s%-20s%s",myCCInfo,master.ownName,master.announcedCC1);
					syslog(LOG_NOTICE,"RREQ response %s",rreqResponse);
					sendto(sockfd,rreqResponse,strlen(rreqResponse),0,(struct sockaddr *)&servaddr,sizeof(servaddr));
					sprintf(rreqResponse,"TS2R%-20s%-20s%s",myCCInfo,master.ownName,master.announcedCC2);
					syslog(LOG_NOTICE,"RREQ response %s",rreqResponse);
					sendto(sockfd,rreqResponse,strlen(rreqResponse),0,(struct sockaddr *)&servaddr,sizeof(servaddr));
					unsigned char ip[INET_ADDRSTRLEN];
					for (i=0;i<highestRepeater;i++){
						if (repeaterList[i].address.sin_addr.s_addr !=0){
							inet_ntop(AF_INET, &(repeaterList[i].address.sin_addr), ip, INET_ADDRSTRLEN);
							sprintf(repInfo,"RPT%02i%-15s%i%i%i%i%-4s%-16s%-9s%-6s0     0     %-10s%-12s%-3s",i,ip,repeaterList[i].id,baseDmrPort+i,
							master.ownCCInt,master.ownRegionInt,version,repeaterList[i].callsign,repeaterList[i].txFreq,repeaterList[i].shift,
							repeaterList[i].hardware,repeaterList[i].firmware,repeaterList[i].mode);
							sendto(sockfd,repInfo,strlen(repInfo),0,(struct sockaddr *)&servaddr,sizeof(servaddr));
							syslog(LOG_NOTICE,"RREQ response %s",repInfo);
							usleep(60000);
						}
					}
					time(&reportTime);
				}
				if (strstr(buffer,"PONG")){
					time(&pongTime);
				}
			}
			else{
				slot = buffer[SLOT_OFFSET1] / 16;
				if (dmrState[slot] == IDLE || sMaster.sending[slot]){
					packetType = buffer[PTYPE_OFFSET];
					sync = buffer[SYNC_OFFSET1] << 8 | buffer[SYNC_OFFSET2];
					switch (packetType){
				
						case 0x01:
						if (sync == VCALL && dmrState[slot] != VOICE && block[slot] == false){
							srcId = buffer[SRC_OFFSET3] << 16 | buffer[SRC_OFFSET2] << 8 | buffer[SRC_OFFSET1];
							dstId = buffer[DST_OFFSET3] << 16 | buffer[DST_OFFSET2] << 8 | buffer[DST_OFFSET1];
							callType = buffer[TYP_OFFSET1];
							toSend = checkTalkGroup(dstId,slot,callType);
							if (toSend.repeater == false){
								block[slot] = true;
								break;
							}
							sMaster.sending[slot] = true;
							dmrState[slot] = VOICE;
							syslog(LOG_NOTICE,"[sMaster]Voice call started on slot %i src %i dst %i type %i",slot,srcId,dstId,callType);
							if (buffer[90] !=0) {
								memcpy(origC[slot],buffer+90,4);
								syslog(LOG_NOTICE,"[sMaster]Replacing country code to %s",origC[slot]);
						{description}
    Copyright (C) {year} {fullname}

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
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.	} 
							else{ 
								memcpy(origC[slot],buffer+64,4);
							}
							break;
						}
						if (sync == DCALL && dmrState[slot] != DATA){
							srcId = buffer[SRC_OFFSET3] << 16 | buffer[SRC_OFFSET2] << 8 | buffer[SRC_OFFSET1];
							dstId = buffer[DST_OFFSET3] << 16 | buffer[DST_OFFSET2] << 8 | buffer[DST_OFFSET1];
							callType = buffer[TYP_OFFSET1];
							syslog(LOG_NOTICE,"[sMaster]Data on slot %i src %i dst %i type %i",slot,srcId,dstId,callType);
							break;
						}
						break;
				
						case 0x03:
						if (sync == VCALLEND){
							dmrState[slot] = IDLE;
							sMaster.sending[slot] = false;
							block[slot] = false;
							syslog(LOG_NOTICE,"[sMaster]Voice call ended on slot %i",slot);
						}
						break;
					}
					memcpy(buffer+64,origC[slot],4);
					if (!block[slot]){
						for (i=0;i<highestRepeater;i++){
							if (repeaterList[i].address.sin_addr.s_addr !=0 && !repeaterList[i].sending[slot]){
								sendto(repeaterList[i].sockfd,buffer,72,0,(struct sockaddr *)&repeaterList[i].address,sizeof(repeaterList[i].address));
							}
						}
					}
				}
			}
		}
		else{
			time(&timeNow);
			if (sMaster.sending[1] && dmrState[1] != IDLE){
				dmrState[1] = IDLE;
				sMaster.sending[1] = false;
				block[1] = false;
				syslog(LOG_NOTICE,"[sMaster]Slot 1 IDLE");
			}
			if (sMaster.sending[2] && dmrState[2] != IDLE){
				dmrState[2] = IDLE;
				sMaster.sending[2] = false;
				block[2] = false;
				syslog(LOG_NOTICE,"[sMaster]Slot 2 IDLE");
			}
			if (difftime(timeNow,needPingTime) > 5){
				time(&needPingTime);
				sendto(sockfd,ping,strlen(ping),0,(struct sockaddr *)&servaddr,sizeof(servaddr));
			}
			if (difftime(timeNow,reportTime) > 320){
				char rreqResponse[200];
				char myCCInfo[21];
				sprintf(myCCInfo,"%s%s",master.ownCountryCode,master.ownRegion);
				sprintf(rreqResponse,"TS1R%-20s%-20s%s",myCCInfo,master.ownName,master.announcedCC1);
				syslog(LOG_NOTICE,"Report to sMaster %s",rreqResponse);
				sendto(sockfd,rreqResponse,strlen(rreqResponse),0,(struct sockaddr *)&servaddr,sizeof(servaddr));
				sprintf(rreqResponse,"TS2R%-20s%-20s%s",myCCInfo,master.ownName,master.announcedCC2);
				syslog(LOG_NOTICE,"Report to sMaster %s",rreqResponse);
				sendto(sockfd,rreqResponse,strlen(rreqResponse),0,(struct sockaddr *)&servaddr,sizeof(servaddr));
				unsigned char ip[INET_ADDRSTRLEN];
				for (i=0;i<highestRepeater;i++){
					if (repeaterList[i].address.sin_addr.s_addr !=0){
						inet_ntop(AF_INET, &(repeaterList[i].address.sin_addr), ip, INET_ADDRSTRLEN);
							sprintf(repInfo,"RPT%02i%-15s%i%i%i%i%-4s%-16s%-9s%-6s0     0     %-10s%-12s%-3s",i,ip,repeaterList[i].id,baseDmrPort+i,
							master.ownCCInt,master.ownRegionInt,version,repeaterList[i].callsign,repeaterList[i].txFreq,repeaterList[i].shift,
							repeaterList[i].hardware,repeaterList[i].firmware,repeaterList[i].mode);
						sendto(sockfd,repInfo,strlen(repInfo),0,(struct sockaddr *)&servaddr,sizeof(servaddr));
						syslog(LOG_NOTICE,"Report to sMaster %s",repInfo);
						usleep(60000);
					}
				}
				time(&reportTime);
			}
		}
	}
}