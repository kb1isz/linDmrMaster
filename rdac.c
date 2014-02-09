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


struct repeater rdacList[100] = {0};


int select_str(char *s)
{
	int i;

	char *accepted_strings[] = {
	"$repeaterId\n",
	"$callsign\n",
	"$hardware\n",
	"$firmware\n",
	"$mode\n",
	"$txfreq\n",
	"$rxfreq\n"
	};
	
	for (i=0; i < sizeof(accepted_strings)/sizeof(*accepted_strings);i++){
		if (!strcmp(s, accepted_strings[i])) return i;
	}
	return -1;
}

void delRdacRepeater(struct sockaddr_in address){
	int i;
	
	for(i=0;i<maxRepeaters;i++){
		if (rdacList[i].address.sin_addr.s_addr == address.sin_addr.s_addr){
			rdacList[i].address.sin_addr.s_addr = 0;
			rdacList[i].rdacOnline = false;
			return;
		} 
	}
	
}

int setRdacRepeater(struct sockaddr_in address){
	int i;
	sqlite3_stmt *stmt;
	char SQLQUERY[300];
	
	for(i=0;i<maxRepeaters;i++){
		if (rdacList[i].address.sin_addr.s_addr == address.sin_addr.s_addr) return i;
	}
	
	
	for(i=0;i<maxRepeaters;i++){
		if (rdacList[i].address.sin_addr.s_addr == 0) break;
	}
	if (i == maxRepeaters){
		syslog(LOG_NOTICE,"Not possible to add repeater, maximum number reached");
		return 99;
	}
	rdacList[i].address = address;
	//See if there is already info in the database based on IP address
	sprintf(SQLQUERY,"SELECT repeaterId,callsign,txFreq,shift,hardware,firmware,mode FROM repeaters WHERE currentAddress = %lu",(long)address.sin_addr.s_addr);
	if (sqlite3_prepare_v2(db,SQLQUERY,-1,&stmt,0) == 0){
		if (sqlite3_step(stmt) == SQLITE_ROW){
			rdacList [i].id = sqlite3_column_int(stmt,0);
			sprintf(rdacList[i].callsign,"%s",sqlite3_column_text(stmt,1));
			sprintf(rdacList[i].txFreq,"%s",sqlite3_column_text(stmt,2));
			sprintf(rdacList[i].shift,"%s",sqlite3_column_text(stmt,3));
			sprintf(rdacList[i].hardware,"%s",sqlite3_column_text(stmt,4));
			sprintf(rdacList[i].firmware,"%s",sqlite3_column_text(stmt,5));
			sprintf(rdacList[i].mode,"%s",sqlite3_column_text(stmt,6));
			syslog(LOG_NOTICE,"Assigning %s %s %s %s %s %s to repeater on pos %i from database",rdacList[i].callsign,rdacList[i].hardware
			,rdacList[i].firmware,rdacList[i].mode,rdacList[i].txFreq,rdacList[i].shift,i);
		}
	}
	sqlite3_finalize(stmt);
	return i;
}


int findRdacRepeater(struct sockaddr_in address){
	int i;

	for(i=0;i<maxRepeaters;i++){
		if (rdacList[i].address.sin_addr.s_addr == address.sin_addr.s_addr) return i;
	}
	
	return 99;
}


