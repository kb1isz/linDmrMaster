#include "master_server.h"

char *htmlReplace(char line[500], char *resource){
	
	char *pos;
	char tmpLine[500];
	int cpLen;

if (strstr(resource,"master.html")){
	if(pos = strstr(line,"$repTS1")){
		cpLen = strlen(line) - strlen(pos);
		strncpy(tmpLine,line,cpLen);
		tmpLine[cpLen] = 0;
		strcat(tmpLine,tsInfo.repTS1);
		strcat(tmpLine,pos+7);
		sprintf(line,"%s",tmpLine);
	}
	if(pos = strstr(line,"$repTS2")){
		cpLen = strlen(line) - strlen(pos);
		strncpy(tmpLine,line,cpLen);
		tmpLine[cpLen] = 0;
		strcat(tmpLine,tsInfo.repTS2);
		strcat(tmpLine,pos+7);
		sprintf(line,"%s",tmpLine);
	}
	if(pos = strstr(line,"$sMasterTS1")){
		cpLen = strlen(line) - strlen(pos);
		strncpy(tmpLine,line,cpLen);
		tmpLine[cpLen] = 0;
		strcat(tmpLine,tsInfo.sMasterTS1);
		strcat(tmpLine,pos+11);
		sprintf(line,"%s",tmpLine);
	}
	if(pos = strstr(line,"$sMasterTS2")){
		cpLen = strlen(line) - strlen(pos);
		strncpy(tmpLine,line,cpLen);
		tmpLine[cpLen] = 0;
		strcat(tmpLine,tsInfo.sMasterTS2);
		strcat(tmpLine,pos+11);
		sprintf(line,"%s",tmpLine);
	}
	if(pos = strstr(line,"$servicePort")){
		char port[7];
		cpLen = strlen(line) - strlen(pos);
		strncpy(tmpLine,line,cpLen);
		tmpLine[cpLen] = 0;
		sprintf(port,"%i",servicePort);
		strcat(tmpLine,port);
		strcat(tmpLine,pos+12);
		sprintf(line,"%s",tmpLine);
	}
	if(pos = strstr(line,"$dmrPort")){
		char port[7];
		cpLen = strlen(line) - strlen(pos);
		strncpy(tmpLine,line,cpLen);
		tmpLine[cpLen] = 0;
		sprintf(port,"%i",dmrPort);
		strcat(tmpLine,port);
		strcat(tmpLine,pos+8);
		sprintf(line,"%s",tmpLine);
	}
	if(pos = strstr(line,"$rdacPort")){
		char port[7];
		cpLen = strlen(line) - strlen(pos);
		strncpy(tmpLine,line,cpLen);
		tmpLine[cpLen] = 0;
		sprintf(port,"%i",rdacPort);
		strcat(tmpLine,port);
		strcat(tmpLine,pos+9);
		sprintf(line,"%s",tmpLine);
	}
	if(pos = strstr(line,"$baseDmrPort")){
		char port[7];
		cpLen = strlen(line) - strlen(pos);
		strncpy(tmpLine,line,cpLen);
		tmpLine[cpLen] = 0;
		sprintf(port,"%i",baseDmrPort);
		strcat(tmpLine,port);
		strcat(tmpLine,pos+12);
		sprintf(line,"%s",tmpLine);
	}
	if(pos = strstr(line,"$baseRdacPort")){
		char port[7];
		cpLen = strlen(line) - strlen(pos);
		strncpy(tmpLine,line,cpLen);
		tmpLine[cpLen] = 0;
		sprintf(port,"%i",baseRdacPort);
		strcat(tmpLine,port);
		strcat(tmpLine,pos+13);
		sprintf(line,"%s",tmpLine);
	}
	if(pos = strstr(line,"$maxRepeaters")){
		char max[7];
		cpLen = strlen(line) - strlen(pos);
		strncpy(tmpLine,line,cpLen);
		tmpLine[cpLen] = 0;
		sprintf(max,"%i",maxRepeaters-1);
		strcat(tmpLine,max);
		strcat(tmpLine,pos+13);
		sprintf(line,"%s",tmpLine);
		htmlReplace(line,resource);
	}
	if(pos = strstr(line,"$echoId")){
		char id[20];
		cpLen = strlen(line) - strlen(pos);
		strncpy(tmpLine,line,cpLen);
		tmpLine[cpLen] = 0;
		sprintf(id,"%i",echoId);
		strcat(tmpLine,id);
		strcat(tmpLine,pos+7);
		sprintf(line,"%s",tmpLine);
		htmlReplace(line,resource);
	}
}
if (strstr(resource,"sMaster.html")){
	if(pos = strstr(line,"$sMasterIp")){
		cpLen = strlen(line) - strlen(pos);
		strncpy(tmpLine,line,cpLen);
		tmpLine[cpLen] = 0;
		strcat(tmpLine,master.sMasterIp);
		strcat(tmpLine,pos+10);
		sprintf(line,"%s",tmpLine);
	}
	if(pos = strstr(line,"$sMasterPort")){
		cpLen = strlen(line) - strlen(pos);
		strncpy(tmpLine,line,cpLen);
		tmpLine[cpLen] = 0;
		strcat(tmpLine,master.sMasterPort);
		strcat(tmpLine,pos+12);
		sprintf(line,"%s",tmpLine);
	}
	if(pos = strstr(line,"$ownCountryCode")){
		cpLen = strlen(line) - strlen(pos);
		strncpy(tmpLine,line,cpLen);
		tmpLine[cpLen] = 0;
		strcat(tmpLine,master.ownCountryCode);
		strcat(tmpLine,pos+15);
		sprintf(line,"%s",tmpLine);
	}
	if(pos = strstr(line,"$ownRegion")){
		cpLen = strlen(line) - strlen(pos);
		strncpy(tmpLine,line,cpLen);
		tmpLine[cpLen] = 0;
		strcat(tmpLine,master.ownRegion);
		strcat(tmpLine,pos+10);
		sprintf(line,"%s",tmpLine);
	}
	if(pos = strstr(line,"$ownName")){
		cpLen = strlen(line) - strlen(pos);
		strncpy(tmpLine,line,cpLen);
		tmpLine[cpLen] = 0;
		strcat(tmpLine,master.ownName);
		strcat(tmpLine,pos+8);
		sprintf(line,"%s",tmpLine);
	}

}
	return line;
}