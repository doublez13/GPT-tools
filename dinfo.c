/*
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#define HEADER_SIZE 512
#define LBA_SIZE 512

struct GPTHeader{
  char               signature[8];
  char               revision[4];
  unsigned int       headerSize;   //92 in most cases. 512-92 is just 0s
  unsigned int       crc32;        //different for primary and backup
  unsigned int       reserved;
  unsigned long long LBA1, LBA2;   //swapped for backup
  unsigned long long LBApart, LBApartLast; //different for primary and backup
  char               GUID[16];
  unsigned long long LBAstart;     //LBA of partition entries. Different on Pr and Ba
  unsigned int       numParts;
  unsigned int       singleSize;
  unsigned int       crc32Part;
};


/*
Return 1 if the device is GPT
Return 0 otherwise
This function checks both the default and backup GPT headers.
*/
int isGPT(FILE *deviceFile){
  int offset = LBA_SIZE;
  char gptSig[9] = "EFI PART";
  char diskSig[9];
  diskSig[8] = '\0';
  int GPT1 = 0;
  int GPT2 = 0;
 
  //Header 1 
  //MAKE USE OF THE FUNCTIONS BELOW
  fseek( deviceFile, offset , SEEK_SET );
  fread(diskSig, 1, 8, deviceFile);
  if(strcmp(gptSig, diskSig) == 0){GPT1 = 1;}

  //Header 2
  //MAKE USE THE FUNCTIONS BELOW
  fseek(deviceFile, 0, SEEK_END);
  offset = ftell(deviceFile);
  offset -= HEADER_SIZE;
  fseek( deviceFile, offset , SEEK_SET );
  fread(diskSig, 1, 8, deviceFile);
  if(strcmp(gptSig, diskSig) == 0){GPT2 = 1;}
  
  return GPT1 | GPT2;
}


void getPrimaryHeader(char* header, FILE *deviceFile){
  fseek( deviceFile, LBA_SIZE, SEEK_SET );  
  fread(header, 1, HEADER_SIZE, deviceFile);
}


void getSecondaryHeader(char* header, FILE *deviceFile){
  int offset;

  fseek(deviceFile, 0, SEEK_END);
  offset = ftell(deviceFile);
  offset -= HEADER_SIZE;  

  fseek( deviceFile, offset, SEEK_SET );
  fread(header, 1, HEADER_SIZE, deviceFile);
}


/*
Returns 1 if the calculated CRC32 matches the on disk CRC32
Returns 0 otherwise
*/
int verifyGPT(char *header){
  unsigned long diskCRC32;
  //FIX THE HARD CODED INTS
  unsigned char* copyHeader = (unsigned char *)calloc(1, 92);
 
  memcpy( copyHeader, header,  92 );
 
  //zero out CRC32
  //maybe clean this up by doing an unsigned int 0 
  copyHeader[16] = 0;
  copyHeader[17] = 0;
  copyHeader[18] = 0;
  copyHeader[19] = 0;

  unsigned long calCRC32 = crc32(0, copyHeader, 92);

  memcpy( &diskCRC32, header + 16,  8 );
 
  free(copyHeader);
  return calCRC32 == diskCRC32;
}




//Populates the GPTHeader struct
void readGPT(struct GPTHeader *header, char* GPTHeader){
  memcpy( &(*header).signature,   GPTHeader +  0,  8 );
  memcpy( &(*header).revision,    GPTHeader +  8,  4 );
  memcpy( &(*header).headerSize,  GPTHeader + 12,  4 );
  memcpy( &(*header).crc32,       GPTHeader + 16,  4 );
  memcpy( &(*header).reserved,    GPTHeader + 20,  4 );
  memcpy( &(*header).LBA1,        GPTHeader + 24,  8 );
  memcpy( &(*header).LBA2,        GPTHeader + 32,  8 );
  memcpy( &(*header).LBApart,     GPTHeader + 40,  8 );
  memcpy( &(*header).LBApartLast, GPTHeader + 48,  8 );
  memcpy( &(*header).GUID,        GPTHeader + 56, 16 );
  memcpy( &(*header).LBAstart,    GPTHeader + 72,  8 );
  memcpy( &(*header).numParts,    GPTHeader + 80,  4 );
  memcpy( &(*header).singleSize,  GPTHeader + 84,  4 );
  memcpy( &(*header).crc32Part,   GPTHeader + 88,  4 );
}


