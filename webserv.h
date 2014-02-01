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

/*  User-defined data types  */

enum Req_Method { GET, HEAD, POST, UNSUPPORTED };
enum Req_Type   { SIMPLE, FULL };

struct ReqInfo {
    enum Req_Method method;
    enum Req_Type   type;
    char           *referer;
    char           *useragent;
    char           *resource;
    int             status;
};


/*  Global macros/variables  */

#define MAX_REQ_LINE         (1024)
#define LISTENQ          (1024)