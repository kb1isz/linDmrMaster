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
			Error_Quit("Error calling select() in get_request()");
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
}

void handleParameter(char buffer[MAX_REQ_LINE]){
	char *param;
	char *value;
	char SQLQUERY[300];
	if(param = strtok(buffer,"=")){
		if (strstr(param,"page")){
			value = strtok(NULL,"=");
			sprintf(page,"%s",value);
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