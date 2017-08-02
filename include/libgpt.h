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

#define HEADER_SIZE 512
#define LBA_SIZE 512

struct GPTHeader{
  char               signature[8];
  char               revision[4];
  unsigned int       headerSize;   //92 in most cases. 512-92 is just 0s
  unsigned int       crc32;        //different for primary and backup
  unsigned int       reserved;
  unsigned long long LBA1, LBA2;   //swapped for backup
  unsigned long long LBApart, LBApartLast; //different for primary and backup
  char               GUID[16];
  unsigned long long LBAstart;     //LBA of partition entries. Different on Pr and Ba
  unsigned int       numParts;
  unsigned int       singleSize;
  unsigned int       crc32Part;
};


/*
Return 1 if the device is GPT
Return 0 otherwise
This function checks both the default and backup GPT headers.
*/
int isGPT(FILE *deviceFile);


unsigned long long getPrimaryHeaderOffset();
unsigned long long getSecondaryHeaderOffset(FILE *deviceFile);


/*
Returns 1 if the calculated CRC32 matches the on disk CRC32
Returns 0 otherwise
*/
int verifyGPT(struct GPTHeader *header);



//Populates the GPTHeader struct
void readGPT(struct GPTHeader *header, FILE *deviceFile, unsigned long long offset);

void readCharGPT(char *dstHeader, FILE *deviceFile, unsigned long long offset);

void charToGPTHeader(struct GPTHeader *dst, char *src);


/*
Write header to disk
This does not alter the partition table
Returns 0 on clean write, -1 on failure
*/
int writeCharGPT(char *srcHeader, FILE *deviceFile, unsigned long long offset);

/*
Writes the GPT found in the struct GPTHeader to disk
both primary and backup
Returns 0 on clean write, -1 on failure
*/
int writeGPT(struct GPTHeader *header, FILE *deviceFile, unsigned long long offset);

void GPTHeaderToChar(char *dst, struct GPTHeader *src);



/*
Builds a fresh GPTHeader pair
No Return value
*/
void buildGPT(struct GPTHeader *GPTHeader1, struct GPTHeader *GPTHeader2);
