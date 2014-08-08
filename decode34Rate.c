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

bool * extractInfo();
int INTERLEAVE[] ={
                        0,1,8,9,16,17,24,25,32,33,40,41,48,49,56,57,64,65,72,73,80,81,88,89,96,97,
                        2,3,10,11,18,19,26,27,34,35,42,43,50,51,58,59,66,67,74,75,82,83,90,91,
                        4,5,12,13,20,21,28,29,36,37,44,45,52,53,60,61,68,69,76,77,84,85,92,93,
                        6,7,14,15,22,23,30,31,38,39,46,47,54,55,62,63,70,71,78,79,86,87,94,95};

 unsigned char STATETABLE[]={
                        0,8,4,12,2,10,6,14,
                        4,12,2,10,6,14,0,8,
                        1,9,5,13,3,11,7,15,
                        5,13,3,11,7,15,1,9,
                        3,11,7,15,1,9,5,13,
                        7,15,1,9,5,13,3,11,
                        2,10,6,14,0,8,4,12,
                        6,14,0,8,4,12,2,10};



char * extractDibits(bool * rawBits){
	int a,index=0,deinterleave;
	char  trellisDibit=0;
	static char encDibit[98];
	for (a=0;a<196;a=a+2)	{
		// Set the dibits
		// 01 = +3
		// 00 = +1
		// 10 = -1
		// 11 = -3
		if ((*(rawBits+a)==false)&&(*(rawBits+a+1)==true)) trellisDibit=+3;
		else if ((*(rawBits+a)==false)&&(*(rawBits+a+1)==false)) trellisDibit=+1;
		else if ((*(rawBits+a)==true)&&(*(rawBits+a+1)==false)) trellisDibit=-1;
		else if ((*(rawBits+a)==true)&&(*(rawBits+a+1)==true)) trellisDibit=-3;
		// Deinterleave
		deinterleave=INTERLEAVE[index];
		encDibit[deinterleave]=trellisDibit;
		// Increase the index
		index++;
	}
	/*printf("Extracted dibits: ");
	for (a=0;a<98;a++){
		printf("%i:",encDibit[a]);
	}
	printf("\n");*/
	return encDibit;
}

char *  constellationOut (char *  encDibit){
	static char  constellationPoints[49];
	int a,index=0;
	//printf("constellation: ");
	for (a=0;a<98;a=a+2)	{
		if ((*(encDibit+a)==+1)&&(*(encDibit+a+1)==-1)) constellationPoints[index]=0;
		else if ((*(encDibit+a)==-1)&&(*(encDibit+a+1)==-1)) constellationPoints[index]=1;
		else if ((*(encDibit+a)==+3)&&(*(encDibit+a+1)==-3)) constellationPoints[index]=2;
		else if ((*(encDibit+a)==-3)&&(*(encDibit+a+1)==-3)) constellationPoints[index]=3;
		else if ((*(encDibit+a)==-3)&&(*(encDibit+a+1)==-1)) constellationPoints[index]=4;
		else if ((*(encDibit+a)==+3)&&(*(encDibit+a+1)==-1)) constellationPoints[index]=5;
		else if ((*(encDibit+a)==-1)&&(*(encDibit+a+1)==-3)) constellationPoints[index]=6;
		else if ((*(encDibit+a)==+1)&&(*(encDibit+a+1)==-3)) constellationPoints[index]=7;
		else if ((*(encDibit+a)==-3)&&(*(encDibit+a+1)==+3)) constellationPoints[index]=8;
		else if ((*(encDibit+a)==+3)&&(*(encDibit+a+1)==+3)) constellationPoints[index]=9;
		else if ((*(encDibit+a)==-1)&&(*(encDibit+a+1)==+1)) constellationPoints[index]=10;
		else if ((*(encDibit+a)==+1)&&(*(encDibit+a+1)==+1)) constellationPoints[index]=11;
		else if ((*(encDibit+a)==+1)&&(*(encDibit+a+1)==+3)) constellationPoints[index]=12;
		else if ((*(encDibit+a)==-1)&&(*(encDibit+a+1)==+3)) constellationPoints[index]=13;
		else if ((*(encDibit+a)==+3)&&(*(encDibit+a+1)==+1)) constellationPoints[index]=14;
		else if ((*(encDibit+a)==-3)&&(*(encDibit+a+1)==+1)) constellationPoints[index]=15;
		//printf("%i:",constellationPoints[index]);
		index++;
	}
	//printf("\n");
	return constellationPoints;
}
 
int * tribitExtract (char *  cons){
	int a,b,rowStart,lastState=0;
	static int tribit[49];
	bool match;
	//printf("tribits: ");
	for (a=0;a<49;a++)	{
		// The lastState variable decides which row of STATETABLE we should use
		rowStart=lastState*8;
		match=false;
		for (b=rowStart;b<(rowStart+8);b++)	{
			// Check if this constellation point matches an element of this row of STATETABLE
			if (*(cons+a)==STATETABLE[b])	{
				// Yes it does
				match=true;
				lastState=b-rowStart;
				tribit[a]=lastState;
			}
		}
		//printf("%i:",tribit[a]);
		// If no match found then we have a problem
		if (match==false){
			//printf("no match %i !!!\n",a);
			return NULL;
		}
	}
	//printf("\n");
	return tribit;
}

bool * binaryConvert (int * tribit){
	int a,b=0;
	static bool out[144];

	for (a=0;a<144;a=a+3)	{
		// Convert three bits at a time
		if ((*(tribit+b)&4)>0) out[a]=true;
		else  out[a]=false;
		if ((*(tribit+b)&2)>0) out[a+1]=true;
		else  out[a+1]=false;
		if ((*(tribit+b)&1)>0) out[a+2]=true;
		else  out[a+2]=false;
		// Increment the bit counter
		b++;
	}
	return out;
}

unsigned char *  decodeThreeQuarterRate(bool bits[264]){

	bool *infoBits;  //196 info bits
	char *dibits; //98 info dibits
	char *constellationPoints; //49 constallation points
	int *tribits; //49 tribits
	bool *decodedBinary;  //144 bits
	static unsigned char bb[18];
	int x=0,a,i;

	infoBits = extractInfo(bits);
	dibits = extractDibits(infoBits);
	constellationPoints = constellationOut(dibits);
	tribits = tribitExtract(constellationPoints);
	decodedBinary = binaryConvert(tribits);

	for (a=0;a<144;a=a+8){
		bb[x] = 0;
		for (i=0;i<8;i++){
			if(decodedBinary[a+i] == true) bb[x] = bb[x] + (char)(128 / pow(2,i));
		}
		printf("(%02X)%c",bb[x],bb[x]);
		x++;
	}
	printf("\n");
	return bb;
}

