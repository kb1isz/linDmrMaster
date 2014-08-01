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

struct BPTC1969{
	bool responseRequested;
        int dataPacketFormat;
        int sapId;
        int appendBlocks;
};

bool * extractInfo(bool bits[264]){

        static bool info[196];
        int bytePos=0;
        int i;

        //printf("Info bits: ");
        for(i=0;i<98;i++){
                info[bytePos] = bits[i];
                //printf("%i",info[bytePos]);
                bytePos++;
        }

        for(i=166;i<264;i++){
                info[bytePos] = bits[i];
                //printf("%i",info[bytePos]);
                bytePos++;
        }

        //printf("\n");
        return info;
}

bool * deInterleave(bool * bits){
        static bool deInterleavedBits[196];
        int a,interleaveSequence;

        for (a=0;a<196;a++){
                interleaveSequence=(a*181)%196;
                deInterleavedBits[a] = *(bits + interleaveSequence);
        }
        return deInterleavedBits;
}

bool * extractPayload(bool * deInterData){
        int a,pos=0;
        static bool outData[96];
        for (a=4;a<=11;a++)     {
                outData[pos]=*(deInterData+a);
                pos++;
        }
        for (a=16;a<=26;a++)    {
                outData[pos]=*(deInterData+a);
                pos++;
        }
        for (a=31;a<=41;a++)    {
                outData[pos]=*(deInterData+a);
                pos++;
        }
        for (a=46;a<=56;a++)    {
                outData[pos]=*(deInterData+a);
                pos++;
        }
        for (a=61;a<=71;a++)    {
                outData[pos]=*(deInterData+a);
                pos++;
        }
        for (a=76;a<=86;a++)    {
                outData[pos]=*(deInterData+a);
                pos++;
        }
        for (a=91;a<=101;a++)   {
                outData[pos]=*(deInterData+a);
                pos++;
        }
        for (a=106;a<=116;a++)  {
                outData[pos]=*(deInterData+a);
                pos++;
        }
        for (a=121;a<=131;a++)  {
                outData[pos]=*(deInterData+a);
                pos++;
        }
        return outData;
}


struct BPTC1969 decodeBPTC1969(bool bits[264]){

        bool *infoBits; //196 info bits
        bool *deInterleavedBits; //196 bits
        static bool *payloadBits; //96  bits
        int blocksToFollow=0,a;
        unsigned char dpf=0,sap=0,bitPadding=0;
	struct BPTC1969 BPTC1969decode;

        infoBits = extractInfo(bits);
        deInterleavedBits = deInterleave(infoBits);
        payloadBits = extractPayload(deInterleavedBits);

        /*printf("Payload bits\n");
        for(a=0;a<96;a++){
                printf("%i",*(payloadBits+a));
        }
        printf("\n");*/
        if(*(payloadBits+1) == 1){
		BPTC1969decode.responseRequested = true;
		 //syslog(LOG_NOTICE,"response requested"); 
	}
	else{
		BPTC1969decode.responseRequested = false;
		//syslog(LOG_NOTICE,"NO response requested");
	}

        for(a=4;a<8;a++){
                if(*(payloadBits + a) == true) dpf = dpf + (char)(8 / pow(2,a-4));
        }
        //syslog(LOG_NOTICE,"Data Packet Format: ");
	BPTC1969decode.dataPacketFormat = dpf;
        switch (dpf){
                case 0:
                //syslog(LOG_NOTICE,"Unified Data Transport\n");
                break;

                case 1:
                //syslog(LOG_NOTICE,"Response packet\n");
                break;

                case 2:
                //syslog(LOG_NOTICE,"Data packet with unconfirmed delivery\n");
                break;

                case 3:
                //syslog(LOG_NOTICE,"Data packet with confirmed delivery\n");
                break;

                case 13:
                //syslog(LOG_NOTICE,"Short Data: Defined\n");
                break;
                case 14:
                //syslog(LOG_NOTICE,"Short Data: Raw or Status/Precoded\n");
                break;

                case 15:
                //syslog(LOG_NOTICE,"Proprietary Data Packet\n");
                break;

        }

        for(a=8;a<12;a++){
                if(*(payloadBits + a) == true) sap = sap + (char)(8 / pow(2,a-8));
        }
		
	//syslog(LOG_NOTICE,"SAP id: ");
	BPTC1969decode.sapId = sap;
        switch (sap){

                case 0:
                //syslog(LOG_NOTICE,"Unified Data Transport\n");
                break;

                case 2:
                //syslog(LOG_NOTICE,"TCP/IP header compression\n");
                break;

                case 3:
                //syslog(LOG_NOTICE,"UDP/IP header compression\n");
                break;

                case 4:
                //syslog(LOG_NOTICE,"IP based Packet data\n");
                break;

                case 5:
                //syslog(LOG_NOTICE,"Address Resolution Protocol(ARP)\n");
                break;

                case 9:
                //syslog(LOG_NOTICE,"Proprietary Packet data\n");
                break;

                case 10:
                //syslog(LOG_NOTICE,"Short Data\n");
                break;

        }
        if (dpf == 13){
                for(a=12;a<16;a++){//only AB in 2nd octet
                         if(*(payloadBits + a) == true) blocksToFollow = blocksToFollow + (char)(8 / pow(2,a-12));
                }
                BPTC1969decode.appendBlocks = blocksToFollow;
		syslog(LOG_NOTICE,"Appended blocks : %i\n",blocksToFollow);

                for(a=72;a<80;a++){
                         if(*(payloadBits + a) == true) bitPadding = bitPadding + (char)(128 / pow(2,a-12));
                }
        }

        return BPTC1969decode;
}
