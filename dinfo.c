#include <stdio.h>
#include <string.h>

int isGPT(FILE *deviceFile){
  char gptSig[9] = "EFI PART";
  char diskSig[9]; 
  
  fseek( deviceFile , 512 , SEEK_SET );
  fread(diskSig, 1, 8, deviceFile);

  if(strcmp(gptSig, diskSig) == 0){return 1;}
  return 0;
}

void GPT_CHKSUM(char* chksum, FILE *deviceFile){
  int offset = 16;

  fseek( deviceFile , 512+offset , SEEK_SET );  
  fread(chksum, 1, 4, deviceFile);
}


int main(){
  FILE *deviceFile;
  deviceFile=fopen("/dev/sdc", "r");

  isGPT(deviceFile);
  char chksum[5];
  GPT_CHKSUM(chksum, deviceFile);
  return 0;
}
