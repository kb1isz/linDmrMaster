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

/*Source code inspired on the work of Paul Griffiths
  www.paulgriffiths.net
*/

#include "master_server.h"
#include "webserv.h"

void *Service_Request();
int  Parse_HTTP_Header(char * buffer, struct ReqInfo * reqinfo);
int  Get_Request      (int conn, struct ReqInfo * reqinfo);
void InitReqInfo      (struct ReqInfo * reqinfo);
void FreeReqInfo      (struct ReqInfo * reqinfo);
int Output_HTTP_Headers(int conn, struct ReqInfo * reqinfo);
int Return_Resource (int conn, FILE *resource, struct ReqInfo * reqinfo);
FILE *Check_Resource  (struct ReqInfo * reqinfo);
int Return_Error_Msg(int conn, struct ReqInfo * reqinfo);
int     Trim      (char * buffer);
int     StrUpper  (char * buffer);
void    CleanURL  (char * buffer);
ssize_t Readline  (int sockd, void *vptr, size_t maxlen);
ssize_t Writeline (int sockd, const void *vptr, size_t n);

#define SERVER_PORT            (8080)

char *htmlReplace();

void *webServerListener(){
    int    listener, conn;
    pid_t  pid;
    struct sockaddr_in servaddr;
	int yes=1;        // for setsockopt() SO_REUSEADDR, below
	pthread_t thread;
    
    /*  Create socket  */

    if ( (listener = socket(AF_INET, SOCK_STREAM, 0)) < 0 ){
		syslog(LOG_NOTICE,"Couldn't create listening socket.");
		pthread_exit(NULL);
    }

    /*  Populate socket address structure  */

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("0.0.0.0");
    servaddr.sin_port        = htons(SERVER_PORT);

	setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    /*  Assign socket address to socket  */ 

    if ( bind(listener, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0 ){
		syslog(LOG_NOTICE,"Couldn't bind listening socket.");
		pthread_exit(NULL);
	}

    /*  Make socket a listening socket  */

    if ( listen(listener, LISTENQ) < 0 ){
		syslog(LOG_NOTICE,"Call to listen failed.");
		pthread_exit(NULL);
	}
	syslog(LOG_NOTICE,"Web server started");
    /*  Loop infinitely to accept and service connections  */

    while ( 1 ) {

	/*  Wait for connection  */

		if ( (conn = accept(listener, NULL, NULL)) < 0 ){
			syslog(LOG_NOTICE,"Error calling accept()");
			continue;
		}
		pthread_create(&thread, NULL, Service_Request,(void *)conn);
    }

}

void *Service_Request(void *f) {

	int conn = (intptr_t)f;
    FILE *resource;
	struct ReqInfo  reqinfo;

    InitReqInfo(&reqinfo);
    
    /*  Get HTTP request  */
    if ( Get_Request(conn, &reqinfo) < 0 ) printf("error");
	//return -1;


	if (reqinfo.method == POST){
		handlePost(conn,&reqinfo);
	}
	if (strlen(reqinfo.resource) == 1) sprintf(reqinfo.resource,"/index.html");

/*  Check whether resource exists, whether we have permission
	to access it, and update status code accordingly.          */
	if ( reqinfo.status == 200 ){
		if ( (resource = Check_Resource(&reqinfo)) < 0 ) {
			if ( errno == EACCES )
			reqinfo.status = 401;
			else
			reqinfo.status = 404;
		}
	}
    /*  Output HTTP response headers if we have a full request  */

    if ( reqinfo.type == FULL )
	Output_HTTP_Headers(conn, &reqinfo);
    /*  Service the HTTP request  */
    if ( reqinfo.status == 200 ) {
		if ( Return_Resource(conn, resource, &reqinfo) ) syslog(LOG_NOTICE,"Something wrong returning resource.");
    }
    else Return_Error_Msg(conn, &reqinfo);

    if ( resource > 0 )
	if ( fclose(resource) < 0 ) syslog(LOG_NOTICE,"Error closing resource.");
    FreeReqInfo(&reqinfo);
	close(conn);
}

int Parse_HTTP_Header(char * buffer, struct ReqInfo * reqinfo) {

    //static int first_header = 1;
    char      *temp;
    char      *endptr;
    int        len;
	int first = 0;

    //if ( first_header == 1 ) {

	/*  If first_header is 0, this is the first line of
	    the HTTP request, so this should be the request line.  */


	/*  Get the request method, which is case-sensitive.       */
	if ( !strncmp(buffer, "GET ", 4) ) {
	    reqinfo->method = GET;
	    buffer += 4;
		first = 1;
	}
	else if ( !strncmp(buffer, "HEAD ", 5) ) {
	    reqinfo->method = HEAD;
	    buffer += 5;
		first = 1;
	}
	else if ( !strncmp(buffer, "POST ", 5) ) {
		reqinfo->method = POST;
	    buffer += 5;
		first = 1;
	}
	/*else {
	    reqinfo->method = UNSUPPORTED;
	    reqinfo->status = 501;
	    return -1;
	}*/

	if (first == 1){
	/*  Skip to start of resource  */

	while ( *buffer && isspace(*buffer) )
	    buffer++;


	/*  Calculate string length of resource...  */

	endptr = strchr(buffer, ' ');
	if ( endptr == NULL )
	    len = strlen(buffer);
	else
	    len = endptr - buffer;
	if ( len == 0 ) {
	    reqinfo->status = 400;
	    return -1;
	}

	/*  ...and store it in the request information structure.  */

	reqinfo->resource = calloc(len + 1, sizeof(char));
	strncpy(reqinfo->resource, buffer, len);
	/*  Test to see if we have any HTTP version information.
	    If there isn't, this is a simple HTTP request, and we
	    should not try to read any more headers. For simplicity,
	    we don't bother checking the validity of the HTTP version
	    information supplied - we just assume that if it is
	    supplied, then it's a full request.                        */
	if ( strstr(buffer, "HTTP/") )
	    reqinfo->type = FULL;
	else
	    reqinfo->type = SIMPLE;

	//first_header = 0;
	return 0;
    }


    /*  If we get here, we have further headers aside from the
	request line to parse, so this is a "full" HTTP request.  */

    /*  HTTP field names are case-insensitive, so make an
	upper-case copy of the field name to aid comparison.
	We need to make a copy of the header up until the colon.
	If there is no colon, we return a status code of 400
	(bad request) and terminate the connection. Note that
	HTTP/1.0 allows (but discourages) headers to span multiple
	lines if the following lines start with a space or a
	tab. For simplicity, we do not allow this here.              */

    endptr = strchr(buffer, ':');
    if ( endptr == NULL ) {
	reqinfo->status = 400;
	return -1;
    }

    temp = calloc( (endptr - buffer) + 1, sizeof(char) );
    strncpy(temp, buffer, (endptr - buffer));
    StrUpper(temp);


    /*  Increment buffer so that it now points to the value.
	If there is no value, just return.                    */

    buffer = endptr + 1;
    while ( *buffer && isspace(*buffer) )
	++buffer;
    if ( *buffer == '\0' )
     	return 0;


    /*  Now update the request information structure with the
	appropriate field value. This version only supports the
	"Referer:" and "User-Agent:" headers, ignoring all others.  */

    if ( !strcmp(temp, "USER-AGENT") ) {
	    reqinfo->useragent = malloc( strlen(buffer) + 1 );
	    strcpy(reqinfo->useragent, buffer);
    }
    else if ( !strcmp(temp, "REFERER") ) {
	    reqinfo->referer = malloc( strlen(buffer) + 1 );
	    strcpy(reqinfo->referer, buffer);
    }

    free(temp);
    return 0;
}


/*  Gets request headers. A CRLF terminates a HTTP header line,
    but if one is never sent we would wait forever. Therefore,
    we use select() to set a maximum length of time we will
    wait for the next complete header. If we timeout before
    this is received, we terminate the connection.               */

int Get_Request(int conn, struct ReqInfo * reqinfo) {

    char   buffer[MAX_REQ_LINE] = {0};
    int    rval;
    fd_set fds;
    struct timeval tv;


    /*  Set timeout to 5 seconds  */

    tv.tv_sec  = 5;
    tv.tv_usec = 0;


    /*  Loop through request headers. If we have a simple request,
	then we will loop only once. Otherwise, we will loop until
	we receive a blank line which signifies the end of the headers,
	or until select() times out, whichever is sooner.                */

    do {

	/*  Reset file descriptor set  */

	FD_ZERO(&fds);
	FD_SET (conn, &fds);


	/*  Wait until the timeout to see if input is ready  */

	rval = select(conn + 1, &fds, NULL, NULL, &tv);


	/*  Take appropriate action based on return from select()  */

	if ( rval < 0 ) {
	    syslog(LOG_NOTICE,"Error calling select() in get_request()");
	}
	else if ( rval == 0 ) {

	    /*  input not ready after timeout  */

	    return -1;

	}
	else {

	    /*  We have an input line waiting, so retrieve it  */

	    Readline(conn, buffer, MAX_REQ_LINE - 1);
	    Trim(buffer);

	    if ( buffer[0] == '\0'){
			break;
		}
	    if ( Parse_HTTP_Header(buffer, reqinfo) )
		break;
	}
    } while ( reqinfo->type != SIMPLE );

    return 0;
}


/*  Initialises a request information structure  */

void InitReqInfo(struct ReqInfo * reqinfo) {
    reqinfo->useragent = NULL;
    reqinfo->referer   = NULL;
    reqinfo->resource  = NULL;
    reqinfo->method    = UNSUPPORTED;
    reqinfo->status    = 200;          
}


/*  Frees memory allocated for a request information structure  */

void FreeReqInfo(struct ReqInfo * reqinfo) {
    if ( reqinfo->useragent )
	free(reqinfo->useragent);
    if ( reqinfo->referer )
	free(reqinfo->referer);
    if ( reqinfo->resource )
	free(reqinfo->resource);
}

int Output_HTTP_Headers(int conn, struct ReqInfo * reqinfo) {

    char buffer[100];

    sprintf(buffer, "HTTP/1.0 %d OK\r\n", reqinfo->status);
    Writeline(conn, buffer, strlen(buffer));

    Writeline(conn, "Server: PGWebServ v0.1\r\n", 24);
    Writeline(conn, "Content-Type: text/html\r\n", 25);
    Writeline(conn, "\r\n", 2);

    return 0;
}


/*  Returns a resource  */

int Return_Resource(int conn, FILE *resource, struct ReqInfo * reqinfo) {

    char c;
    int  i;
	char line[2000];
	char sendLine[2000];
	if (strstr(reqinfo->resource,".html")){
		while(fgets(line,sizeof(line),resource)){
			sprintf(sendLine,"%s",htmlReplace(line,reqinfo->resource));
			if ( write(conn, sendLine, strlen(sendLine)) < 1 ){
				syslog(LOG_NOTICE,"Error sending file.");
				break;
			}
		}
	}
	else{
		while (!feof(resource)){
			c = fgetc(resource);
			if ( write(conn, &c, 1) < 1 ){
				syslog(LOG_NOTICE,"Error sending file.");
				break;
			}
		}
	}

    return 0;
}


/*  Tries to open a resource. The calling function can use
    the return value to check for success, and then examine
    errno to determine the cause of failure if neceesary.    */

FILE *Check_Resource(struct ReqInfo * reqinfo) {

    /*  Resource name can contain urlencoded
	data, so clean it up just in case.    */
	char server_root[1000] = "./html";
    CleanURL(reqinfo->resource);

    
    /*  Concatenate resource name to server root, and try to open  */

    strcat(server_root, reqinfo->resource);
	return fopen(server_root, "r");
}


/*  Returns an error message  */

int Return_Error_Msg(int conn, struct ReqInfo * reqinfo) {
    
    char buffer[100];

    sprintf(buffer, "<HTML>\n<HEAD>\n<TITLE>Server Error %d</TITLE>\n"
	            "</HEAD>\n\n", reqinfo->status);
    Writeline(conn, buffer, strlen(buffer));

    sprintf(buffer, "<BODY>\n<H1>Server Error %d</H1>\n", reqinfo->status);
    Writeline(conn, buffer, strlen(buffer));

    sprintf(buffer, "<P>The request could not be completed.</P>\n"
	            "</BODY>\n</HTML>\n");
    Writeline(conn, buffer, strlen(buffer));

    return 0;
}

 
/*  Read a line from a socket  */

ssize_t Readline(int sockd, void *vptr, size_t maxlen) {
    ssize_t n, rc;
    char    c, *buffer;

    buffer = vptr;

    for ( n = 1; n < maxlen; n++ ) {
	
	if ( (rc = read(sockd, &c, 1)) == 1 ) {
	    *buffer++ = c;
	    if ( c == '\n' )
		break;
	}
	else if ( rc == 0 ) {
	    if ( n == 1 )
		return 0;
	    else
		break;
	}
	else {
	    if ( errno == EINTR )
		continue;
	    syslog(LOG_NOTICE,"Error in Readline()");
	}
    }

    *buffer = 0;
    return n;
}


/*  Write a line to a socket  */

ssize_t Writeline(int sockd, const void *vptr, size_t n) {
    size_t      nleft;
    ssize_t     nwritten;
    const char *buffer;

    buffer = vptr;
    nleft  = n;

    while ( nleft > 0 ) {
	if ( (nwritten = write(sockd, buffer, nleft)) <= 0 ) {
	    if ( errno == EINTR )
		nwritten = 0;
	    else
		syslog(LOG_NOTICE,"Error in Writeline()");
	}
	nleft  -= nwritten;
	buffer += nwritten;
    }

    return n;
}


/*  Removes trailing whitespace from a string  */

int Trim(char * buffer) {
    int n = strlen(buffer) - 1;

    while ( !isalnum(buffer[n]) && n >= 0 )
	buffer[n--] = '\0';

    return 0;
}


/*  Converts a string to upper-case  */
    
int StrUpper(char * buffer) {
    while ( *buffer ) {
	*buffer = toupper(*buffer);
	++buffer;
    }
    return 0;
}


/*  Cleans up url-encoded string  */
	
void CleanURL(char * buffer) {
    char asciinum[3] = {0};
    int i = 0, c;
    
    while ( buffer[i] ) {
	if ( buffer[i] == '+' )
	    buffer[i] = ' ';
	else if ( buffer[i] == '%' ) {
	    asciinum[0] = buffer[i+1];
	    asciinum[1] = buffer[i+2];
	    buffer[i] = strtol(asciinum, NULL, 16);
	    c = i+1;
	    do {
		buffer[c] = buffer[c+2];
	    } while ( buffer[2+(c++)] );
	}
	++i;
    }
}
