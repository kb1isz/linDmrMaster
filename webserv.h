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