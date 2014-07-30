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
//#define VCALL 4369                                        //HEX 1111
//#define DCALL 26214                                        //HEX 6666
//#define ISSYNC 61166										//HEX EEEE
//#define VCALLEND 8738										//HEX 2222
//#define CALL 2
//#define CALLEND 3
//#define PTYPE_ACTIVE1 2                                        
//#define PTYPE_END1 3
//#define PTYPE_ACTIVE2 66
//#define PTYPE_END2 67
#define VFRAMESIZE 72                                        //UDP PAYLOAD SIZE OF REPEATER VOICE/DATA TRAFFIC
//#define SYNC_OFFSET1 18                                        //UDP OFFSETS FOR VARIOUS BYTES IN THE DATA STREAM
//#define SYNC_OFFSET2 19                                        //
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

#define SLOT_TYPE_OFFSET1 18
#define SLOT_TYPE_OFFSET2 19
#define FRAME_TYPE_OFFSET1 22
#define FRAME_TYPE_OFFSET2 23

struct allow{
	bool repeater;
	bool sMaster;
	bool isRange;
};
void delRdacRepeater();
void delRepeater();
bool * convertToBits();
bool * decodeBPTC1969();

struct allow checkTalkGroup(int dstId, int slot, int callType){
	struct allow toSend = {0};
	int i;
	
	toSend.isRange = false;
	//First check sMaster talk groups. All sMaster talkgroups apply to the repeaters also
	if (slot == 1){
		for(i=0;i<master.sMasterTS1GroupCount;i++){
			if (dstId >= sMasterTS1List[i][0] && dstId <= sMasterTS1List[i][1]){
				toSend.repeater = true;
				toSend.sMaster = true;
				if (sMasterTS1List[i][0] != sMasterTS1List[i][1]) toSend.isRange = true;
				return toSend;
			}
		}
	}
	else{
		for(i=0;i<master.sMasterTS2GroupCount;i++){
			if (dstId >= sMasterTS2List[i][0] && dstId <= sMasterTS2List[i][1]){
				toSend.repeater = true;
				toSend.sMaster = true;
				if (sMasterTS2List[i][0] != sMasterTS2List[i][1]) toSend.isRange = true;
				return toSend;
			}
		}
	}
	
	if (slot == 1){
		for(i=0;i<master.repTS1GroupCount;i++){
			if (dstId >= repTS1List[i][0] && dstId <= repTS1List[i][1]){
				toSend.repeater = true;
				toSend.sMaster = false;
				if (repTS1List[i][0] != repTS1List[i][1]) toSend.isRange = true;
				return toSend;
			}
		}
	}
	else{
		for(i=0;i<master.repTS2GroupCount;i++){
			if (dstId >= repTS2List[i][0] && dstId <= repTS2List[i][1]){
				toSend.repeater = true;
				toSend.sMaster = false;
				if (repTS2List[i][0] != repTS2List[i][1]) toSend.isRange = true;
				return toSend;
			}
		}
	}
	toSend.repeater = false;
	toSend.sMaster = false;
	syslog(LOG_NOTICE,"Talk group %i not allowed on slot %i",dstId,slot);
	return toSend;
}

