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
#include "webserv.h"

char page[50];
void handleParameter();

int handlePost(int conn, struct ReqInfo  *reqinfo){
	char buffer[MAX_REQ_LINE] = {0};
    fd_set fds;
    struct timeval tv;
	int rval;
	
	tv.tv_sec  = 1;
    tv.tv_usec = 0;
	
	for (;;){
		FD_ZERO(&fds);
		FD_SET (conn, &fds);
	
		rval = select(conn + 1, &fds, NULL, NULL, &tv);
		if ( rval < 0 ) {
			syslog(LOG_NOTICE,"Error calling select() in get_request()");
		}
		else if ( rval == 0 ) {
			break;
		}
		else {
			Readline(conn, buffer, MAX_REQ_LINE - 1);
			Trim(buffer);
			handleParameter(buffer);
		}
	}
	restart = 1;
}

void handleParameter(char buffer[MAX_REQ_LINE]){
	char *param;
	char *value;
	char *listValue;
	char SQLQUERY[300];
	if(param = strtok(buffer,"=")){
		if (strstr(param,"page")){
			value = strtok(NULL,"=");
			sprintf(page,"%s",value);
		}
		else if(strstr(param,"addedList")){
			value = strtok(NULL,"=");
			if (value != NULL){
				listValue = strtok(value,";");
				if (listValue != NULL){
					sprintf(SQLQUERY,"INSERT INTO %s (repeaterId) VALUES (%s)",page,listValue);
					syslog(LOG_NOTICE,"Updating database '%s'",SQLQUERY);
					if (sqlite3_exec(db,SQLQUERY,0,0,0) != 0){
						syslog(LOG_NOTICE,"Database error: %s",sqlite3_errmsg(db));
					}
				}
				while (listValue = strtok(NULL,";")){
					sprintf(SQLQUERY,"INSERT INTO %s (repeaterId) VALUES (%s)",page,listValue);
					syslog(LOG_NOTICE,"Updating database '%s'",SQLQUERY);
					if (sqlite3_exec(db,SQLQUERY,0,0,0) != 0){
						syslog(LOG_NOTICE,"Database error: %s",sqlite3_errmsg(db));
					}
				}
			}
		}
		else if(strstr(param,"removedList")){
			value = strtok(NULL,"=");
			if (value != NULL){
				listValue = strtok(value,";");
				if (listValue != NULL){
					sprintf(SQLQUERY,"DELETE FROM %s WHERE repeaterId = %s",page,listValue);
					syslog(LOG_NOTICE,"Updating database '%s'",SQLQUERY);
					if (sqlite3_exec(db,SQLQUERY,0,0,0) != 0){
						syslog(LOG_NOTICE,"Database error: %s",sqlite3_errmsg(db));
					}
				}
				while (listValue = strtok(NULL,";")){
					sprintf(SQLQUERY,"DELETE FROM %s WHERE repeaterId = %s",page,listValue);
					syslog(LOG_NOTICE,"Updating database '%s'",SQLQUERY);
					if (sqlite3_exec(db,SQLQUERY,0,0,0) != 0){
						syslog(LOG_NOTICE,"Database error: %s",sqlite3_errmsg(db));
					}
				}
			}
		}
		else{
			value = strtok(NULL,"=");
			if (value != NULL){
				sprintf(SQLQUERY,"UPDATE %s SET %s = '%s'",page,param,value);
				syslog(LOG_NOTICE,"Updating database '%s'",SQLQUERY);
				if (sqlite3_exec(db,SQLQUERY,0,0,0) != 0){
					syslog(LOG_NOTICE,"Database error: %s",sqlite3_errmsg(db));
				}
			}
		}
	}
}