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
	return db;
}

void closeDatabase(sqlite3 *db){
	if (sqlite3_close(db) != SQLITE_OK) syslog(LOG_NOTICE,"Failed to close SQLITE database");
}

bool isFieldExisting(sqlite3 *db, char *table, char *field){
	sqlite3_stmt *stmt;
	char SQLQUERY[200];

	syslog(LOG_NOTICE,"Database, checking column '%s' in table '%s'",field,table);
	sprintf(SQLQUERY,"PRAGMA table_info(%s)",table);
	if (sqlite3_prepare_v2(db,SQLQUERY,-1,&stmt,0) == 0){
		while (sqlite3_step(stmt) == SQLITE_ROW){
			if (strcmp(sqlite3_column_text(stmt,1),field) == 0){
				sqlite3_finalize(stmt);
				return true;
			}
		}
	}
	else{
		syslog(LOG_NOTICE,"Database error: %s",sqlite3_errmsg(db));
		sqlite3_finalize(stmt);
		return false;
	}
		sqlite3_finalize(stmt);
		syslog(LOG_NOTICE,"field %s doesn't exist",field);
		return false;
}

bool isTableExisting(sqlite3 *db, char *table){
	sqlite3_stmt *stmt;
	char SQLQUERY[200];
	
	syslog(LOG_NOTICE,"Database, checking table '%s'",table);
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
	
	if (!isTableExisting(db,"master")){
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
	if (!isTableExisting(db,"sMaster")){
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

	if (!isTableExisting(db,"repeaters")){
		sprintf(SQLQUERY,"CREATE TABLE repeaters (repeaterId INTEGER default 0 ,callsign VARCHAR(10) default '',txFreq VARCHAR(10) default '',shift VARCHAR(7) default '', hardware VARCHAR(11) default '', firmware VARCHAR(12) default '', mode VARCHAR(4) default '', currentAddress INTEGER default 0, timeStamp varchar(20) default '1970-1-1 00:00:00', ipAddress VARCHAR(50) default '',language VARCHAR(50) default 'english', geoLocation VARCHAR(20) default '', aprsPass VARCHAR(5) default '0000', aprsBeacon VARCHAR(100) default 'DMR repeater', aprsPHG VARCHHAR(7) default '')");
		if (sqlite3_exec(db,SQLQUERY,0,0,0) == 0){
			neededToCreate = true;
			syslog(LOG_NOTICE,"Table repeater created");
		}
		else{
			syslog(LOG_NOTICE,"Database error: %s",sqlite3_errmsg(db));
			return 0;
		}
	}

	if (!isTableExisting(db,"callsigns")){
		sprintf(SQLQUERY,"CREATE TABLE callsigns (radioId INTEGER default 0, callsign VARCHAR(32) default '', name VARCHAR(32) default '', aprsSuffix VARCHAR(3) default '', aprsBeacon VARCHAR(100) default 'DMR terminal', aprsSymbol INTEGER default 62, hasSendAprs INTEGER default 0, messageStore INTEGER default 1, email VARCHAR(100) default '', login VARCHAR(50) default '', password VARCHAR(50) default '', lastAprsTime INTEGER default 0, madeChange INTEGER default 0, city VARCHAR(32) default '', state VARCHAR(32) default '', country VARCHAR(32) default '', radio VARCHAR(32) default '', homeRepeaterId VARCHAR(32) default '', remarks VARCHAR(32) default '')");
		if (sqlite3_exec(db,SQLQUERY,0,0,0) == 0){
			neededToCreate = true;
			syslog(LOG_NOTICE,"Table callsigns created");
		}
		else{
			syslog(LOG_NOTICE,"Database error: %s",sqlite3_errmsg(db));
			return 0;
		}
	}
	
	//check for new fields added later in development
	if (!isFieldExisting(db,"repeaters","language")){
		sprintf(SQLQUERY,"ALTER TABLE repeaters ADD COLUMN language varchar(50) default 'english'");
		if (sqlite3_exec(db,SQLQUERY,0,0,0) == 0){
			syslog(LOG_NOTICE,"field language in repeater created");
		}
		else{
			syslog(LOG_NOTICE,"Database error: %s",sqlite3_errmsg(db));
			return 0;
		}
	}

        if (!isFieldExisting(db,"repeaters","geoLocation")){
                sprintf(SQLQUERY,"ALTER TABLE repeaters ADD COLUMN geoLocation varchar(20) default ''");
                if (sqlite3_exec(db,SQLQUERY,0,0,0) == 0){
                        syslog(LOG_NOTICE,"field geoLocation in repeater created");
                }
                else{
                        syslog(LOG_NOTICE,"Database error: %s",sqlite3_errmsg(db));
                        return 0;
                }
        }

        if (!isFieldExisting(db,"repeaters","aprsPass")){
                sprintf(SQLQUERY,"ALTER TABLE repeaters ADD COLUMN aprsPass varchar(5) default '0000'");
                if (sqlite3_exec(db,SQLQUERY,0,0,0) == 0){
                        syslog(LOG_NOTICE,"field aprsPass in repeater created");
                }
                else{
                        syslog(LOG_NOTICE,"Database error: %s",sqlite3_errmsg(db));
                        return 0;
                }
        }

        if (!isFieldExisting(db,"repeaters","aprsBeacon")){
                sprintf(SQLQUERY,"ALTER TABLE repeaters ADD COLUMN aprsBeacon varchar(100) default 'DMR repeater'");
                if (sqlite3_exec(db,SQLQUERY,0,0,0) == 0){
                        syslog(LOG_NOTICE,"field aprsBeacon in repeater created");
                }
                else{
                        syslog(LOG_NOTICE,"Database error: %s",sqlite3_errmsg(db));
                        return 0;
                }
        }

        if (!isFieldExisting(db,"repeaters","aprsPHG")){
                sprintf(SQLQUERY,"ALTER TABLE repeaters ADD COLUMN aprsPHG varchar(7) default ''");
                if (sqlite3_exec(db,SQLQUERY,0,0,0) == 0){
                        syslog(LOG_NOTICE,"field aprsPHG in repeater created");
                }
                else{
                        syslog(LOG_NOTICE,"Database error: %s",sqlite3_errmsg(db));
                        return 0;
                }
        }

		
	if (neededToCreate) return 2; else return 1;
}
