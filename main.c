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

int servicePort = 50000;
int dmrPort = 50001;
int baseDmrPort = 50100;
int rdacPort = 50002;
int baseRdacPort = 50200;
int maxRepeaters = 20;
int echoId = 9990;
int rrsGpsId = 500;
char version[5] = "1.0";

struct repeater repeaterList[100] = {0};
struct repeater emptyRepeater = {0};
struct masterInfo master;
static const struct masterInfo emptyMaster = {0};
struct ts tsInfo = {0};
struct sockaddr_in discardList[100] = {0};

int rdacSock=0;
int highestRepeater = 0;
int restart = 0;

sqlite3 *db;
sqlite3 *openDatabase();

int setRdacRepeater();
int findRdacRepeater();

state dmrState[3];

int (*sMasterTS1List)[2];
int (*sMasterTS2List)[2];
int (*repTS1List)[2];
int (*repTS2List)[2];



void *dmrListener();
void *rdacListener();
void *sMasterThread();
void *webServerListener();

void discard(struct sockaddr_in address){
	int i;
	for(i=0;i<maxRepeaters;i++){
		if (discardList[i].sin_addr.s_addr == address.sin_addr.s_addr) return;
	}
	//If not found, find first empty position in the list
	for(i=0;i<maxRepeaters;i++){
		if (discardList[i].sin_addr.s_addr == 0) break;
	}
	
	//oops, max repeaters reached
	if (i == maxRepeaters){
		syslog(LOG_NOTICE,"Not possible to add repeater in discard list, maximum number reached");
		return;
	}
	discardList[i] = address;
}

bool isDiscarded(struct sockaddr_in address){
	int i;
	for(i=0;i<maxRepeaters;i++){
		if (discardList[i].sin_addr.s_addr == address.sin_addr.s_addr) return true;
	}
	return false;
}

int initRepeater(struct repeater repeaterInfo){
	//Initialize a repeater in the DMR list
	int i;

	//If found based on IP, just return it's position in the list
	for(i=0;i<maxRepeaters;i++){
		if (repeaterList[i].address.sin_addr.s_addr == repeaterInfo.address.sin_addr.s_addr) return i;
	}
	
	//If not found, find first empty position in the list
	for(i=0;i<maxRepeaters;i++){
		if (repeaterList[i].address.sin_addr.s_addr == 0) break;
	}
	
	//oops, max repeaters reached
	if (i == maxRepeaters){
		syslog(LOG_NOTICE,"Not possible to add repeater, maximum number reached");
		return 99;
	}
	//Set all the needed info for a repeater
	repeaterInfo.address.sin_port=htons(dmrPort);
	repeaterList[i].address = repeaterInfo.address;
	repeaterList[i].id = repeaterInfo.id;
	sprintf(repeaterList[i].callsign,"%s",repeaterInfo.callsign);
	sprintf(repeaterList[i].txFreq,"%s",repeaterInfo.txFreq);
	sprintf(repeaterList[i].shift,"%s",repeaterInfo.shift);
	sprintf(repeaterList[i].hardware,"%s",repeaterInfo.hardware);
	sprintf(repeaterList[i].firmware,"%s",repeaterInfo.firmware);
	sprintf(repeaterList[i].mode,"%s",repeaterInfo.mode);
	sprintf(repeaterList[i].language,"%s",repeaterInfo.language);
	syslog(LOG_NOTICE,"Repeater added to list position %i",i);
	//Highest filled position in the list
	if (i +1 > highestRepeater) highestRepeater = i + 1;
	return i;
}

int findRepeater(struct sockaddr_in address){
	//Find a repeater in the DMR list
	int i;

	for(i=0;i<maxRepeaters;i++){
		if (repeaterList[i].address.sin_addr.s_addr == address.sin_addr.s_addr) return i;
	}
	//If not found
	return 99;
}

