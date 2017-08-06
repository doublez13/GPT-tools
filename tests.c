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
#include <wchar.h>
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
    printf("\nNo GPT header found on %s\n", path);
    return 0;
  }
  else{
    printf("\nGPT HEADER FOUND on device %s\n", path);
  }



  //Reads the Primary and Secondary GPTs
  printf("Test 01: Verify on disk GPTs\n");
  struct GPTHeader *GPTHeader1 = calloc(1,sizeof(struct GPTHeader));
  struct GPTHeader *GPTHeader2 = calloc(1,sizeof(struct GPTHeader));
  readGPT( GPTHeader1, deviceFile, getPrimaryHeaderOffset() );
  readGPT( GPTHeader2, deviceFile, getSecondaryHeaderOffset(deviceFile) ); 
  if( !verifyGPT(GPTHeader1) ){
    printf("Test 01: Primary header corrupted\n");
    return 0;
  }
  if( !verifyGPT(GPTHeader2) ){
    printf("Test 01: Secondary header corrupted\n");
    return 0;
  }
  printf("Test 01: Passed\n\n");


  //Writes the Primary and Secondary GPTs back on the disk
  //Reads the Primary and Secondary GPTs
  printf("Test 02: Verify GPT writes to disk\n");
  writeGPT( GPTHeader1, deviceFile, getPrimaryHeaderOffset() );
  writeGPT( GPTHeader2, deviceFile, getSecondaryHeaderOffset(deviceFile) );
  free(GPTHeader1);
  free(GPTHeader2);
  GPTHeader1 = calloc(1,sizeof(struct GPTHeader));
  GPTHeader2 = calloc(1,sizeof(struct GPTHeader));
  readGPT(GPTHeader1, deviceFile, getPrimaryHeaderOffset() );
  readGPT(GPTHeader2, deviceFile, getSecondaryHeaderOffset(deviceFile) );
  if( !verifyGPT(GPTHeader1) ){
    printf("Test 02: Primary header corrupted\n");
    return 0;
  }
  if( !verifyGPT(GPTHeader2) ){
    printf("Test 02: Secondary header corrupted\n");
    return 0;
  }
  printf("Test 02: Passed\n\n");





  printf("Test 03: Verify Partition Table\n");
  free(GPTHeader1);
  free(GPTHeader2);
  GPTHeader1 = calloc(1,sizeof(struct GPTHeader));
  GPTHeader2 = calloc(1,sizeof(struct GPTHeader));
  readGPT(GPTHeader1, deviceFile, getPrimaryHeaderOffset() );
  readGPT(GPTHeader2, deviceFile, getSecondaryHeaderOffset(deviceFile) );
  struct partTable* partTable1 = readPartTable( GPTHeader1, deviceFile);
  if( !verifyPartTable(GPTHeader1, partTable1) ){
    printf("Test 03: Primary Part Table corrupted\n");
    return 0;
  }
  struct partTable* partTable2 = readPartTable( GPTHeader2, deviceFile);
  if( !verifyPartTable(GPTHeader2, partTable2) ){
    printf("Test 03: Secondary Part Table corrupted\n");
    return 0;
  }
  printf("Test 03: Passed\n\n");




  printf("Test 04: Verify Partition Tables match\n"); 
  if( crc32PartTable(partTable1) != crc32PartTable(partTable2) ){
    printf("Primary and backup partition tables don't match\n");
    return 0;
  }
  printf("Test 04: Passed\n\n");



  printf("Test 05: Corrupt Primary Header. Then repair\n");
  char garbage[100] = "lkfsdalkfjklajsdklfjklsadjfkljaskldjfkllasdjfklj";
  fseek( deviceFile, 512 , SEEK_SET );
  fwrite( garbage, 1, 100, deviceFile );
  free(GPTHeader1);
  free(GPTHeader2);
  GPTHeader1 = calloc(1,sizeof(struct GPTHeader));
  GPTHeader2 = calloc(1,sizeof(struct GPTHeader));
  readGPT(GPTHeader2, deviceFile, getSecondaryHeaderOffset(deviceFile) );
  partTable1 = readPartTable( GPTHeader1, deviceFile);
  partTable2 = readPartTable( GPTHeader2, deviceFile);
  genHeaderFromBackup(GPTHeader1, partTable1, GPTHeader2, partTable2);
  writeGPT( GPTHeader1, deviceFile, getPrimaryHeaderOffset() );
  free(GPTHeader1);
  free(GPTHeader2);
  GPTHeader1 = calloc(1,sizeof(struct GPTHeader));
  readGPT(GPTHeader1, deviceFile, getPrimaryHeaderOffset() );
  if( !verifyGPT(GPTHeader1) ){
    printf("Test 05: Primary header could not be repaired\n");
    return 0;
  } 
  printf("Test 05: Passed\n\n");




  printf("Test 06: Corrupt Primary Header and Primary GPT Table. Then repair.\n");
  char garbage2[92+128*128];
  int i;
  for(i=0; i < 92+128*128; i++){garbage2[i] = 'g';}
  fseek( deviceFile, 512 , SEEK_SET );
  fwrite( garbage2, 1, 92+128*128, deviceFile );
  free(GPTHeader1);
  GPTHeader1 = calloc(1,sizeof(struct GPTHeader));
  GPTHeader2 = calloc(1,sizeof(struct GPTHeader));
  readGPT(GPTHeader1, deviceFile, getPrimaryHeaderOffset() );
  if( !verifyGPT(GPTHeader1) ){
    printf("Test 06: We've corrupted the primary GPT header AND the primary part table\n");
  }

  readGPT(GPTHeader2, deviceFile, getSecondaryHeaderOffset(deviceFile) );
  partTable2 = readPartTable( GPTHeader2, deviceFile);
  genHeaderFromBackup(GPTHeader1, partTable1, GPTHeader2, partTable2);
  writeGPT( GPTHeader1, deviceFile, getPrimaryHeaderOffset() ); //Fix the header first
  writePartTable(GPTHeader1,  getPrimaryHeaderOffset(), partTable1, deviceFile);
  
  free(GPTHeader1);
  free(GPTHeader2);
  GPTHeader1 = calloc(1,sizeof(struct GPTHeader));
  readGPT(GPTHeader1, deviceFile, getPrimaryHeaderOffset() );
  partTable1 = readPartTable( GPTHeader1, deviceFile);
  if( !verifyGPT(GPTHeader1) ){
    printf("Test 06: Primary header could not be repaired\n");
    return 0;
  }
  if( !verifyPartTable(GPTHeader1, partTable1) ){
    printf("Test 06: Primary partition table could not be repaired\n");
    return 0;
  }
  free(GPTHeader1);
  printf("Test 06: Passed\n\n");

  

  printf("Test 07: Creating a new Partition\n");
  GPTHeader1 = calloc(1,sizeof(struct GPTHeader));
  GPTHeader2 = calloc(1,sizeof(struct GPTHeader));
  readGPT(GPTHeader1, deviceFile, getPrimaryHeaderOffset() );
  readGPT(GPTHeader2, deviceFile, getSecondaryHeaderOffset(deviceFile) );

  partTable1 = readPartTable( GPTHeader1, deviceFile);
  createPart(partTable1, 40, 99999, 0, "TEST PART\0");
  writePartTable(GPTHeader1,  getPrimaryHeaderOffset(), partTable1, deviceFile);
  writePartTable(GPTHeader2,  getSecondaryHeaderOffset(deviceFile), partTable1, deviceFile);
  free(GPTHeader1);
  free(GPTHeader2);

  fclose(deviceFile);
  printf("ALL TESTS PASSED\n");
  return 0;
}
