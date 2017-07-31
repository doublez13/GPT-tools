#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>


//Return 1 if the device is GPT
//Return 0 otherwise
int isGPT(FILE *deviceFile){
  int headerSize = 512;
  int offset;
  char gptSig[9] = "EFI PART";
  char diskSig[9];
  int GPT1 = 0;
  int GPT2 = 0;
 
  //Header 1 
  fseek( deviceFile, 512 , SEEK_SET );
  fread(diskSig, 1, 8, deviceFile);
  if(strcmp(gptSig, diskSig) == 0){GPT1 = 1;}

  //Header 2
  fseek(deviceFile, 0, SEEK_END);
  offset = ftell(deviceFile);
  offset -= headerSize;
  fseek( deviceFile, offset , SEEK_SET );
  fread(diskSig, 1, 8, deviceFile);
  if(strcmp(gptSig, diskSig) == 0){GPT2 = 1;}
  
  return GPT1 | GPT2;
}

void getPrimaryHeader(char* header, FILE *deviceFile){
  int headerSize = 512;
  int offset = 512;
  
  fseek( deviceFile, offset, SEEK_SET );  
  fread(header, 1, headerSize, deviceFile);
}

void getSecondaryHeader(char* header, FILE *deviceFile){
  int headerSize = 512;
  int offset;

  fseek(deviceFile, 0, SEEK_END);
  offset = ftell(deviceFile);
  offset -= headerSize;  

  fseek( deviceFile, offset, SEEK_SET );
  fread(header, 1, headerSize, deviceFile);
}



//Keep working on this later
//What type should we store the data in?
//void getGPGFields(char* GPTHeader){
//  char signature[9];
//  char revision[5];
//
//  strncpy(signature, GPTHeader, 8);
//  strncpy(revision, GPTHeader+8, 4);
// 
//  printf("%s\n", revision);
//}



int verifyHeaders(char *header1, char* header2){
 int identical = 0;
 if(strcmp(header1, header2) == 0){identical = 1;}

 //Do CRC32 header checks here!
 
 return identical;
}


//Copy the srcHeader to the Destination and write to disk
//This does not alter the partition table
int copyHeaders(char *srcHeader, char *destHeader, FILE *deviceFile){
  long offset = 512;
  int headerSize = 512;
  int ret1, ret2;

  fseek( deviceFile, offset , SEEK_SET );
  ret1 = fwrite( srcHeader, 1, headerSize, deviceFile );

  fseek(deviceFile, 0, SEEK_END);
  offset = ftell(deviceFile);
  offset -= headerSize;
  fseek( deviceFile, offset , SEEK_SET );
  ret2 = fwrite( srcHeader, 1, headerSize, deviceFile );

  if((ret1 & ret2) == headerSize){return 0;}
  return -1;
}



int main(){
  FILE *deviceFile;
  char path[10] = "/dev/sdb";
  
  if(access(path, F_OK)){
    printf("No Such Device\n");
    return 0;
  }
  deviceFile=fopen(path, "w+");

  if(!isGPT(deviceFile)){
    printf("Not a GPT device\n");
    return 0;
  }

  char* GPTHeader1 = (char *)malloc(512);
  char* GPTHeader2 = (char *)malloc(512);
  getPrimaryHeader(GPTHeader1, deviceFile);
  getSecondaryHeader(GPTHeader2, deviceFile); 

  int identicalHeaders = verifyHeaders(GPTHeader1, GPTHeader2);
  if(!identicalHeaders){
    printf("Your GPT Headers don't match\n");
    printf("Fixing...\n");
    //See which one (maybe both) has and an invalid checksum
    //copyHeaders(GPTHeader1, GPTHeader2, deviceFile);
    copyHeaders(GPTHeader2, GPTHeader1, deviceFile);
    getPrimaryHeader(GPTHeader1, deviceFile);
    getSecondaryHeader(GPTHeader2, deviceFile);
    if(verifyHeaders(GPTHeader1, GPTHeader2)){
      printf("Fixed\n");
    }
    else{
      printf("GPT header could not be restored...");
      printf("Exiting...");
    }
  }
  
  

  //getGPGFields(GPTHeader);  
  return 0;
}
