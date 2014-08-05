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
sqlite3 *openDatabase();
void closeDatabase();

char aprsUrl[100];
char aprsPort[7];


int openAprsSock(){

	int rv;
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	char ipstr[INET_ADDRSTRLEN];

    syslog(LOG_NOTICE,"Opening APRS socket");
	if ((rv = getaddrinfo(aprsUrl, aprsPort, &hints, &servinfo)) != 0) {
		syslog(LOG_NOTICE,"getaddrinfo: %s\n", gai_strerror(rv));
		return;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		struct in_addr *addr;
		struct sockaddr_in *ipv = (struct sockaddr_in *) p->ai_addr;
		addr = &(ipv->sin_addr);
		inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr); 
		syslog(LOG_NOTICE,"address: %s",ipstr);
		if ((sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) {
			syslog(LOG_NOTICE,"Not able to create APRS sock");
			continue;
		}
		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			syslog(LOG_NOTICE,"Error to connect to APRS");
			continue;
		}
		break;
	}
    if (p == NULL)  syslog(LOG_NOTICE,"Failed to open APRS socket");
	
	freeaddrinfo(servinfo); // all done with this structure	
}

void sendAprsBeacon(char callsign[10],char pass[6],char loc[20],char phg[7],char text[100]){
	char toSend[300];
	char ipstr[INET_ADDRSTRLEN];
	int sockfd;

	sprintf(toSend,"user %s pass %s vers DMRgate 1.0\n%s>APRS,%s,qAR,%s:!%s&%s%s",callsign,pass,callsign,callsign,callsign,loc,phg,text);
	openAprsSock();
	send(sockfd,toSend,strlen(toSend),0);
	close(sockfd);

}

void sendAprs(struct gpsCoordinates gpsData, int radioId, struct repeater repeater){
	char toSend[300];
	char timeString[7];
	char SQLQUERY[200];
	int sockfd;
	struct idInfo radioIdent = {0};
	sqlite3 *dbase;
	sqlite3_stmt *stmt;
	unsigned char aprsCor[30];

	dbase = openDatabase();
	sprintf(SQLQUERY,"SELECT callsign,aprsSuffix,aprsBeacon,aprsSymbol,lastAprsTime FROM callsigns WHERE radioId = %i",radioId);
	if (sqlite3_prepare_v2(dbase,SQLQUERY,-1,&stmt,0) == 0){
		if (sqlite3_step(stmt) == SQLITE_ROW){
			sprintf(radioIdent.callsign,"%s",sqlite3_column_text(stmt,0));
			sprintf(radioIdent.aprsSuffix,"%s",sqlite3_column_text(stmt,1));
			sprintf(radioIdent.aprsBeacon,"%s",sqlite3_column_text(stmt,2));
			radioIdent.aprsSymbol = sqlite3_column_int(stmt,3);
			radioIdent.aprsTimeStamp = sqlite3_column_int(stmt,4);
			sqlite3_finalize(stmt);

		}
		else{
			sqlite3_finalize(stmt);
			syslog(LOG_NOTICE,"[%s]DMR ID %i not found in database, not sending to aprs.fi",repeater.callsign,radioId);
			closeDatabase(dbase);
			return;
		}
	}
	else{
		syslog(LOG_NOTICE,"[%s]Bad query %s",repeater.callsign,SQLQUERY);
		closeDatabase(dbase);
		return;
	}
	
	if (time(NULL) - radioIdent.aprsTimeStamp < 5){
		syslog(LOG_NOTICE,"[%s]Preventing aprs.fi flooding for %s",repeater.callsign,radioIdent.callsign);
		closeDatabase(dbase);
		return;
	}

	sprintf(SQLQUERY,"UPDATE callsigns SET hasSendAprs = 1, lastAprsTime = %lu where radioId = %i",time(NULL),radioId);
	sqlite3_exec(dbase,SQLQUERY,0,0,0);
	closeDatabase(dbase);
	sprintf(aprsCor,"%s/%s>%s/%s",gpsData.latitude,gpsData.longitude,gpsData.heading,gpsData.speed);
	aprsCor[18] = radioIdent.aprsSymbol;
	sprintf(toSend,"user %s pass %s vers DMRgate 1.0\n%s%s>APRS,%s,qAR,%s:!%s %s\n",repeater.callsign,repeater.aprsPass,radioIdent.callsign,radioIdent.aprsSuffix,repeater.callsign,repeater.callsign,aprsCor,radioIdent.aprsBeacon);

	//sockfd = openAprsSock();
	//send(sockfd,toSend,strlen(toSend),0);
	//close(sockfd);
	syslog(LOG_NOTICE,"[%s]Would send info to APRS network for %s [%s]",repeater.callsign,radioIdent.callsign,toSend);
}


int checkCoordinates(struct gpsCoordinates gpsData, struct repeater repeater){

        regex_t regex;
        int reti;



        reti = regcomp(&regex, "^[0-9][0-9][0-9][0-9][.][0-9][0-9][NZ]$", 0);
        if(reti){
                syslog(LOG_NOTICE,"[%s]Hyt GPS decode,could not compile regex latitude",repeater.callsign);
                return 0;
        }
        reti = regexec(&regex,gpsData.latitude,0,NULL,0);
        if(reti == REG_NOMATCH){
                syslog(LOG_NOTICE,"[%s]Corrupt latitude received",repeater.callsign);
                regfree(&regex);
                return 0;
        }
        regfree(&regex);

        reti = regcomp(&regex, "^[0-9][0-9][0-9][0-9][0-9][.][0-9][0-9][EW]$", 0);
        if(reti){
                syslog(LOG_NOTICE,"[%s]Hyt GPS decode,could not compile regex longitude",repeater.callsign);
                regfree(&regex);
                return 0;
        }
        reti = regexec(&regex,gpsData.longitude,0,NULL,0);
        if(reti == REG_NOMATCH){
                syslog(LOG_NOTICE,"[%s]Corrupt longitude received",repeater.callsign);
                regfree(&regex);
                return 0;
        }

        reti = regcomp(&regex, "^[0-9][0-9][0-9]$", 0);
        if(reti){
                syslog(LOG_NOTICE,"[%s]Hyt GPS decode,could not compile regex heading",repeater.callsign);
                regfree(&regex);
                return 0;
        }
        reti = regexec(&regex,gpsData.heading,0,NULL,0);
        if(reti == REG_NOMATCH){
                syslog(LOG_NOTICE,"[%s]Corrupt heading received",repeater.callsign);
                regfree(&regex);
                return 0;
        }

        reti = regcomp(&regex, "^[0-9.][0-9.][0-9.]$", 0);
        if(reti){
                syslog(LOG_NOTICE,"[%s]Hyt GPS decode,could not compile regex speed",repeater.callsign);
                regfree(&regex);
                return 0;
        }
        reti = regexec(&regex,gpsData.speed,0,NULL,0);
        if(reti == REG_NOMATCH){
                syslog(LOG_NOTICE,"[%s]Corrupt speed received",repeater.callsign);
                regfree(&regex);
                return 0;
        }

        regfree(&regex);
	return 1;
} 