bool getRepeaterInfo(int sockfd,int repPos,struct sockaddr_in cliaddrOrg){
	FILE *fp;
	struct sockaddr_in cliaddr;
	char *lineread;
	int cc,i,rc,n;
	unsigned char fl[256] = {0};
	unsigned char line[400];
	unsigned char buffer[256] ={0};
	fd_set master;
	struct timeval timeout;
	char callsignUtf16[20] = {0};
	char callsignUtf8[12] = {0};
	char hardwareUtf16[10] = {0};
	char hardwareUtf8[7] = {0};
	char firmwareUtf16[24] = {0};
	char firmwareUtf8[14] = {0};
	char timeStamp[20];
	double txFreq;
	double rxFreq;
	double shift;
	int cnt;
	socklen_t len;
	size_t srclen,dstlen;
	sqlite3_stmt *stmt;
	char SQLQUERY[300];
		
	FD_ZERO(&master);
	len = sizeof(cliaddr);
	fp=fopen("rdac.in","r");
	while (!feof(fp)){
		if (fgets(line,400,fp) != NULL){
			lineread = strtok(line,":");
			if(!strcmp(lineread,"R")){		//See if line is something we need to receive
				cc = 0;
				while(lineread = strtok(NULL,":")){
					fl[cc] = strtol(lineread,NULL,16);
					cc++;
				}
				cnt = 0;
				do{
					FD_SET(sockfd, &master);
					timeout.tv_sec = 5;
					timeout.tv_usec = 0;
					if (rc = select(sockfd+1, &master, NULL, NULL, &timeout) == -1) { 
						return false;
					}
		
					if (FD_ISSET(sockfd,&master)) {
						n = recvfrom(sockfd,buffer,256,0,(struct sockaddr *)&cliaddr,&len);
						cnt++;
					
						if (cliaddr.sin_addr.s_addr == cliaddrOrg.sin_addr.s_addr){
							if (memcmp(buffer,fl,cc) != 0){ 
								fclose(fp);
								return false;  //did not receive expected info	
							}
							//printf("Received string we are waiting for\n");
						}
					}
					else{
						//timeout
						fclose(fp);
						return false;
					}
				} while(cliaddr.sin_addr.s_addr != cliaddrOrg.sin_addr.s_addr && cnt < 5);
				continue;
			}
			if(!strcmp(lineread,"S")){		//See if line is something we need to send
				cc = 0;
				while(lineread = strtok(NULL,":")){
					fl[cc] = strtol(lineread,NULL,16);
					cc++;
				}
				sendto(sockfd,fl,cc,0,(struct sockaddr *)&cliaddrOrg,sizeof(cliaddrOrg));
				continue;
			}
			if(!strcmp(lineread,"E")){		//See if line is something we need to extract
				while(lineread = strtok(NULL,":")){
					//printf("Extracting %s\n",lineread);
					switch (select_str(lineread)){
						case 0:  //0 = repeaterId
						rdacList[repPos].id = buffer[20] << 16 | buffer[19] << 8 | buffer[18];
						syslog(LOG_NOTICE,"Assigning id %i to repeater on RDAC pos %i",rdacList[repPos].id,repPos);
						break;
						
						case 1: {//1 = callsign
						memcpy(callsignUtf16,buffer + 0x58,20);
						char * pIn = callsignUtf16;
						char * pOut = (char*)callsignUtf8;
						srclen = 20;
						dstlen = 10;
						iconv_t conv = iconv_open("UTF-8","UTF-16LE");
						iconv(conv, &pIn, &srclen, &pOut, &dstlen);
						iconv_close(conv);
						sprintf(rdacList[repPos].callsign,"%s",callsignUtf8);
						break;}
						
						case 2: {//2 = hardware
						memcpy(hardwareUtf16,buffer + 0x78,10);
						char * pIn = hardwareUtf16;
						char * pOut = (char*)hardwareUtf8;
						srclen = 10;
						dstlen = 5;
						iconv_t conv = iconv_open("UTF-8","UTF-16LE");
						iconv(conv, &pIn, &srclen, &pOut, &dstlen);
						iconv_close(conv);
						sprintf(rdacList[repPos].hardware,"%s",hardwareUtf8);
						break;}

						case 3: {//2 = firmware
						memcpy(firmwareUtf16,buffer + 0x38,24);
						char * pIn = firmwareUtf16;
						char * pOut = (char*)firmwareUtf8;
						srclen = 24;
						dstlen = 12;
						iconv_t conv = iconv_open("UTF-8","UTF-16LE");
						iconv(conv, &pIn, &srclen, &pOut, &dstlen);
						iconv_close(conv);
						sprintf(rdacList[repPos].firmware,"%s",firmwareUtf8);
						break;}
						
						case 4: { //3 = mode
						if (buffer[26] == 0) sprintf(rdacList[repPos].mode,"DIG");
						if (buffer[26] == 1) sprintf(rdacList[repPos].mode,"ANA");
						if (buffer[26] == 2) sprintf(rdacList[repPos].mode,"MIX");
						break;}
						
						case 5: {//4 = txfreq
						txFreq = buffer[32] << 24 | buffer[31] << 16 | buffer[30] << 8 | buffer[29];
						txFreq = txFreq/1000000;
						sprintf(rdacList[repPos].txFreq,"%.4f",txFreq);
						break;}
						
						case 6: {//5 = rxfreq
						rxFreq = buffer[36] << 24 | buffer[35] << 16 | buffer[34] << 8 | buffer[33];
						rxFreq = rxFreq/1000000;
						shift = rxFreq - txFreq;
						sprintf(rdacList[repPos].shift,"%1.1f",shift);
						rdacList[repPos].rdacUpdated = true;
						break;}
					}
				}
			}
		}
	}
	syslog(LOG_NOTICE,"Updating from RDAC %s %s %s %s %s %s to repeater on pos %i",rdacList[repPos].callsign,rdacList[repPos].hardware
	,rdacList[repPos].firmware,rdacList[repPos].mode,rdacList[repPos].txFreq,rdacList[repPos].shift,repPos);
	sprintf(SQLQUERY,"SELECT count(*) FROM repeaters WHERE repeaterId = %i",rdacList[repPos].id);
	time_t now = time(NULL);
	struct tm *t = localtime(&now);
	strftime(timeStamp,sizeof(timeStamp),"%Y-%m-%d %H:%M:%S",t);
	if (sqlite3_prepare_v2(db,SQLQUERY,-1,&stmt,0) == 0){
		if (sqlite3_step(stmt) == SQLITE_ROW){
			sprintf(SQLQUERY,"UPDATE repeaters SET callsign = '%s', txFreq = '%s', shift = '%s', hardware = '%s', firmware = '%s', mode = '%s', currentAddress = %lu, timeStamp = '%s' WHERE repeaterId = %i",
			rdacList[repPos].callsign,rdacList[repPos].txFreq,rdacList[repPos].shift,rdacList[repPos].hardware,rdacList[repPos].
			firmware,rdacList[repPos].mode,(long)cliaddrOrg.sin_addr.s_addr,timeStamp,rdacList[repPos].id);
			sqlite3_exec(db,SQLQUERY,0,0,0);

		}
		else{
			syslog(LOG_NOTICE,"RDAC, no row, removing repeater from list");
			delRdacRepeater(cliaddrOrg);
		}
	}
	else{
		syslog(LOG_NOTICE,"RDAC, bad query, removing repeater from list");
		delRdacRepeater(cliaddrOrg);
		close(sockfd);
		pthread_exit(NULL);
	}
	sqlite3_finalize(stmt);
	fclose(fp);
	return true;
}