/*
Write header to disk
This does not alter the partition table
Returns 0 on clean write, -1 on failure
*/
int writeCharGPT(char *srcHeader, FILE *deviceFile){
  //THIS CURRENTLY WRITES THE SAME GPT TO PRIMARY AND BACKUP
  //THIS IS INCORRECT AND NEEDS TO BE CHANGED
  long offset = LBA_SIZE;
  int ret1, ret2;

  fseek( deviceFile, offset , SEEK_SET );
  ret1 = fwrite( srcHeader, 1, HEADER_SIZE, deviceFile );

  fseek(deviceFile, 0, SEEK_END);
  offset = ftell(deviceFile);
  offset -= HEADER_SIZE;
  fseek( deviceFile, offset , SEEK_SET );
  ret2 = fwrite( srcHeader, 1, HEADER_SIZE, deviceFile );

  if((ret1 & ret2) == HEADER_SIZE){return 0;}
  return -1;
}

/*
Writes the GPT found in the struct GPTHeader to disk
both primary and backup
Returns 0 on clean write, -1 on failure
*/
int writeGPT(struct GPTHeader *header, FILE *deviceFile){
  char* charHeader = (char *)calloc(1, HEADER_SIZE);
  int result; 
 
  memcpy( charHeader +  0, &(*header).signature,    8 );
  memcpy( charHeader +  8, &(*header).revision,     4 );
  memcpy( charHeader + 12, &(*header).headerSize,   4 );
  memcpy( charHeader + 16, &(*header).crc32,        4 );
  memcpy( charHeader + 20, &(*header).reserved,     4 );
  memcpy( charHeader + 24, &(*header).LBA1,         8 );
  memcpy( charHeader + 32, &(*header).LBA2,         8 );
  memcpy( charHeader + 40, &(*header).LBApart,      8 );
  memcpy( charHeader + 48, &(*header).LBApartLast,  8 );
  memcpy( charHeader + 56, &(*header).GUID,        16 );
  memcpy( charHeader + 72, &(*header).LBAstart,     8 );
  memcpy( charHeader + 80, &(*header).numParts,     4 );
  memcpy( charHeader + 84, &(*header).singleSize,   4 );
  memcpy( charHeader + 88, &(*header).crc32Part,    4 );
  
  result = writeCharGPT(charHeader, deviceFile);
  free(charHeader);
  return result;
}


/*
Builds a fresh GPTHeader pair
No Return value
*/
void buildGPT(struct GPTHeader *GPTHeader1, struct GPTHeader *GPTHeader2){
  struct GPTHeader *primary = calloc(1,sizeof(struct GPTHeader));
  //struct GPTHeader *secondary = calloc(1,sizeof(struct GPTHeader));

  //primary->signature = sig;
  //char               revision[4];
  primary->headerSize = 92;   //92 in most cases. 512-92 is just 0s
  primary->crc32      = 0;        //different for primary and backup
  primary->reserved   = 0;
  //primary->LBA1 =; 
  //primary->LBA2 =;   //swapped for backup
  //unsigned long long LBApart, LBApartLast; //different for primary and backup
  //char               GUID[16];
  primary->LBAstart   = 2;     //LBA of partition entries. Different on Pr and Ba
  primary->numParts   = 128;
  primary->singleSize = 128;
  //primary->crc32Part  = 0;//FIX THIS!
  
} 


int main(){
  FILE *deviceFile;
  char path[10] = "/dev/sdc";
  
  if(access(path, F_OK)){
    printf("No Such Device\n");
    return 0;
  }
  deviceFile=fopen(path, "w+");

  if(!isGPT(deviceFile)){
    printf("Not a GPT device\n");
    return 0;
  }

  char* GPTHeader1 = (char *)calloc(1, HEADER_SIZE);
  char* GPTHeader2 = (char *)calloc(1, HEADER_SIZE);
  getPrimaryHeader(GPTHeader1, deviceFile);
  getSecondaryHeader(GPTHeader2, deviceFile); 

  if( !verifyGPT(GPTHeader1) ){printf("Primary header corrupted\n");}
  if( !verifyGPT(GPTHeader2) ){printf("Secondary header corrupted\n");}
 
 
  struct GPTHeader *header = calloc(1,sizeof(struct GPTHeader));
  readGPT(header, GPTHeader1);
  //writeGPT(header, deviceFile);


  fclose(deviceFile);
  return 0;
}
