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

sqlite3 *openDatabase(){
	int rc;
	sqlite3 *db;
	
	rc = sqlite3_open("Master_Server.sql", &db);
	if (rc){
		syslog(LOG_NOTICE,"Cant't open database: %s",sqlite3_errmsg(db));
		sqlite3_close(db);
		return NULL;
	}
	syslog(LOG_NOTICE,"Database opened");
	return db;
}

bool isTableExists(sqlite3 *db, char *table){
	sqlite3_stmt *stmt;
	char SQLQUERY[200];
	
	sprintf(SQLQUERY,"SELECT count(*) FROM sqlite_master WHERE type='table' AND name = '%s'",table);
	if (sqlite3_prepare_v2(db,SQLQUERY,-1,&stmt,0) == 0){
		if (sqlite3_step(stmt) == SQLITE_ROW){
			if(sqlite3_column_int(stmt,0) == 0){
				syslog(LOG_NOTICE,"table %s doesn't exist",table);
				sqlite3_finalize(stmt);
				return false;
			}
		}
	}
	else{
		syslog(LOG_NOTICE,"Database error: %s",sqlite3_errmsg(db));
		sqlite3_finalize(stmt);
		return false;
	}
	sqlite3_finalize(stmt);
	return true;
}

int initDatabase(sqlite3 *db){
	sqlite3_stmt *stmt;
	char SQLQUERY[400];
	bool neededToCreate = false;
	
	if (!isTableExists(db,"master")){
		sprintf(SQLQUERY,"CREATE TABLE master (repTS1 VARCHAR(100) default '',repTS2 VARCHAR(100) default '',sMasterTS1 VARCHAR(100) default '',sMasterTS2 VARCHAR(100) default '', timeBase INTEGER default 60, servicePort int default 50000, rdacPort int default 50002,dmrPort int default 50001, baseDmrPort int default 50100, baseRdacPort int default 50200, maxRepeaters int default 20, echoId int default 9990)");
		if (sqlite3_exec(db,SQLQUERY,0,0,0) == 0){
			neededToCreate = true;
			sprintf(SQLQUERY,"INSERT INTO master (repTS1) VALUES ('')");
			if (sqlite3_exec(db,SQLQUERY,0,0,0) == 0){
				syslog(LOG_NOTICE,"Table master created");
			}
			else{
				syslog(LOG_NOTICE,"Database error: %s",sqlite3_errmsg(db));
			return 0;
			}
		}
		else{
			syslog(LOG_NOTICE,"Database error: %s",sqlite3_errmsg(db));
			return 0;
		}
	}
	if (!isTableExists(db,"sMaster")){
		sprintf(SQLQUERY,"CREATE TABLE sMaster (ownName VARCHAR(100) default '',ownCountryCode VARCHAR(5) default '',ownRegion VARCHAR(2) default '',sMasterIp VARCHAR(100) default '', sMasterPort VARCHAR(5) default '62010')");
		if (sqlite3_exec(db,SQLQUERY,0,0,0) == 0){
			neededToCreate = true;
			sprintf(SQLQUERY,"INSERT INTO sMaster (ownName) VALUES ('')");
			if (sqlite3_exec(db,SQLQUERY,0,0,0) == 0){
				syslog(LOG_NOTICE,"Table sMaster created");
			}
			else{
				syslog(LOG_NOTICE,"Database error: %s",sqlite3_errmsg(db));
			return 0;
			}
		}
		else{
			syslog(LOG_NOTICE,"Database error: %s",sqlite3_errmsg(db));
			return 0;
		}
	}
	if (!isTableExists(db,"repeaters")){
		sprintf(SQLQUERY,"CREATE TABLE repeaters (repeaterId INTEGER default 0 ,callsign VARCHAR(10) default '',txFreq VARCHAR(10) default '',shift VARCHAR(7) default '', hardware VARCHAR(11) default '', firmware VARCHAR(12) default '', mode VARCHAR(4) default '', currentAddress INTEGER default 0, timeStamp varchar(20) default '1970-1-1 00:00:00')");
		if (sqlite3_exec(db,SQLQUERY,0,0,0) == 0){
			neededToCreate = true;
			syslog(LOG_NOTICE,"Table repeater created");
		}
		else{
			syslog(LOG_NOTICE,"Database error: %s",sqlite3_errmsg(db));
			return 0;
		}
	}
	if (neededToCreate) return 2; else return 1;
}