void delRepeater(struct sockaddr_in address){
        int i;

        for(i=0;i<maxRepeaters;i++){
                if (repeaterList[i].address.sin_addr.s_addr == address.sin_addr.s_addr){
                        repeaterList[i].address.sin_addr.s_addr = 0;
                        repeaterList[i].rdacOnline = false;
                        repeaterList[i].rdacUpdated = false;
                        repeaterList[i].dmrOnline = false;
                        repeaterList[i].id = 0;
                        repeaterList[i].lastPTPPConnect = 0;
                        repeaterList[i].lastDMRConnect = 0;
                        repeaterList[i].lastRDACConnect = 0;
                        memset(repeaterList[i].callsign,0,17);
                        memset(repeaterList[i].txFreq,0,10);
                        memset(repeaterList[i].shift,0,7);
                        memset(repeaterList[i].hardware,0,11);
                        memset(repeaterList[i].firmware,0,14);
                        memset(repeaterList[i].mode,0,4);
                        syslog(LOG_NOTICE,"Repeater deleted from list pos %i",i);
                        return;
                }
        }

}

void serviceListener(port){
	
	pthread_t thread;
	int sockfd,n,i,rc;
	struct sockaddr_in servaddr,cliaddr;
	socklen_t len;
	unsigned char buffer[500];
	unsigned char response[500] ={0};
	unsigned char command[] = {0x50,0x32,0x50,0x50};
	unsigned char ping[] = {0x5a,0x5a,0x5a,0x5a};
	unsigned char ping2[] = {0xe8,0x40,0xb4,0x10};
	char str[INET_ADDRSTRLEN];
	int redirectPort;
	int repPos;
	struct repeater repeaterInfo;
	sqlite3_stmt *stmt;
	unsigned char SQLQUERY[200] = {0};
	fd_set fdService;
	struct timeval timeout;
	time_t timeNow;

	syslog(LOG_NOTICE,"Listener for port %i started",port);

	//Create the socket to listen to the service port
	sockfd=socket(AF_INET,SOCK_DGRAM,0);
	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
	servaddr.sin_port=htons(port);
	bind(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr));
	FD_ZERO(&fdService);
	
	for (;;){
		FD_SET(sockfd, &fdService);
		timeout.tv_sec = 2;
		timeout.tv_usec = 0;
		rc = select(sockfd+1, &fdService, NULL, NULL, &timeout);
		if (FD_ISSET(sockfd,&fdService)) {
			len = sizeof(cliaddr);
			memset(&buffer,0,500);
			n = recvfrom(sockfd,buffer,500,0,(struct sockaddr *)&cliaddr,&len);
			if (memcmp(buffer,command,sizeof(command)) == 0){  //See if what is received is a command or heartbeat
				inet_ntop(AF_INET, &(cliaddr.sin_addr), str, INET_ADDRSTRLEN);
				time(&timeNow);
				switch (buffer[20]){
					int rdacPos;
					case 0x10:{  //PTPP request received
					if(isDiscarded(cliaddr)) continue;
					rdacPos = setRdacRepeater(cliaddr);
					if (difftime(timeNow,rdacList[rdacPos].lastPTPPConnect) < 10) continue;  //Ignore connect request
					syslog(LOG_NOTICE,"PTPP request from repeater [%s]",str);
					memcpy(response,buffer,n);
					//Assign device ID
					response[4]++;
					response[13]=0x01;
					response[n] = 0x01;
					sendto(sockfd,response,n+1,0,(struct sockaddr *)&cliaddr,sizeof(cliaddr));
					syslog(LOG_NOTICE,"Assigned PTPP device 1 to repeater [%s]",str);
					time(&rdacList[rdacPos].lastPTPPConnect);
					break;}
				
					case 0x11:{  //Request to startup DMR received
					int rdacPos;
					//See if the repeater is already known in the RDAC list
					if(isDiscarded(cliaddr)) continue;
					rdacPos = findRdacRepeater(cliaddr);
					if (difftime(timeNow,rdacList[rdacPos].lastDMRConnect) < 10) continue;  //Ignore connect request
					time(&rdacList[rdacPos].lastDMRConnect);
					syslog(LOG_NOTICE,"DMR request from repeater [%s]",str);
					if (rdacPos == 99){  //If  not ignore the DMR request
						syslog(LOG_NOTICE,"DMR request from repeater not in RDAC list [%s], ignoring",str);
						continue;
					}
					//See if RDAC info from the repeater has already been received
					if (!rdacList[rdacPos].id > 0){ //If not ignore DMR request
						syslog(LOG_NOTICE,"RDAC info not received from repeater yet [%s], ignoring",str);
						continue;
					}
					//Now that we have repeater info, initialize the repeater
					repPos = initRepeater(rdacList[rdacPos]);
					if (repPos == 99) continue;   //If 99 returned, more repeaters then allowed
					memcpy(response,buffer,n);
					//Assign device ID
					response[4]++;
					response[13]=0x01;
					response[n] = 0x01;
					cliaddr.sin_port=htons(port);
					sendto(sockfd,response,n+1,0,(struct sockaddr *)&cliaddr,sizeof(cliaddr));
					syslog(LOG_NOTICE,"Assigned DMR device 1 to repeater [%s - %s]",str,repeaterList[repPos].callsign);
					//Assign port number
					response[4] = 0x0b;
					unsigned char rep[] = {0x00,0xff,0x01,0x00};
					memcpy(response + 12,rep,4);
					unsigned char rport[] = {0xff,0x01,0x00,0x00};
					redirectPort = baseDmrPort + repPos; 
					rport[2] = redirectPort;
					rport[3] = redirectPort >> 8;
					memcpy(response + n,rport,4);
					if (repeaterList[repPos].dmrOnline){  //If repeater is not offline, but we still get a request, just point it back to old thread
						syslog(LOG_NOTICE,"DMR request from repeater [%s - %s] already assigned a DMR port, not starting thread",str,repeaterList[repPos].callsign);
					}
					else{  //Start a new DMR thread for this repeater
                                                struct sockInfo *param = malloc(sizeof(struct sockInfo));
                                                param->address = cliaddr;
                                                param->port = redirectPort;
						pthread_create(&thread, NULL, dmrListener,param);
					}
					sendto(sockfd,response,n+4,0,(struct sockaddr *)&cliaddr,sizeof(cliaddr));
					syslog(LOG_NOTICE,"Re-directed repeater [%s - %s] to DMR port %i",str,repeaterList[repPos].callsign,redirectPort);
					break;}

					case 0x12:{  ////Request to startup RDAC received
					int rdacPos;
					//Initialize this repeater for RDAC
					if(isDiscarded(cliaddr)) continue;
					rdacPos = setRdacRepeater(cliaddr);
					if (difftime(timeNow,rdacList[rdacPos].lastRDACConnect) < 10) continue;  //Ignore connect request
					syslog(LOG_NOTICE,"RDAC request from repeater [%s]",str);
					if (rdacPos == 99) continue;   //If 99 returned, more repeaters then allowed
					memcpy(response,buffer,n);
					//Assign device ID
					response[4]++;
					response[13]=0x01;
					response[n] = 0x01;
					cliaddr.sin_port=htons(port);
					sendto(sockfd,response,n+1,0,(struct sockaddr *)&cliaddr,sizeof(cliaddr));
					syslog(LOG_NOTICE,"Assigned RDAC device 1 to repeater [%s]",str);
					//Assign port number
					response[4] = 0x0b;
					unsigned char rep[] = {0xff,0xff,0x01,0x00};
					memcpy(response + 12,rep,4);
					unsigned char port[] = {0xff,0x01,0x00,0x00};
					redirectPort = baseRdacPort + (rdacPos * 2);
					port[2] = redirectPort;
					port[3] = redirectPort >> 8;
					memcpy(response + n,port,4);
					if (rdacList[rdacPos].rdacOnline){  //If repeater is not offline, but we still get a request, just point it back to old thread
						syslog(LOG_NOTICE,"RDAC request from repeater [%s - %s] already assigned a RDAC port, not starting thread",str,rdacList[rdacPos].callsign);
					}
					else{  //Start a new RDAC thread for this repeater
						struct sockInfo *param = malloc(sizeof(struct sockInfo));
						param->address = cliaddr;
						param->port = redirectPort;
						pthread_create(&thread, NULL, rdacListener,param);
					}
					sendto(sockfd,response,n+4,0,(struct sockaddr *)&cliaddr,sizeof(cliaddr));
					syslog(LOG_NOTICE,"Re-directed repeater [%s] to RDAC port %i",str,redirectPort);
					time(&rdacList[rdacPos].lastRDACConnect);
					break;}
				}
			}
		
			if ((memcmp(buffer,ping,sizeof(ping)) == 0 || memcmp(buffer,ping2,sizeof(ping2)) == 0) && repeaterList[findRepeater(cliaddr)].dmrOnline){//Is this a heartbeat from a repeater on the service port ? And do we know the repeater ?
				memcpy(response,buffer,n);
				response[12]++;
				sendto(sockfd,response,n,0,(struct sockaddr *)&cliaddr,sizeof(cliaddr));
				if (restart){
					syslog(LOG_NOTICE,"Exiting serviceListener (restart)");
					return;
				}
			}
		}
		else{
			if (restart){
				syslog(LOG_NOTICE,"Exiting serviceListener (restart)");
				return;
			}
		}
	}
}

