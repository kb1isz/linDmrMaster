#include "master_server.h"


MYSQL *OpenMySqlDatabase (int port)
{
	MYSQL *conn;
	conn = mysql_init(NULL);
   // Connect to database
   if (!mysql_real_connect(conn, databaseServer,databaseUser, databasePassword, database, 0, NULL, 0)) {
	  syslog(LOG_NOTICE,"%s[%i]", mysql_error(conn),port);
	  return conn;
   }
   return conn;
}

void closeMysqlDatabase(MYSQL *conn,MYSQL_RES *res){
	mysql_free_result(res);
	mysql_close(conn);
}

MYSQL_RES *sqlQuery (char query[200],MYSQL *conn){
	
	int sqlRes;
	MYSQL_RES *result;
	if (mysql_real_query(conn, query, strlen(query))){
        syslog(LOG_NOTICE,"%s",query);
		syslog(LOG_NOTICE,"%s", mysql_error(conn));
		sqlRes = mysql_errno(conn);
		if (sqlRes == 2006 || sqlRes == 2013){
			syslog(LOG_NOTICE,"Re-opening mysql connection");
			closeMysqlDatabase(conn,result);
			*conn = *OpenMySqlDatabase(0);
			if (!conn){
				syslog(LOG_NOTICE,"Failed to re-open mysql connection");
				return result;
			}
			if (mysql_real_query(conn, query, strlen(query))){
				syslog(LOG_NOTICE,"%s",query);
				syslog(LOG_NOTICE,"%s", mysql_error(conn));
				return result;	
			}
		}
		return result;
	}
	result = mysql_store_result(conn);
	return result;
}
