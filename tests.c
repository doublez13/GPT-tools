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
#include "include/libgpt.h"

int main(){
  FILE *deviceFile;
  char path[10] = "/dev/sdc";
  
  if(access(path, F_OK)){
    printf("Device %s not found \n", path);
    return 0;
  }
  deviceFile=fopen(path, "w+");

  if(!isGPT(deviceFile)){
    printf("No GPT header found on %s\n", path);
    return 0;
  }


  //Reads the Primary and Secondary GPTs
  struct GPTHeader *GPTHeader1 = calloc(1,sizeof(struct GPTHeader));
  struct GPTHeader *GPTHeader2 = calloc(1,sizeof(struct GPTHeader));
  readGPT( GPTHeader1, deviceFile, getPrimaryHeaderOffset() );
  readGPT( GPTHeader2, deviceFile, getSecondaryHeaderOffset(deviceFile) ); 
  if( !verifyGPT(GPTHeader1) ){printf("Primary header corrupted\n");}
  if( !verifyGPT(GPTHeader2) ){printf("Secondary header corrupted\n");}
 
  //Writes the Primary and Secondary GPTs back on the disk
  //Reads the Primary and Secondary GPTs
  writeGPT( GPTHeader1, deviceFile, getPrimaryHeaderOffset() );
  writeGPT( GPTHeader2, deviceFile, getSecondaryHeaderOffset(deviceFile) );
  free(GPTHeader1);
  free(GPTHeader2);
  GPTHeader1 = calloc(1,sizeof(struct GPTHeader));
  GPTHeader2 = calloc(1,sizeof(struct GPTHeader));
  readGPT(GPTHeader1, deviceFile, getPrimaryHeaderOffset() );
  readGPT(GPTHeader2, deviceFile, getSecondaryHeaderOffset(deviceFile) );
  if( !verifyGPT(GPTHeader1) ){printf("Primary header corrupted\n");}
  if( !verifyGPT(GPTHeader2) ){printf("Secondary header corrupted\n");}

 

  fclose(deviceFile);
  printf("Testing Completed\n");
  return 0;
}