int getMasterInfo(){
	//Get the needed info from the SQLITE database
	
	unsigned char SQLQUERY[200] = {0};
	sqlite3_stmt *stmt;
	
	sprintf(SQLQUERY,"SELECT ownName,ownCountryCode,ownRegion,sMasterIp,sMasterPort FROM sMaster");
	if (sqlite3_prepare_v2(db,SQLQUERY,-1,&stmt,0) == 0){
		if (sqlite3_step(stmt) == SQLITE_ROW){
			sprintf(master.ownName,"%s",sqlite3_column_text(stmt,0));
			sprintf(master.ownCountryCode,"%s",sqlite3_column_text(stmt,1));
			sprintf(master.ownRegion,"%s",sqlite3_column_text(stmt,2));
			sprintf(master.sMasterIp,"%s",sqlite3_column_text(stmt,3));
			sprintf(master.sMasterPort,"%s",sqlite3_column_text(stmt,4));
			master.ownCCInt = atoi(sqlite3_column_text(stmt,1));
			master.ownRegionInt = atoi(sqlite3_column_text(stmt,2));
		}
		else{
			syslog(LOG_NOTICE,"failed to read sMasterInfo, no row");
			return 0;
		}
	}
	else{
		syslog(LOG_NOTICE,"failed to read sMasterInfo, query bad");
		return 0;
	}
	sqlite3_finalize(stmt);
    syslog(LOG_NOTICE,"sMaster info: ownName %s, ownCountryCode %s, ownRegion %s, sMasterIp %s, sMasterPort %s",
	master.ownName,master.ownCountryCode,master.ownRegion,master.sMasterIp,master.sMasterPort);
	
	sprintf(SQLQUERY,"SELECT servicePort, rdacPort, dmrPort, baseDmrPort, maxRepeaters, echoId FROM master");
	if (sqlite3_prepare_v2(db,SQLQUERY,-1,&stmt,0) == 0){
		if (sqlite3_step(stmt) == SQLITE_ROW){
			servicePort = sqlite3_column_int(stmt,0);
			rdacPort = sqlite3_column_int(stmt,1);
			dmrPort = sqlite3_column_int(stmt,2);
			baseDmrPort = sqlite3_column_int(stmt,3);
			maxRepeaters = sqlite3_column_int(stmt,4) + 1;
			echoId = sqlite3_column_int(stmt,5);
		}
		else{
			syslog(LOG_NOTICE,"failed to read masterInfo, no row");
			sqlite3_finalize(stmt);
			return 0;
		}
	}
	else{
		syslog(LOG_NOTICE,"failed to read masterInfo, query bad");
		sqlite3_finalize(stmt);
		return 0;
	}
	syslog(LOG_NOTICE,"ServicePort %i rdacPort %i dmrPort %i baseDmrPort %i baseRdacPort %i maxRepeaters %i echoId %i rrsGpsId",
	servicePort,rdacPort,dmrPort,baseDmrPort,baseRdacPort,maxRepeaters-1,echoId,rrsGpsId);
	if (maxRepeaters > 98){
		syslog(LOG_NOTICE,"maxRepeaters exceeded 98, quiting application");
		sqlite3_finalize(stmt);
		return 0;
	}
	sqlite3_finalize(stmt);
	return 1;
}