void echoTest(unsigned char buffer[VFRAMESIZE],int sockfd, struct sockaddr_in address, int srcId, int repPos){
	struct frame{
		unsigned char buf[VFRAMESIZE];
	};
	struct frame record[2000];
	long frames = 0;
	int n,rc,i;
	int slotType=0,frameType=0;
	fd_set fdMaster;
	struct timeval timeout;
	struct sockaddr_in cliaddr;
	socklen_t len;
	bool timedOut = false;
	FILE *referenceFile;
	FILE *recordFile;
	char fileName[100];

	sprintf(fileName,"%i.record",srcId);
	
	recordFile = fopen(fileName,"wb");
	
	FD_ZERO(&fdMaster);
	
	memcpy(record[frames].buf,buffer,VFRAMESIZE);
	fwrite(buffer,VFRAMESIZE,1,recordFile);
	len = sizeof(cliaddr);
	do{
		FD_SET(sockfd, &fdMaster);
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		if (rc = select(sockfd+1, &fdMaster, NULL, NULL, &timeout) == -1) { 
			close(sockfd);
			pthread_exit(NULL);
        }
		if (FD_ISSET(sockfd,&fdMaster)) {
			n = recvfrom(sockfd,buffer,VFRAMESIZE,0,(struct sockaddr *)&cliaddr,&len);
		
			if (n>2){
				slotType = buffer[SLOT_TYPE_OFFSET1] << 8 | buffer[SLOT_TYPE_OFFSET2];
				frameType = buffer[FRAME_TYPE_OFFSET1] << 8 | buffer[FRAME_TYPE_OFFSET2];
				if (frames < 2000){
					frames++;
					memcpy(record[frames].buf,buffer,VFRAMESIZE);
					fwrite(buffer,VFRAMESIZE,1,recordFile);
				} 
			}
		}
		else{
			timedOut = true;
		}
	}while ((slotType != 0x2222 && frameType != 0xaaaa) || timedOut == false);
	fclose(recordFile);
	sleep(1);
	syslog(LOG_NOTICE,"Playing echo back for radio %i",srcId);
	
	for (i=0;i<=frames;i++){
		sendto(sockfd,record[i].buf,VFRAMESIZE,0,(struct sockaddr *)&address,sizeof(address));
		slotType = record[i].buf[SLOT_TYPE_OFFSET1] << 8 | record[i].buf[SLOT_TYPE_OFFSET2];
		frameType = record[i].buf[FRAME_TYPE_OFFSET1] << 8 | record[i].buf[FRAME_TYPE_OFFSET2];
		if (slotType != 0xeeee && frameType != 0x1111) usleep(60000);
	}
	sprintf(fileName,"reference_%s.voice",repeaterList[repPos].language);
        if (referenceFile = fopen(fileName,"rb")){
		sleep(1);
		syslog(LOG_NOTICE,"Playing reference file %s",fileName);
		while (fread(buffer,VFRAMESIZE,1,referenceFile)){
			sendto(sockfd,buffer,VFRAMESIZE,0,(struct sockaddr *)&address,sizeof(address));
			slotType = buffer[SLOT_TYPE_OFFSET1] << 8 | buffer[SLOT_TYPE_OFFSET2];
			frameType = buffer[FRAME_TYPE_OFFSET1] << 8 | buffer[FRAME_TYPE_OFFSET2];
			if (slotType != 0xeeee && frameType != 0x1111) usleep(60000);
		}
	fclose(referenceFile);
	}
	else{
		syslog(LOG_NOTICE,"reference file %s not found",fileName); 
	}
}

