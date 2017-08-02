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
  printf("Testing Completed\n");
  return 0;
}
