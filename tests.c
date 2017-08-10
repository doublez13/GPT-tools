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
  char path[10] = "/dev/sdb";
  
  if(access(path, F_OK)){
    printf("Device %s not found \n", path);
    return 0;
  }
  deviceFile=fopen(path, "w+");

  if(!is_gpt(deviceFile)){
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
  read_gpt( GPTHeader1, deviceFile, get_primary_header_offset() );
  read_gpt( GPTHeader2, deviceFile, get_secondary_header_offset(deviceFile) ); 
  if( !verify_gpt(GPTHeader1) ){
    printf("Test 01: Primary header corrupted\n");
    return 0;
  }
  if( !verify_gpt(GPTHeader2) ){
    printf("Test 01: Secondary header corrupted\n");
    return 0;
  }
  printf("Test 01: Passed\n\n");


  //Writes the Primary and Secondary GPTs back on the disk
  //Reads the Primary and Secondary GPTs
  printf("Test 02: Verify GPT writes to disk\n");
  write_gpt( GPTHeader1, deviceFile, get_primary_header_offset() );
  write_gpt( GPTHeader2, deviceFile, get_secondary_header_offset(deviceFile) );
  free(GPTHeader1);
  free(GPTHeader2);
  GPTHeader1 = calloc(1,sizeof(struct GPTHeader));
  GPTHeader2 = calloc(1,sizeof(struct GPTHeader));
  read_gpt(GPTHeader1, deviceFile, get_primary_header_offset() );
  read_gpt(GPTHeader2, deviceFile, get_secondary_header_offset(deviceFile) );
  if( !verify_gpt(GPTHeader1) ){
    printf("Test 02: Primary header corrupted\n");
    return 0;
  }
  if( !verify_gpt(GPTHeader2) ){
    printf("Test 02: Secondary header corrupted\n");
    return 0;
  }
  printf("Test 02: Passed\n\n");





  printf("Test 03: Verify Partition Table\n");
  free(GPTHeader1);
  free(GPTHeader2);
  GPTHeader1 = calloc(1,sizeof(struct GPTHeader));
  GPTHeader2 = calloc(1,sizeof(struct GPTHeader));
  read_gpt(GPTHeader1, deviceFile, get_primary_header_offset() );
  read_gpt(GPTHeader2, deviceFile, get_secondary_header_offset(deviceFile) );
  struct partTable* partTable1 = read_partTable( GPTHeader1, deviceFile);
  if( !verify_partTable(GPTHeader1, partTable1) ){
    printf("Test 03: Primary Part Table corrupted\n");
    return 0;
  }
  struct partTable* partTable2 = read_partTable( GPTHeader2, deviceFile);
  if( !verify_partTable(GPTHeader2, partTable2) ){
    printf("Test 03: Secondary Part Table corrupted\n");
    return 0;
  }
  printf("Test 03: Passed\n\n");




  printf("Test 04: Verify Partition Tables match\n"); 
  if( crc32_partTable(partTable1) != crc32_partTable(partTable2) ){
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
  read_gpt(GPTHeader2, deviceFile, get_secondary_header_offset(deviceFile) );
  partTable1 = read_partTable( GPTHeader1, deviceFile);
  partTable2 = read_partTable( GPTHeader2, deviceFile);
  header_from_backup(GPTHeader1, partTable1, GPTHeader2, partTable2);
  write_gpt( GPTHeader1, deviceFile, get_primary_header_offset() );
  free(GPTHeader1);
  free(GPTHeader2);
  GPTHeader1 = calloc(1,sizeof(struct GPTHeader));
  read_gpt(GPTHeader1, deviceFile, get_primary_header_offset() );
  if( !verify_gpt(GPTHeader1) ){
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
  read_gpt(GPTHeader1, deviceFile, get_primary_header_offset() );
  if( !verify_gpt(GPTHeader1) ){
    printf("Test 06: We've corrupted the primary GPT header AND the primary part table\n");
  }

  read_gpt(GPTHeader2, deviceFile, get_secondary_header_offset(deviceFile) );
  partTable2 = read_partTable( GPTHeader2, deviceFile);
  header_from_backup(GPTHeader1, partTable1, GPTHeader2, partTable2);
  write_gpt( GPTHeader1, deviceFile, get_primary_header_offset() ); //Fix the header first
  write_partTable(GPTHeader1,  get_primary_header_offset(), partTable1, deviceFile);
  
  free(GPTHeader1);
  free(GPTHeader2);
  GPTHeader1 = calloc(1,sizeof(struct GPTHeader));
  read_gpt(GPTHeader1, deviceFile, get_primary_header_offset() );
  partTable1 = read_partTable( GPTHeader1, deviceFile);
  if( !verify_gpt(GPTHeader1) ){
    printf("Test 06: Primary header could not be repaired\n");
    return 0;
  }
  if( !verify_partTable(GPTHeader1, partTable1) ){
    printf("Test 06: Primary partition table could not be repaired\n");
    return 0;
  }
  free(GPTHeader1);
  printf("Test 06: Passed\n\n");

  
//FIX THESE TO GRAB THE GUID MADE HERE SO WE CAN DELETE IT LATER
  printf("Test 07: Creating a new Partition\n");
  GPTHeader1 = calloc(1,sizeof(struct GPTHeader));
  GPTHeader2 = calloc(1,sizeof(struct GPTHeader));
  read_gpt(GPTHeader1, deviceFile, get_primary_header_offset() );
  read_gpt(GPTHeader2, deviceFile, get_secondary_header_offset(deviceFile) );

  partTable1 = read_partTable( GPTHeader1, deviceFile);
  if(create_part(partTable1, 2048, 99999, 0, "TEST PART\0")){
    printf("Test 07: Failed to create the partition\n");
    return -1;
  }
  write_partTable(GPTHeader1,  get_primary_header_offset(), partTable1, deviceFile);
  write_partTable(GPTHeader2,  get_secondary_header_offset(deviceFile), partTable1, deviceFile);
  free(GPTHeader1);
  free(GPTHeader2);



  printf("Test 08: Deleting the Partition\n");
  GPTHeader1 = calloc(1,sizeof(struct GPTHeader));
  GPTHeader2 = calloc(1,sizeof(struct GPTHeader));
  read_gpt(GPTHeader1, deviceFile, get_primary_header_offset() );
  read_gpt(GPTHeader2, deviceFile, get_secondary_header_offset(deviceFile) );

  partTable1 = read_partTable( GPTHeader1, deviceFile);
  if( delete_part( partTable1, "FAB11E4D-BFF7-4D1B-8A64-8F6A053DB907" ) ){
    printf("Test 08: Failed to delete the partition\n");
    return -1;
  }
  write_partTable(GPTHeader1,  get_primary_header_offset(), partTable1, deviceFile);
  write_partTable(GPTHeader2,  get_secondary_header_offset(deviceFile), partTable1, deviceFile);
  free(GPTHeader1);
  free(GPTHeader2);



  fclose(deviceFile);
  printf("ALL TESTS PASSED\n");
  return 0;
}
