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
#include <stdint.h>
#include <uuid/uuid.h>

#define HEADER_SIZE 512
#define LBA_SIZE 512

struct GPTHeader{
  char     signature[8];
  char     revision[4];
  uint32_t headerSize;   //92 in most cases. 512-92 is just 0s
  uint32_t crc32;        //different for primary and backup
  uint32_t reserved;
  uint64_t LBA1, LBA2;   //swapped for backup
  uint64_t LBAfirstUse, LBAlastUse; //different for primary and backup
  uuid_t   GUID;
  uint64_t LBApartStart;     //LBA of partition entries. Different on Pr and Ba
  uint32_t numParts;
  uint32_t singleSize;
  uint32_t crc32Part;
};

/*
Return 1 if the device is GPT
Return 0 otherwise
This function checks both the default and backup GPT headers.
*/
int isGPT(FILE *deviceFile);


uint64_t getPrimaryHeaderOffset();
uint64_t getSecondaryHeaderOffset(FILE *deviceFile);


/*
Returns 1 if the calculated CRC32 matches the on disk CRC32
Returns 0 otherwise
*/
int verifyGPT(struct GPTHeader *header);

uint32_t crc32GPT(struct GPTHeader *header);


//Populates the GPTHeader struct
void readGPT(struct GPTHeader *header, FILE *deviceFile, uint64_t offset);

void readCharGPT(char *dstHeader, FILE *deviceFile, uint64_t offset);

void charToGPTHeader(struct GPTHeader *dst, char *src);


/*
Write header to disk
This does not alter the partition table
Returns 0 on clean write, -1 on failure
*/
int writeCharGPT(char *srcHeader, FILE *deviceFile, uint64_t offset);

/*
Writes the GPT found in the struct GPTHeader to disk
both primary and backup
Returns 0 on clean write, -1 on failure
*/
int writeGPT(struct GPTHeader *header, FILE *deviceFile, uint64_t offset);

void GPTHeaderToChar(char *dst, struct GPTHeader *src);


/*
Builds a fresh GPTHeader pair
No Return value
*/
struct partTable* buildGPT(struct GPTHeader *primary, struct GPTHeader *backup, uint64_t maxLBA);







//////PARTITIONS///////
struct partTable{
  uint32_t numParts;             //Number of partition entries. Found in GPT
  uint32_t singleSize;           //Size of a single partition entry. Found in GPT
  struct   partEntry *entries;   //Pointer to copied partition table. Should be (numParts * singleSize) in length
};

struct partEntry{
  uuid_t   typeGUID;
  uuid_t   partGUID;
  uint64_t firstLBA;
  uint64_t lastLBA;
  uint64_t flags;
  char     name[72]; //UTF-16, Max 36 chars
};

struct partTable* readPartTable( struct GPTHeader *header, FILE *deviceFile);
void readCharPartTable(char *dstHeader, uint64_t tableSize, FILE *deviceFile, uint64_t offset);

void charToPartEntry(struct partEntry *entry, char* partTable, uint64_t start, uint32_t length);
void partEntryToChar(char* charTable, struct partEntry *entry, uint64_t start, uint32_t length);
void partTableToChar(char* charTable, struct partTable *table);


void writePartTable(struct GPTHeader *header, uint64_t offset, struct partTable *table, FILE *deviceFile);
void writeCharPartTable(char *srcTable, uint64_t tableSize, FILE *deviceFile, uint64_t offset);


//void createPartTable(struct partTable *table, uint64_t stLBA, uint64_t endLBA, uint64_t flags, char *name);

void genHeaderFromBackup(struct GPTHeader *new ,struct partTable *newTable,
struct GPTHeader *working, struct partTable *workingTable);

int createPart(struct partTable *table, uint64_t stLBA, uint64_t endLBA, uint64_t flags, char *name);
void uuid_to_char(unsigned char* out, uuid_t in);
void char_to_uuid(uuid_t out, unsigned char* in);
int deletePart( struct partTable *table, char* strGUID);

int verifyPartTable(struct GPTHeader *header, struct partTable *table);
uint32_t crc32PartTable(struct partTable *table);