int loadTalkGroups(){
	//Loading the allowed talkgroups from the SQLITE database
	
	unsigned char SQLQUERY[200] = {0};
	char *lineread;
	int i;
	int size = 10;
	sqlite3_stmt *stmt;
	sMasterTS1List = malloc ((sizeof *sMasterTS1List) * size);
	sMasterTS2List = malloc ((sizeof *sMasterTS2List) * size);
	repTS1List = malloc ((sizeof *repTS1List) * size);
	repTS2List = malloc ((sizeof *repTS2List) * size);
	unsigned char sMasterTS1[100];
	unsigned char sMasterTS2[100];
	unsigned char repTS1[100];
	unsigned char repTS2[100];
	
	sprintf(SQLQUERY,"SELECT repTS1,repTS2,sMasterTS1,sMasterTS2 FROM master");
	if (sqlite3_prepare_v2(db,SQLQUERY,-1,&stmt,0) == 0){
		if (sqlite3_step(stmt) == SQLITE_ROW){
			sprintf(tsInfo.repTS1,"%s",sqlite3_column_text(stmt,0));
			sprintf(tsInfo.repTS2,"%s",sqlite3_column_text(stmt,1));
			sprintf(tsInfo.sMasterTS1,"%s",sqlite3_column_text(stmt,2));
			sprintf(tsInfo.sMasterTS2,"%s",sqlite3_column_text(stmt,3));
			sqlite3_finalize(stmt);
			memcpy(sMasterTS1,tsInfo.sMasterTS1,sizeof(sMasterTS1));
			memcpy(sMasterTS2,tsInfo.sMasterTS2,sizeof(sMasterTS2));
			memcpy(repTS1,tsInfo.repTS1,sizeof(repTS1));
			memcpy(repTS2,tsInfo.repTS2,sizeof(repTS2));
			//Assign the talkgroups that we allow on TS1 to and from sMaster
			if (lineread = strtok(sMasterTS1,",")){
				if (strstr(lineread,"**")){ //If ** in talkgroup, this is a range of 100
					sMasterTS1List[0][0] = atoi(lineread) * 100;
					sMasterTS1List[0][1] = (atoi(lineread) * 100) + 99;
					sprintf(master.announcedCC1,"%4s",master.ownCountryCode);
				}
				else{
					sMasterTS1List[0][0] = atoi(lineread);
					sMasterTS1List[0][1] = atoi(lineread);
					sprintf(master.announcedCC1,"%4s",lineread);
				}
				master.sMasterTS1GroupCount++;
				while (lineread = strtok(NULL,",")){
					if (master.sMasterTS1GroupCount > size) sMasterTS1List = realloc(sMasterTS1List, (sizeof *sMasterTS1List) * master.sMasterTS1GroupCount);
					if (strstr(lineread,"**")){ //If ** in talkgroup, this is a range of 100
						sMasterTS1List[master.sMasterTS1GroupCount][0] = atoi(lineread) * 100;
						sMasterTS1List[master.sMasterTS1GroupCount][1] = (atoi(lineread) * 100) + 99;
						sprintf(master.announcedCC1,"%s%4s",master.announcedCC1,master.ownCountryCode);
					}
					else{
						sMasterTS1List[master.sMasterTS1GroupCount][0] = atoi(lineread);
						sMasterTS1List[master.sMasterTS1GroupCount][1] = atoi(lineread);
						sprintf(master.announcedCC1,"%s%4s",master.announcedCC1,lineread);
					}
					master.sMasterTS1GroupCount++;
				}
			}
			if (master.sMasterTS1GroupCount < 10){  //If not 10 talkgroups for TS1, fill with 0 to report to sMaster
				for (i=0;i<(10-master.sMasterTS1GroupCount);i++){
					sprintf(master.announcedCC1,"%s   0",master.announcedCC1);
				}
			}
            //Assign the talkgroups that we allow on TS2 to and from sMaster
			if (lineread = strtok(sMasterTS2,",")){
				if (strstr(lineread,"**")){//If ** in talkgroup, this is a range of 100
					sMasterTS2List[0][0] = atoi(lineread) * 100;
					sMasterTS2List[0][1] = (atoi(lineread) * 100) + 99;
					sprintf(master.announcedCC2,"%4s",master.ownCountryCode);
				}
				else{
					sMasterTS2List[0][0] = atoi(lineread);
					sMasterTS2List[0][1] = atoi(lineread);
					sprintf(master.announcedCC2,"%4s",lineread);
				}
				master.sMasterTS2GroupCount++;
				while (lineread = strtok(NULL,",")){
					if (master.sMasterTS2GroupCount > size) sMasterTS2List = realloc(sMasterTS2List, (sizeof *sMasterTS2List) * master.sMasterTS2GroupCount);
					if (strstr(lineread,"**")){//If ** in talkgroup, this is a range of 100
						sMasterTS2List[master.sMasterTS2GroupCount][0] = atoi(lineread) * 100;
						sMasterTS2List[master.sMasterTS2GroupCount][1] = (atoi(lineread) * 100) + 99;
						sprintf(master.announcedCC2,"%s%4s",master.announcedCC2,master.ownCountryCode);
					}
					else{
						sMasterTS2List[master.sMasterTS2GroupCount][0] = atoi(lineread);
						sMasterTS2List[master.sMasterTS2GroupCount][1] = atoi(lineread);
						sprintf(master.announcedCC2,"%s%4s",master.announcedCC2,lineread);
					}
					master.sMasterTS2GroupCount++;
				}
			}
			if (master.sMasterTS2GroupCount < 10){//If not 10 talkgroups for TS2, fill with 0 to report to sMaster
				for (i=0;i<(10-master.sMasterTS2GroupCount);i++){
					sprintf(master.announcedCC2,"%s   0",master.announcedCC2);
				}
			}
			//Assign the talkgroups allowed between repeaters on TS1
			if (lineread = strtok(repTS1,",")){
				if (strstr(lineread,"**")){
					repTS1List[0][0] = atoi(lineread) * 100;
					repTS1List[0][1] = (atoi(lineread) * 100) + 99;
				}
				else{
					repTS1List[0][0] = atoi(lineread);
					repTS1List[0][1] = atoi(lineread);
				}
				master.repTS1GroupCount++;
				while (lineread = strtok(NULL,",")){
					if (master.repTS1GroupCount > size) repTS1List = realloc(repTS1List, (sizeof *repTS1List) * master.repTS1GroupCount);
					if (strstr(lineread,"**")){//If ** in talkgroup, this is a range of 100
						repTS1List[master.repTS1GroupCount][0] = atoi(lineread) * 100;
						repTS1List[master.repTS1GroupCount][1] = (atoi(lineread) * 100) + 99;
					}
					else{
						repTS1List[master.repTS1GroupCount][0] = atoi(lineread);
						repTS1List[master.repTS1GroupCount][1] = atoi(lineread);
					}
					master.repTS1GroupCount++;
				}
			}

			if (lineread = strtok(repTS2,",")){
				if (strstr(lineread,"**")){//If ** in talkgroup, this is a range of 100
					repTS2List[0][0] = atoi(lineread) * 100;
					repTS2List[0][1] = (atoi(lineread) * 100) + 99;
				}
				else{
					repTS2List[0][0] = atoi(lineread);
					repTS2List[0][1] = atoi(lineread);
				}
				master.repTS2GroupCount++;
				while (lineread = strtok(NULL,",")){
					if (master.repTS2GroupCount > size) repTS2List = realloc(repTS2List, (sizeof *repTS2List) * master.repTS2GroupCount);
					if (strstr(lineread,"**")){
						repTS2List[master.repTS2GroupCount][0] = atoi(lineread) * 100;
						repTS2List[master.repTS2GroupCount][1] = (atoi(lineread) * 100) + 99;
					}
					else{
						repTS2List[master.repTS2GroupCount][0] = atoi(lineread);
						repTS2List[master.repTS2GroupCount][1] = atoi(lineread);
					}
					master.repTS2GroupCount++;
				}
			}
			//Below code just to show the loaded talkgroups in syslog
			syslog(LOG_NOTICE,"sMaster talk groups TS1");
			if (master.sMasterTS1GroupCount>0){
				for (i=0;i<master.sMasterTS1GroupCount;i++){
					syslog(LOG_NOTICE,"(%i) %i - %i",i,sMasterTS1List[i][0],sMasterTS1List[i][1]);
				}
			}
			else  syslog(LOG_NOTICE,"NONE");
			syslog(LOG_NOTICE,"Announced country codes TS1|%s|",master.announcedCC1);
					
			syslog(LOG_NOTICE,"sMaster talk groups TS2");
			if (master.sMasterTS2GroupCount>0){
				for (i=0;i<master.sMasterTS2GroupCount;i++){
					syslog(LOG_NOTICE,"(%i) %i - %i",i,sMasterTS2List[i][0],sMasterTS2List[i][1]);
				}
			}
			else  syslog(LOG_NOTICE,"NONE");
			syslog(LOG_NOTICE,"Announced country codes TS2|%s|",master.announcedCC2);

			syslog(LOG_NOTICE,"repTS1 talk groups");
			if (master.repTS1GroupCount>0){
				for (i=0;i<master.repTS1GroupCount;i++){
					syslog(LOG_NOTICE,"(%i) %i - %i",i,repTS1List[i][0],repTS1List[i][1]);
				}
			}
			else syslog(LOG_NOTICE,"NONE");
				
			syslog(LOG_NOTICE,"repTS2 talk groups");
			if (master.repTS2GroupCount>0){
				for (i=0;i<master.repTS2GroupCount;i++){
					syslog(LOG_NOTICE,"(%i) %i - %i",i,repTS2List[i][0],repTS2List[i][1]);
				}
			}
			else syslog(LOG_NOTICE,"NONE");
			return 1;
		}
	}
	sqlite3_finalize(stmt);
	syslog(LOG_NOTICE,"Failed to load talkgroups. Is master table populated in database ?");
	return 0;
}


