#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define HEADER_SIZE 512


struct GPTHeader{
  char signature[8];
  char revision[4];
  unsigned int headerSize; //92 in most cases. 512-92 is just 0s
  unsigned int crc32;
  unsigned int reserved;
  unsigned long long LBA1, LBA2;
  unsigned long long LBApart, LBApartLast;
  char GUID[16];
  char LBAstart[8];
  unsigned int numParts;
  unsigned int singleSize;
  unsigned int crc32Part;
};


//Return 1 if the device is GPT
//Return 0 otherwise
//This function checks both the default and
//backup GPT headers.
int isGPT(FILE *deviceFile){
  int offset = 512;
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


void getPrimaryHeader(char* header, FILE *deviceFile){
  int offset = 512;
  
  fseek( deviceFile, offset, SEEK_SET );  
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

//THIS CURRENTLY ISN'T WORKING
int verifyHeaders(char *header1, char* header2){
  int identical = 0;
  if(strcmp(header1, header2) == 0){identical = 1;}

 //Do CRC32 header checks here!

  return identical;
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


//Write header to disk
//This does not alter the partition table
//Returns 0 on clean write, -1 on failure
int writeCharGPT(char *srcHeader, FILE *deviceFile){
  long offset = 512;
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

//Writes the GPT found in the struct GPTHeader to disk
//both primary and backup
//Returns 0 on clean write, -1 on failure
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



int main(){
  FILE *deviceFile;
  char path[10] = "/dev/sdd";
  
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

  //int identicalHeaders = verifyHeaders(GPTHeader1, GPTHeader2);
  //if(!identicalHeaders){
  //  printf("Your GPT Headers don't match\n");
  //  printf("Fixing...\n");
  //  //See which one (maybe both) has and an invalid checksum
  //  //copyHeaders(GPTHeader1, GPTHeader2, deviceFile);
  //  writeCharGPT(GPTHeader2, deviceFile);
  //  getPrimaryHeader(GPTHeader1, deviceFile);
  //  getSecondaryHeader(GPTHeader2, deviceFile);
  //  if(verifyHeaders(GPTHeader1, GPTHeader2)){
  //    printf("Fixed\n");
  //  }
  //  else{
  //    printf("GPT header could not be restored...");
  //    printf("Exiting...");
  //  }
  //}
  
  
  struct GPTHeader *header = calloc(1,sizeof(struct GPTHeader));
  readGPT(header, GPTHeader1);
  writeGPT(header, deviceFile);
  return 0;
}
