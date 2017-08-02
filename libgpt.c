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
#include "include/libgpt.h"

#define HEADER_SIZE 512
#define LBA_SIZE 512


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
  fseek( deviceFile, offset , SEEK_SET );
  fread(diskSig, 1, 8, deviceFile);
  if(strcmp(gptSig, diskSig) == 0){GPT1 = 1;}

  //Header 2
  fseek(deviceFile, 0, SEEK_END);
  offset = ftell(deviceFile);
  offset -= HEADER_SIZE;
  fseek( deviceFile, offset , SEEK_SET );
  fread(diskSig, 1, 8, deviceFile);
  if(strcmp(gptSig, diskSig) == 0){GPT2 = 1;}
  
  return GPT1 | GPT2;
}



unsigned long long getPrimaryHeaderOffset(){
  return LBA_SIZE;  
}


unsigned long long getSecondaryHeaderOffset( FILE *deviceFile ){
  fseek(deviceFile, 0, SEEK_END);
  return ftell(deviceFile) - HEADER_SIZE;
}




/*
Returns 1 if the calculated CRC32 matches the on disk CRC32
Returns 0 otherwise
*/
int verifyGPT(struct GPTHeader *header){
  unsigned long diskCRC32;
  unsigned long calcCRC32;
  char *charHeader;

  struct GPTHeader *copyHeader = (struct GPTHeader*)calloc(1, sizeof(struct GPTHeader));
  memcpy( copyHeader, header,  sizeof(struct GPTHeader) );
  copyHeader->crc32 = 0;
  
  charHeader = (char *)calloc(1, 92);
  GPTHeaderToChar(charHeader, copyHeader); 

  diskCRC32 = header->crc32;
  calcCRC32 = crc32(0, (unsigned char*)charHeader, 92); 
 
  free(copyHeader);
  free(charHeader);
  return calcCRC32 == diskCRC32;
}




/*
 *Populates header with the GPT header found in deviceFile at offset
 *
 */
void readGPT(struct GPTHeader *header, FILE *deviceFile, unsigned long long offset){
  char *charHeader = (char *)calloc(1, HEADER_SIZE);
  readCharGPT(charHeader, deviceFile, offset);
  charToGPTHeader(header, charHeader);
}

void readCharGPT(char *dstHeader, FILE *deviceFile, unsigned long long offset){
  fseek( deviceFile, offset , SEEK_SET );
  fread( dstHeader, 1, HEADER_SIZE, deviceFile );
}

void charToGPTHeader(struct GPTHeader *dst, char *src){
  memcpy( &(*dst).signature,   src +  0,  8 );
  memcpy( &(*dst).revision,    src +  8,  4 );
  memcpy( &(*dst).headerSize,  src + 12,  4 );
  memcpy( &(*dst).crc32,       src + 16,  4 );
  memcpy( &(*dst).reserved,    src + 20,  4 );
  memcpy( &(*dst).LBA1,        src + 24,  8 );
  memcpy( &(*dst).LBA2,        src + 32,  8 );
  memcpy( &(*dst).LBApart,     src + 40,  8 );
  memcpy( &(*dst).LBApartLast, src + 48,  8 );
  memcpy( &(*dst).GUID,        src + 56, 16 );
  memcpy( &(*dst).LBAstart,    src + 72,  8 );
  memcpy( &(*dst).numParts,    src + 80,  4 );
  memcpy( &(*dst).singleSize,  src + 84,  4 );
  memcpy( &(*dst).crc32Part,   src + 88,  4 );
}





/*
Writes the GPT found in the struct GPTHeader to disk at byte offset
Returns 0 on clean write, -1 on failure
*/
int writeGPT(struct GPTHeader *header, FILE *deviceFile, unsigned long long offset){
  char* charHeader = (char *)calloc(1, HEADER_SIZE);
  int result;
  GPTHeaderToChar(charHeader, header);
  
  result = writeCharGPT(charHeader, deviceFile, offset);
  free(charHeader);
  return result;
}

/*
 * Write header to disk at byte offset
 * This does not alter the partition table
 * Returns 0 on clean write, -1 on failure
 * */
int writeCharGPT(char *srcHeader, FILE *deviceFile, unsigned long long offset){
  int ret;
  fseek( deviceFile, offset , SEEK_SET );
  ret = fwrite( srcHeader, 1, HEADER_SIZE, deviceFile );

  if( ret == HEADER_SIZE){return 0;}
  return -1;
}

void GPTHeaderToChar(char *dst, struct GPTHeader *src){
  memcpy( dst +  0, &(*src).signature,    8 );
  memcpy( dst +  8, &(*src).revision,     4 );
  memcpy( dst + 12, &(*src).headerSize,   4 );
  memcpy( dst + 16, &(*src).crc32,        4 );
  memcpy( dst + 20, &(*src).reserved,     4 );
  memcpy( dst + 24, &(*src).LBA1,         8 );
  memcpy( dst + 32, &(*src).LBA2,         8 );
  memcpy( dst + 40, &(*src).LBApart,      8 );
  memcpy( dst + 48, &(*src).LBApartLast,  8 );
  memcpy( dst + 56, &(*src).GUID,        16 );
  memcpy( dst + 72, &(*src).LBAstart,     8 );
  memcpy( dst + 80, &(*src).numParts,     4 );
  memcpy( dst + 84, &(*src).singleSize,   4 );
  memcpy( dst + 88, &(*src).crc32Part,    4 );
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