int main(int argc, char**argv)
{
	
	// Our process ID and Session ID
	pid_t pid,sid;
	
	/*printf("Becoming a daemon...\n");
	// Fork off the parent process
	pid = fork();
    if (pid < 0) {
		exit(EXIT_FAILURE);
    }
	
	// If we got a good PID, then we can exit the parent process. 
    if (pid > 0) {
		exit(EXIT_SUCCESS);
    }
	
	// Change the file mode mask 
    umask(0);*/
    
	setlogmask (LOG_UPTO (LOG_NOTICE));
	openlog("Master-server", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
     
	// Create a new SID for the child process 
    /*sid = setsid();
    if (sid < 0) {
		// Log the failure 
		exit(EXIT_FAILURE);
    }
	
	// Close out the standard file descriptors 
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);*/
	
	pthread_t thread;
	int port;
	int dbInit;
	int i;
	
	db = openDatabase();
	
	//Init and create sqlite database if needed
	dbInit = initDatabase(db);
	if (dbInit == 0){
		syslog(LOG_NOTICE,"Failed to init database");
		return 0;
	}

	//Start webserver thread
	pthread_create(&thread, NULL, webServerListener,NULL);

    for(;;){
		restart = 0;
		dmrState[1] = IDLE;
		dmrState[2] = IDLE;
		//Get info to get us going
		if(!getMasterInfo()) return 0;
		//Load the allowed talkgroups
		if(!loadTalkGroups()) return 0;
		//Start sMaster Thread
		pthread_create(&thread, NULL, sMasterThread,NULL);
		//Start listening on the service port
		serviceListener(servicePort);
		//If we got here, we are restarting, waiting for all threads to end
		sleep(6);
		master = emptyMaster;
		highestRepeater = 0;
		for (i=0;i<99;i++){
			repeaterList[i] = emptyRepeater;
			rdacList[i] = emptyRepeater;
		}
	}
}