void *rdacListener(void* f){
	int sockfd,n,i,rc;
	struct sockaddr_in servaddr;
	socklen_t len;
	unsigned char buffer[500];
	unsigned char response[500] ={0};
	//int port = (intptr_t)f;
	struct sockInfo* param = (struct sockInfo*) f;
	int port = param->port;
	struct sockaddr_in cliaddr = param->address;
	int repPos = 0;
	fd_set fdMaster;
	struct timeval timeout;
	time_t timeNow,pingTime;
	
	syslog(LOG_NOTICE,"Listener for port %i started",port);
	repPos = findRdacRepeater(cliaddr);
	rdacList[repPos].rdacOnline = true;
	sockfd=socket(AF_INET,SOCK_DGRAM,0);
	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
	servaddr.sin_port=htons(port);
	bind(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr));
	FD_ZERO(&fdMaster);
	time(&pingTime);
	
	len = sizeof(cliaddr);
	for (;;){
		FD_SET(sockfd, &fdMaster);
		timeout.tv_sec = 5;
		timeout.tv_usec = 0;
		if (rc = select(sockfd+1, &fdMaster, NULL, NULL, &timeout) == -1) { 
			syslog(LOG_NOTICE,"Select error, closing socket port %i",port);
			close(sockfd);
			pthread_exit(NULL);
        }
		if (FD_ISSET(sockfd,&fdMaster)) {
			if (restart){
				syslog(LOG_NOTICE,"Exiting RDAC thread (restart)");
				close(sockfd);
				pthread_exit(NULL);
			}
			n = recvfrom(sockfd,buffer,500,0,(struct sockaddr *)&cliaddr,&len);
			if (n>2){
			}
			else{
				time(&pingTime);
				response[0] = 0x41;
				if (repPos !=99) sendto(sockfd,response,n,0,(struct sockaddr *)&cliaddr,sizeof(cliaddr));
				if (repPos !=99 && !rdacList[repPos].rdacUpdated){
					getRepeaterInfo(sockfd,repPos,cliaddr);
				}
			}
		}
		else{
			if (restart){
				syslog(LOG_NOTICE,"Exiting RDAC thread (restart)");
				close(sockfd);
				pthread_exit(NULL);
			}
			time(&timeNow);
			if (difftime(timeNow,pingTime) > 60) {
				repPos = findRdacRepeater(cliaddr);
				if (repPos !=99){
					syslog(LOG_NOTICE,"PING timeout on RDAC port %i repeater %s, exiting thread",port,rdacList[repPos].callsign);
					syslog(LOG_NOTICE,"Removing repeater from list position %i",repPos);
				}
				else{
					syslog(LOG_NOTICE,"PING timeout on RDAC port %i repeater already removed from list, exiting thread",port);
				}
				delRdacRepeater(cliaddr);
				close(sockfd);
				pthread_exit(NULL);
			}
		}
	}
}
