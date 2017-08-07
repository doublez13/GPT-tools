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
#include <uuid/uuid.h>
#include "include/libgpt.h"
#include "include/guids.h"

#define HEADER_SIZE 512
#define LBA_SIZE 512

//THIS CODE IS NOT ENDIAN SAFE
//LITTLE ENDIAN ONLY AT THIS POINT
//FIX INBOUND


/*
Return 1 if the device is GPT
Return 0 otherwise
This function checks both the default and backup GPT headers.
*/
int isGPT(FILE *deviceFile){
  uint64_t offset = LBA_SIZE;
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
  offset = getSecondaryHeaderOffset( deviceFile );
  //printf(offset)
  fseek( deviceFile, offset , SEEK_SET );
  fread(diskSig, 1, 8, deviceFile);
  if(strcmp(gptSig, diskSig) == 0){GPT2 = 1;}
  
  return GPT1 | GPT2;
}



uint64_t getPrimaryHeaderOffset(){
  return LBA_SIZE;  
}

uint64_t getSecondaryHeaderOffset( FILE *deviceFile ){
  fseek(deviceFile, 0, SEEK_END);
  return ftell(deviceFile) - HEADER_SIZE;
}




/*
Returns 1 if the calculated CRC32 matches the on disk CRC32
Returns 0 otherwise
*/
int verifyGPT(struct GPTHeader *header){
  return header->crc32 == crc32GPT(header);
}

/*
 *Returns the crc32 value for the GPTHeader header
 */
uint32_t crc32GPT(struct GPTHeader *header){
  char *charHeader;
  struct GPTHeader *copyHeader;
  uint32_t crc;

  copyHeader = (struct GPTHeader*)calloc(1, sizeof(struct GPTHeader));
  memcpy( copyHeader, header,  sizeof(struct GPTHeader) );
  copyHeader->crc32 = 0;
  charHeader = (char *)calloc(1, 92);
  GPTHeaderToChar(charHeader, copyHeader);
  crc = crc32(0, (unsigned char*)charHeader, 92);

  free(charHeader);
  free(copyHeader);
  return crc;
}



/*
 *Populates header with the GPT header found in deviceFile at offset
 *
 */
void readGPT(struct GPTHeader *header, FILE *deviceFile, uint64_t offset){
  char *charHeader = (char *)calloc(1, HEADER_SIZE);
  readCharGPT(charHeader, deviceFile, offset);
  charToGPTHeader(header, charHeader);
  free(charHeader);
}

void readCharGPT(char *dstHeader, FILE *deviceFile, uint64_t offset){
  fseek( deviceFile, offset , SEEK_SET );
  fread( dstHeader, 1, HEADER_SIZE, deviceFile );
}

void charToGPTHeader(struct GPTHeader *dst, char *src){
  memcpy( &dst->signature,   src +  0,  8 );
  memcpy( &dst->revision,    src +  8,  4 );
  memcpy( &dst->headerSize,  src + 12,  4 );
  memcpy( &dst->crc32,       src + 16,  4 );
  memcpy( &dst->reserved,    src + 20,  4 );
  memcpy( &dst->LBA1,        src + 24,  8 );
  memcpy( &dst->LBA2,        src + 32,  8 );
  memcpy( &dst->LBAfirstUse,     src + 40,  8 );
  memcpy( &dst->LBAlastUse, src + 48,  8 );
  memcpy( &dst->GUID,        src + 56, 16 );
  memcpy( &dst->LBApartStart,    src + 72,  8 );
  memcpy( &dst->numParts,    src + 80,  4 );
  memcpy( &dst->singleSize,  src + 84,  4 );
  memcpy( &dst->crc32Part,   src + 88,  4 );
}

void genHeaderFromBackup(struct GPTHeader *new ,struct partTable *newTable, 
struct GPTHeader *working, struct partTable *workingTable){
  uint64_t LBApartStart = 2;
  if(working->LBA1 == 1){
    LBApartStart = working->LBAlastUse + 1; //Is this always safe? alignment issues?
  }

  memcpy( &new->signature,   &working->signature,  8 );
  memcpy( &new->revision,    &working->revision,  4 );
  new->headerSize   = working->headerSize;
  new->crc32        = 0;
  new->reserved     = working->reserved;
  new->LBA1         = working->LBA2;
  new->LBA2         = working->LBA1;
  new->LBAfirstUse  = working->LBAfirstUse;
  new->LBAlastUse   = working->LBAlastUse;
  memcpy( &new->GUID,   &working->GUID,  16 );
  new->LBApartStart = LBApartStart;
  new->numParts     = working->numParts;
  new->singleSize   = working->singleSize;
  new->crc32Part    = working->crc32Part;

  new->crc32 = crc32GPT(new);

  //Copy the valid part table
  //CURRENTLY DONT HAVE CODE TO WRITE THIS BACK TO DISK
  newTable->numParts = workingTable->numParts;
  newTable->singleSize = workingTable->singleSize;
  memcpy( newTable->entries,  workingTable->entries,  sizeof(struct partEntry)*newTable->numParts );
}



