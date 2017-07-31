#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int isGPT(FILE *deviceFile){
  char gptSig[9] = "EFI PART";
  char diskSig[9]; 
  
  fseek( deviceFile, 512 , SEEK_SET );
  fread(diskSig, 1, 8, deviceFile);

  if(strcmp(gptSig, diskSig) == 0){return 1;}
  return 0;
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
void getGPGFields(char* GPTHeader){
  char signature[9];
  char revision[5];

  strncpy(signature, GPTHeader, 8);
  strncpy(revision, GPTHeader+8, 4);
 
  printf("%s\n", revision);
}



int verifyHeaders(char *header1, char* header2){
 int identical = 0;
 if(strcmp(header1, header2) == 0){identical = 1;}

 //Do CRC32 header checks here!
 
 return identical;
}



int main(){
  FILE *deviceFile;
  deviceFile=fopen("/dev/sdb", "r");

  isGPT(deviceFile);

  char* GPTHeader1 = (char *)malloc(512);
  char* GPTHeader2 = (char *)malloc(512);
  getPrimaryHeader(GPTHeader1, deviceFile);
  getSecondaryHeader(GPTHeader2, deviceFile); 

  int identicalHeaders = verifyHeaders(GPTHeader1, GPTHeader2);
  if(!identicalHeaders){
    printf("Your GPT Headers don't match\n");
  }
  
  

  //getGPGFields(GPTHeader);  
  return 0;
}