void *dmrListener(void *f){
	int sockfd,n,i,rc;
	struct sockaddr_in servaddr,cliaddr;
	socklen_t len;
	unsigned char buffer[VFRAMESIZE];
	unsigned char response[VFRAMESIZE] ={0};
	//int repPos = (intptr_t)f;
	struct sockInfo* param = (struct sockInfo*) f;
	int repPos = param->port - baseDmrPort;
	struct sockaddr_in cliaddrOrg = param->address;
	int packetType = 0;
	int frameType = 0;
	int slotType = 0;
	int srcId = 0;
	int dstId = 0;
	int callType = 0;
	unsigned char slot = 0;
	fd_set fdMaster;
	struct timeval timeout;
	time_t timeNow,pingTime;
	struct allow toSend = {0};
	bool block[3];
	unsigned char sMasterFrame[103];
	char myId[11];
	unsigned char webUserInfo[100];
	//{0x00,0x00,0x00,0x00,0x32,0x30,0x34,0x31,0x39,0x2e,0x37}
	unsigned char dmrPacket[33];
	bool *bits;

	syslog(LOG_NOTICE,"DMR thread for port %i started",baseDmrPort + repPos);
	sockfd=socket(AF_INET,SOCK_DGRAM,0);
	repeaterList[repPos].sockfd = sockfd;
	repeaterList[repPos].dmrOnline = true;
	repeaterList[repPos].sending[1] = false;
	repeaterList[repPos].sending[2] = false;
	block[1] = false;
	block[2] = false;
	dmrState[1] = IDLE;
	dmrState[2] = IDLE;
	//create frame to append after packet for sMaster
	memset(sMasterFrame,0,103);
	memcpy(myId,(char*)&repeaterList[repPos].id,sizeof(int));
	memcpy(myId+4,master.ownCountryCode,4);
	memcpy(myId+7,master.ownRegion,1);
	memcpy(myId+8,version,3);
    
	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
	servaddr.sin_port=htons(baseDmrPort + repPos);
	bind(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr));
	FD_ZERO(&fdMaster);
	time(&pingTime);
	len = sizeof(cliaddr);
	
	for (;;){
		FD_SET(sockfd, &fdMaster);
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		if (rc = select(sockfd+1, &fdMaster, NULL, NULL, &timeout) == -1) { 
			syslog(LOG_NOTICE,"Select error, closing socket port %i",baseDmrPort + repPos);
			close(sockfd);
			pthread_exit(NULL);
        }
		
		if (FD_ISSET(sockfd,&fdMaster)) {
			n = recvfrom(sockfd,buffer,VFRAMESIZE,0,(struct sockaddr *)&cliaddr,&len);
		
			if (n>2){
				slot = buffer[SLOT_OFFSET1] / 16;
				if (dmrState[slot] == IDLE || repeaterList[repPos].sending[slot]){
					packetType = buffer[PTYPE_OFFSET];
					slotType = buffer[SLOT_TYPE_OFFSET1] << 8 | buffer[SLOT_TYPE_OFFSET2];
					frameType = buffer[FRAME_TYPE_OFFSET1] << 8 | buffer[FRAME_TYPE_OFFSET2];
					switch (packetType){
				
						case 0x02:
						if (slotType == 0xeeee && frameType == 0x1111 && dmrState[slot] != VOICE && block[slot] == false){ //Hytera voice sync packet
							//Sync packet is send before Voice LC header and every time the embedded LC (4 packets) in a voice superframe has been send
							//When voice call starts, this is the first packet where we can see src and dst)
							sMasterFrame[98] = slot;
							srcId = buffer[SRC_OFFSET3] << 16 | buffer[SRC_OFFSET2] << 8 | buffer[SRC_OFFSET1];
							dstId = buffer[DST_OFFSET3] << 16 | buffer[DST_OFFSET2] << 8 | buffer[DST_OFFSET1];
							callType = buffer[TYP_OFFSET1];
							repeaterList[repPos].sending[slot] = true;
							sprintf(webUserInfo,"RX_Slot=%i,GROUP=%i,USER_ID=%i,TYPE=Voice,VERS=%s,RPTR=%i,%s\n",slot,dstId,srcId,version,repeaterList[repPos].id,master.ownName);
							if (sMaster.online){
								sendto(sMaster.sockfd,webUserInfo,strlen(webUserInfo),0,(struct sockaddr *)&sMaster.address,sizeof(sMaster.address));
							}
							if (dstId == echoId){
								syslog(LOG_NOTICE,"[%i-%s]Echo test started on slot %i src %i",baseDmrPort + repPos,repeaterList[repPos].callsign,slot,srcId);
								echoTest(buffer,sockfd,repeaterList[repPos].address,srcId,repPos);
								repeaterList[repPos].sending[slot] = false;
								break;
							} 
							toSend = checkTalkGroup(dstId,slot,callType);
							if (toSend.repeater == false){
								block[slot] = true;
								break;
							}
							if(toSend.isRange && dstId != master.ownCCInt){
								memcpy(sMasterFrame+90,(char*)&master.ownCCInt,sizeof(int));
							}
							else{
								memset(sMasterFrame+90,0,4);
							}
							dmrState[slot] = VOICE;
							syslog(LOG_NOTICE,"[%i-%s]Voice call started on slot %i src %i dst %i type %i",baseDmrPort + repPos,repeaterList[repPos].callsign,slot,srcId,dstId,callType);
							//break;
						}
						break;
						
						case 0x01:
						if (slotType == 0x3333 && dmrState[slot] != DATA){  //CSBK (first slot type for data where we can see src and dst)
							srcId = buffer[SRC_OFFSET3] << 16 | buffer[SRC_OFFSET2] << 8 | buffer[SRC_OFFSET1];
							dstId = buffer[DST_OFFSET3] << 16 | buffer[DST_OFFSET2] << 8 | buffer[DST_OFFSET1];
							callType = buffer[TYP_OFFSET1];
							toSend.sMaster = false;
							if (dstId == rrsGpsId) toSend.repeater = false;
							break;
						}
						
						memcpy(dmrPacket,buffer+26,34);  //copy the dmr part out of the Hyetra packet
						bits = convertToBits(dmrPacket); //convert it to bits
						
						if (slotType == 0x4444){  //Data header
							syslog(LOG_NOTICE,"[%i-%s]Data header on slot %i src %i dst %i type %i",baseDmrPort + repPos,repeaterList[repPos].callsign,slot,srcId,dstId,callType);
							dmrState[slot] = DATA;
							repeaterList[repPos].sending[slot] = true;
							//decodeBPTC1969(bits);
							break;
						}
						
						if (slotType == 0x5555 && dmrState[slot] == DATA){ // 1/2 rate data continuation
							//put idle when blocks to follow equals
							dmrState[slot] = IDLE;
							repeaterList[repPos].sending[slot] = false;
							syslog(LOG_NOTICE,"[%i-%s]1/2 rate data continuation on slot %i src %i dst %i type %i",baseDmrPort + repPos,repeaterList[repPos].callsign,slot,srcId,dstId,callType);
							break;
						}
						if (slotType == 0x6666 && dmrState[slot] == DATA){ // 3/4 rate data continuation
							syslog(LOG_NOTICE,"[%i-%s]3/4 rate data continuation on slot %i src %i dst %i type %i",baseDmrPort + repPos,repeaterList[repPos].callsign,slot,srcId,dstId,callType);
							//put idle when blocks to follow equals
							dmrState[slot] = IDLE;
							repeaterList[repPos].sending[slot] = false;
							break;
						}
						break;
						
						case 0x03:
						if (slotType == 0x2222){  //Terminator with LC
							dmrState[slot] = IDLE;
							repeaterList[repPos].sending[slot] = false;
							syslog(LOG_NOTICE,"[%i-%s]Voice call ended on slot %i",baseDmrPort + repPos,repeaterList[repPos].callsign,slot);
							if (block[slot] == true) syslog(LOG_NOTICE,"[%i-%s] But was blocked because of not allowed talk group",baseDmrPort + repPos,repeaterList[repPos].callsign);
							block[slot] = false;
						}
						break;
					}
					if (!block[slot]){
						for (i=0;i<highestRepeater;i++){
							if (repeaterList[i].address.sin_addr.s_addr !=0 && repeaterList[i].address.sin_addr.s_addr != cliaddrOrg.sin_addr.s_addr){
								sendto(repeaterList[i].sockfd,buffer,n,0,(struct sockaddr *)&repeaterList[i].address,sizeof(repeaterList[i].address));
							}
						}
						if (toSend.sMaster && sMaster.online){
							memcpy(sMasterFrame,buffer,n);
							memcpy(sMasterFrame + n,myId,11);
							sendto(sMaster.sockfd,sMasterFrame,103,0,(struct sockaddr *)&sMaster.address,sizeof(sMaster.address));
						}
					}
				}
				else{
					syslog(LOG_NOTICE,"[%i-%s]Incomming traffic on slot %i, but DMR not IDLE",baseDmrPort + repPos,repeaterList[repPos].callsign,slot);
				}
			}
			else{
				response[0] = 0x41;
				sendto(sockfd,response,n,0,(struct sockaddr *)&cliaddr,sizeof(cliaddr));
				time(&pingTime);
			}
		}
		else{
			if (restart){
				syslog(LOG_NOTICE,"Exiting dmr thread (restart)");
				close(sockfd);
				pthread_exit(NULL);
			}
			time(&timeNow);
			if (repeaterList[repPos].sending[1] && dmrState[1] != IDLE){
				if (dmrState[1] == VOICE) syslog(LOG_NOTICE,"[%i-%s]Voice call ended after timeout on slot 1",baseDmrPort + repPos,repeaterList[repPos].callsign);
				if (dmrState[1] == DATA) syslog(LOG_NOTICE,"[%i-%s]Data call ended after timeout on slot 1",baseDmrPort + repPos,repeaterList[repPos].callsign);
				dmrState[1] = IDLE;
				repeaterList[repPos].sending[1] = false;
				block[1] = false;
				syslog(LOG_NOTICE,"[%i-%s]Slot 1 IDLE",baseDmrPort + repPos,repeaterList[repPos].callsign);
			}
			if (repeaterList[repPos].sending[2] && dmrState[2] != IDLE){
				if (dmrState[2] == VOICE) syslog(LOG_NOTICE,"[%i-%s]Voice call ended after timeout on slot 2",baseDmrPort + repPos,repeaterList[repPos].callsign);
				if (dmrState[2] == DATA) syslog(LOG_NOTICE,"[%i-%s]Data call ended after timeout on slot 2",baseDmrPort + repPos,repeaterList[repPos].callsign);
				dmrState[2] = IDLE;
				repeaterList[repPos].sending[2] = false;
				block[2] = false;
				syslog(LOG_NOTICE,"[%i-%s]Slot 2 IDLE",baseDmrPort + repPos,repeaterList[repPos].callsign);
			}
			if (difftime(timeNow,pingTime) > 60 && !repeaterList[repPos].sending[slot]){
				syslog(LOG_NOTICE,"PING timeout on DMR port %i repeater %s, exiting thread",baseDmrPort + repPos,repeaterList[repPos].callsign);
				syslog(LOG_NOTICE,"Removing repeater from list position %i",repPos);
				delRepeater(cliaddrOrg);
				if (repPos + 1 == highestRepeater) highestRepeater--;
				delRdacRepeater(cliaddrOrg);
				close(sockfd);
				pthread_exit(NULL);
			}
		}
	}
}