/*
Writes the GPT found in the struct GPTHeader to disk at byte offset
Returns 0 on clean write, -1 on failure
*/
int writeGPT(struct GPTHeader *header, FILE *deviceFile, uint64_t offset){
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
int writeCharGPT(char *srcHeader, FILE *deviceFile, uint64_t offset){
  int ret;
  fseek( deviceFile, offset , SEEK_SET );
  ret = fwrite( srcHeader, 1, HEADER_SIZE, deviceFile );

  if( ret == HEADER_SIZE){return 0;}
  return -1;
}

void GPTHeaderToChar(char *dst, struct GPTHeader *src){
  memcpy( dst +  0, &src->signature,    8 );
  memcpy( dst +  8, &src->revision,     4 );
  memcpy( dst + 12, &src->headerSize,   4 );
  memcpy( dst + 16, &src->crc32,        4 );
  memcpy( dst + 20, &src->reserved,     4 );
  memcpy( dst + 24, &src->LBA1,         8 );
  memcpy( dst + 32, &src->LBA2,         8 );
  memcpy( dst + 40, &src->LBAfirstUse,  8 );
  memcpy( dst + 48, &src->LBAlastUse,   8 );
  memcpy( dst + 56, &src->GUID,        16 );
  memcpy( dst + 72, &src->LBApartStart, 8 );
  memcpy( dst + 80, &src->numParts,     4 );
  memcpy( dst + 84, &src->singleSize,   4 );
  memcpy( dst + 88, &src->crc32Part,    4 );
}







/*
Builds a fresh GPTHeader pair and partition table
No Return value
maxLBA exclusive
*/
struct partTable* buildGPT(struct GPTHeader *primary, struct GPTHeader *backup, uint64_t maxLBA){
  uint32_t numParts   = 128;
  uint32_t singleSize = 128;
  struct partTable *pt = (struct partTable*)calloc(1, sizeof(struct partTable) );
  pt->entries         = (struct partEntry*)calloc(1, numParts*sizeof(struct partEntry) ); 

  strcpy(primary->signature, "EFI PART");
  strcpy(backup->signature, "EFI PART");
  
  char revision[4] = {0x00, 0x00, 0x01, 0x00};
  memcpy(primary->revision, revision, 4 );
  memcpy(primary->revision, revision, 4 );  

  primary->headerSize  =  92;   //92 in most cases. 512-92 is just 0s
  backup->headerSize   =   primary->headerSize;

  primary->crc32       =   0;    //zero this out initially
  backup->crc32        =   primary->crc32;

  primary->reserved    =   0;
  backup->reserved     =   primary->reserved;  

  primary->LBA1        =   1;
  backup ->LBA1        =   maxLBA - 1;
  primary->LBA2        =   backup->LBA1;   //swapped for backup
  backup ->LBA2        =   primary->LBA1;
   
  primary->LBAfirstUse =   1 + 1 + numParts * singleSize / LBA_SIZE;
  backup->LBAfirstUse  =   primary->LBAfirstUse;
  primary->LBAlastUse  =   maxLBA - (1 + numParts * singleSize / LBA_SIZE); 
  backup->LBAlastUse   =   primary->LBAlastUse;

  //primary->GUID        = ;
  //backup->GUID         = ;

  primary->LBApartStart =   2;     //LBA of partition entries. Different on Pr and Ba
  backup->LBApartStart  = primary->LBAlastUse + 1;

  primary->numParts    = numParts;
  backup->numParts     = numParts;

  primary->singleSize  = singleSize;
  backup->singleSize   = singleSize;


  primary->crc32Part =   crc32PartTable(pt);
  backup->crc32Part  =   primary->crc32Part;

  primary->crc32 = crc32GPT(primary);
  backup->crc32  = crc32GPT(backup);
 
  return pt; 
} 









//PARTITON FUNCTIONS

struct partTable* readPartTable(struct GPTHeader *header, FILE *deviceFile){
  uint32_t numParts;   //Number of partition entries. Found in GPT
  uint32_t singleSize; //Size of a single partition entry. Found in GPT
  struct   partTable *table;
  char*    charTable;
  uint64_t offset;
  uint64_t tableSize;
  int      part;

  numParts   = header->numParts;
  singleSize = header->singleSize;
  table = (struct partTable*)calloc(1, sizeof(struct partTable) );
  table->entries = (struct partEntry*)calloc(1, numParts*sizeof(struct partEntry) );
  table->numParts   = numParts; 
  table->singleSize = singleSize;

  tableSize =  numParts*singleSize;
  charTable = (char*)malloc( tableSize ); 
  offset    =  header->LBApartStart*LBA_SIZE;
  readCharPartTable(charTable, tableSize, deviceFile, offset); 

  for(part = 0; part < numParts; part++)
    charToPartEntry( &(table->entries[part]), charTable, part*singleSize, singleSize);

  return table;
}


void readCharPartTable(char *dstTable, uint64_t tableSize, FILE *deviceFile, uint64_t offset){
  fseek( deviceFile, offset , SEEK_SET );
  fread( dstTable, 1, tableSize, deviceFile );
}


void charToPartEntry(struct partEntry *entry, char* charTable, uint64_t start, uint32_t length){
  //we're not making use of length right now
  memcpy( &entry->typeGUID,  charTable + start +  0,  16 );
  memcpy( &entry->partGUID,  charTable + start + 16,  16 );
  memcpy( &entry->firstLBA,  charTable + start + 32,   8 );
  memcpy( &entry->lastLBA,   charTable + start + 40,   8 );
  memcpy( &entry->flags,     charTable + start + 48,   8 );
  memcpy( &entry->name,      charTable + start + 56,  72 );
}





void writePartTable(struct GPTHeader *header,  uint64_t headerOffset, struct partTable *table, FILE *deviceFile){
  char *charTable;
  uint32_t crc32Part = crc32PartTable(table);
  uint64_t offset    = header->LBApartStart * LBA_SIZE;
  
  charTable = (char *)calloc(1, 128*128);
  partTableToChar(charTable, table);
  
  writeCharPartTable(charTable, table->numParts * table->singleSize, deviceFile, offset);
  header->crc32Part = crc32Part;

  header->crc32 = crc32GPT(header);
  writeGPT(header, deviceFile, headerOffset);
}

void writeCharPartTable(char *srcTable, uint64_t tableSize, FILE *deviceFile, uint64_t offset){
  fseek( deviceFile, offset , SEEK_SET );
  fwrite( srcTable, 1, tableSize, deviceFile );
}







int createPart(struct partTable *table, uint64_t stLBA, uint64_t endLBA, uint64_t flags, char *name){
  uint32_t numParts;   //Number of partition entries. Found in GPT
  uint32_t partNum;
  struct partEntry *newPart;
  numParts   = table->numParts;
  
  for(partNum = 0; partNum < numParts; partNum++){
    newPart = &table->entries[partNum];
    if( (newPart->firstLBA | newPart->lastLBA | newPart->flags) == 0){break;}
  }

  char *UTF16name = (char *)calloc(1, 72);
  int c;
  for(c=0; c<72; c++){
    if(name[c] == '\0'){break;}
    UTF16name[2*c] = name[c];
  }

  /*
   * I was hoping that TT's libuuid had all the uuid manipulating power
   * However, as I can see from Rod Smith's gdisk code, manual bit flipping
   * is required :(
   */ 
  uuid_t partGUID;
  uuid_generate(partGUID);
  char charPartUUID[16];
  uuid_to_char(charPartUUID, partGUID);
  
  char UUIDlinux[37] = "0FC63DAF-8483-4772-8E79-3D69D8477DE4\0"; 
  uuid_t typeGUID;
  if(uuid_parse(UUIDlinux, typeGUID))
    return -1;
  char charUUID[16];
  uuid_to_char(charUUID, typeGUID);


  memcpy(newPart->typeGUID, charUUID, 16);
  memcpy(newPart->partGUID, charPartUUID, 16);
  newPart->firstLBA = stLBA;
  newPart->lastLBA  = endLBA;
  newPart->flags    = flags;
  memcpy(newPart->name, UTF16name, 72);
 
  return 0;
}

void uuid_to_char(char* out, uuid_t in){
  memcpy(out, in, 16);
  char temp;
  temp   = out[0];
  out[0] = out[3];
  out[3] = temp;
  temp   = out[1];
  out[1] = out[2];
  out[2] = temp;
  temp   = out[4];
  out[4] = out[5];
  out[5] = temp;
  temp   = out[6];
  out[6] = out[7];
  out[7] = temp;
}

void char_to_uuid(uuid_t out, char* in){
  
}


//Simply zero out the partition entry on disk.
//
//int deletePart( struct partTable *table, uuid_t partGUID){
//  uint32_t part;
//  struct partEntry *current;
//  for(part=0; part<table->numParts; part++){
//    current = &table->entries[part];
//    if(!uuid_compare(partGUID, uuid_t uu2))
//  }
//}


int verifyPartTable(struct GPTHeader *header, struct partTable *table){
  return header->crc32Part == crc32PartTable(table);
}


uint32_t crc32PartTable(struct partTable *table){
  char *charTable;
  charTable = (char *)calloc(1, 128*128);
  partTableToChar(charTable, table); 
  return crc32(0, (unsigned char*)charTable, 128*128); 
}


void partTableToChar(char* charTable, struct partTable *table){
  uint64_t entNum = 0;
  struct partEntry *entries = table->entries;
  for(entNum=0; entNum < (table->numParts); entNum++){
    partEntryToChar(charTable, &(entries[entNum]), 128*entNum, table->singleSize);
  }
}


void partEntryToChar(char* charTable, struct partEntry *entry, uint64_t start, uint32_t length){
  memcpy( charTable + start +  0, &entry->typeGUID,  16 );
  memcpy( charTable + start + 16, &entry->partGUID,  16 );
  memcpy( charTable + start + 32, &entry->firstLBA,   8 );
  memcpy( charTable + start + 40, &entry->lastLBA,    8 );
  memcpy( charTable + start + 48, &entry->flags,      8 );
  memcpy( charTable + start + 56, &entry->name,      72 );
  //TODO: Fill remaining space (length -128) with zeros
}  
