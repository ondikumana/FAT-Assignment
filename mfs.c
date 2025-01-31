/*
  Name: Olivier Ndikumana
  ID: 1001520973
*/

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

FILE *fp;

#define BPB_BytesPerSec_Offset 11
#define BPB_BytesPerSec_Size 2

#define BPB_SecPerClus_Offset 13
#define BPB_SecPerClus_Size 1

#define BPB_RsvdSecCnt_Offset 14
#define BPB_RsvdSecCnt_Size 2

#define BPB_NumFATs_Offset 16
#define BPB_NumFATs_Size 1

#define BPB_RootEntCnt_Offset 17
#define BPB_RootEntCnt_Size 2

#define BPB_FATSz32_Offset 36
#define BPB_FATSz32_Size 4

#define BS_Vollab_Offset 71
#define BS_Vollab_Size 11

#define Volume_Name_Offset 71 // volume label/name offset
#define Volume_Name_Size 11  // volume length

#define MAX_FILE_NAME_SIZE 20
#define MAX_COMMAND_SIZE 200
#define MAX_NUM_ARGUMENTS 5
#define WHITESPACE " \t\n"

#define NUMBER_OF_ENTRIES 16 // fixed number of entries
#define LENTH_OF_DIR_NAME 11 // fixed length of directory/file name

struct __attribute__((__packed__)) DirectoryEntry {
  char DIR_Name[11];
  uint8_t Dir_Attr;
  uint8_t Unused1[8];
  uint16_t DIR_FirstClusterHigh;
  uint8_t Unused[4];
  uint16_t DIR_FirstClusterLow;
  uint32_t DIR_FileSize;
};

struct DirectoryEntry dir[NUMBER_OF_ENTRIES];

uint16_t BPB_BytesPerSec;
uint8_t BPB_SecPerClus;
uint16_t BPB_RsvdSecCnt;
uint8_t BPB_NumFATs;
uint32_t BPB_FATSz32;
uint16_t BPB_RootEntCnt;
uint32_t RootClusAddress;
uint32_t currentClusAddress;
char BS_Vollab[11];
char Volume_Name[11];

char * currentlyOpenFileName; // contains the name of currently open file

int16_t nextLB(uint32_t sector) {
  uint32_t FATAddress = (BPB_BytesPerSec * BPB_RsvdSecCnt) + (sector * 4);
  int16_t val;
  fseek(fp, FATAddress, SEEK_SET);
  fread(&val, 2, 1, fp);
  return val;
}

void getInfo() {
  /* This function reads the img given the offsets and finds the appropriate values*/
  if (!fp) return;
  //BPB_BytesPerSec
  fseek(fp, BPB_BytesPerSec_Offset, SEEK_SET);
  fread(&BPB_BytesPerSec, BPB_BytesPerSec_Size, 1, fp);

  //BPB_SecPerClus
  fseek(fp, BPB_SecPerClus_Offset, SEEK_SET);
  fread(&BPB_SecPerClus, BPB_SecPerClus_Size, 1, fp);

  //BPB_RsvdSecCnt
  fseek(fp, BPB_RsvdSecCnt_Offset, SEEK_SET);
  fread(&BPB_RsvdSecCnt, BPB_RsvdSecCnt_Size, 1, fp);

  //BPB_NumFATs
  fseek(fp, BPB_NumFATs_Offset, SEEK_SET);
  fread(&BPB_NumFATs, BPB_NumFATs_Size, 1, fp);

  //BPB_RootEntCnt
  fseek(fp, BPB_RootEntCnt_Offset, SEEK_SET);
  fread(&BPB_RootEntCnt, BPB_RootEntCnt_Size, 1, fp);

  //BPB_FATSz32
  fseek(fp, BPB_FATSz32_Offset, SEEK_SET);
  fread(&BPB_FATSz32, BPB_FATSz32_Size, 1, fp);

  //BS_Vollab
  fseek(fp, BS_Vollab_Offset, SEEK_SET);
  fread(&BS_Vollab, BS_Vollab_Size, 1, fp);

  //Volume Name
  fseek(fp, Volume_Name_Offset, SEEK_SET);
  fread(&Volume_Name, Volume_Name_Size, 1, fp);

  // sets RootClusAddress
  RootClusAddress = currentClusAddress = (BPB_NumFATs * BPB_FATSz32 * BPB_BytesPerSec) + (BPB_RsvdSecCnt * BPB_BytesPerSec);
}

void printInfo() {
  /* This function prints the info from getInfo(). excluding some of them like volume name */

  if (!fp) return;

  //BPB_BytesPerSec
  printf("BPB_BytesPerSec: %d %x\n", BPB_BytesPerSec, BPB_BytesPerSec);

  //BPB_SecPerClus
  printf("BPB_SecPerClus: %d %x\n", BPB_SecPerClus, BPB_SecPerClus);

  //BPB_RsvdSecCnt
  printf("BPB_RsvdSecCnt: %d %x\n", BPB_RsvdSecCnt, BPB_RsvdSecCnt);

  //BPB_NumFATs
  printf("BPB_NumFATs: %d %x\n", BPB_NumFATs, BPB_NumFATs);

  //BPB_RootEntCnt
  printf("BPB_RootEntCnt: %d %x\n", BPB_RootEntCnt, BPB_RootEntCnt);

  //BPB_FATSz32
  printf("BPB_FATSz32: %d %x\n", BPB_FATSz32, BPB_FATSz32);

}

void readDirectory(int clusAddress) {
  /* This function reads a directory given the cluster address. It assumes that there are only going to be 16 entries */
  if (!clusAddress) return;
  // reads directory adding it to dir struct array
  fseek( fp, clusAddress, SEEK_SET);
  int i;
  for (i = 0; i < NUMBER_OF_ENTRIES; i++) {
    fread(&dir[i], sizeof(struct DirectoryEntry), 1, fp);
  }
}

void printFilesInDir() {
  //prints the names of the files in the dir struct arr
  int i;
  for (i = 0; i < NUMBER_OF_ENTRIES; i++) {
    char name[12];
    memcpy(name , dir[i].DIR_Name, 11);
    name[11] = '\0';

    if (name[0] == (char)0xE5 || name[0] == (char)0x05 || name[0] == (char)0x00 ||dir[i].DIR_FirstClusterLow == 0)
      continue;

    uint8_t attribute = dir[i].Dir_Attr;

    if (attribute != 0x01 && attribute != 0x10 && attribute != 0x20)
      continue;

    printf("%s\n", name);

  }
}

int LBAToOffset(int32_t sector) {
  return ((sector -2) * BPB_BytesPerSec) + (BPB_BytesPerSec * BPB_RsvdSecCnt) + (BPB_NumFATs * BPB_FATSz32 * BPB_BytesPerSec);
}

void makeUpperCase(char *word) {
  /*Makes any string into upper case*/
  int i = 0;
  while(word[i]) {
      word[i] = toupper( word[i] );
      i++;
   }
}

void makeLowerCase(char *word) {
  /*Makes any string into lower case*/
  int i = 0;
  while(word[i]) {
      word[i] = tolower( word[i] );
      i++;
   }
}

char* makeIntoDirName(char *fileName) {
  /*This function takes the string given by user and farmats it to look like the ones in the fat32 img*/
  int i, j, fileNameLength = strlen(fileName);

  // decides if it's a directory or file by checking whether it has a .
  int isDirectory = fileName[fileNameLength - 4] == '.' ? 0 : 1;
  makeUpperCase(fileName);

  char *dirName = (char*) malloc( LENTH_OF_DIR_NAME * sizeof(char) );

  if (isDirectory) {
    for (i = 0; i < LENTH_OF_DIR_NAME; i++) {
      if (i >= fileNameLength) {
        dirName[i] = ' ';
      }
      else {
        dirName[i] = fileName[i];
      }

    }
    return dirName;
  }

  else {
    char *dirName = (char*) malloc( LENTH_OF_DIR_NAME * sizeof(char) );
    i = 0;
    while (fileName[i] != '.') {
      dirName[i] = fileName[i];
      i++;
    }

    while (i < LENTH_OF_DIR_NAME - 3) {
      dirName[i] = ' ';
      i++;
    }

    dirName[LENTH_OF_DIR_NAME - 3] = fileName[fileNameLength - 3];
    dirName[LENTH_OF_DIR_NAME - 2] = fileName[fileNameLength - 2];
    dirName[LENTH_OF_DIR_NAME - 1] = fileName[fileNameLength - 1];

    return dirName;
  }
}

void makeIntoProperName(char *fileName) {
/*Makes directory name or file name in the fat32 image into proper format that can be used ouside of the image*/
  makeLowerCase(fileName);

  int lengthOfFileName = strlen(fileName);
  char extention[3];
  extention[0] = fileName[lengthOfFileName - 3];
  extention[1] = fileName[lengthOfFileName - 2];
  extention[2] = fileName[lengthOfFileName - 1];

  char *token = strtok(fileName, " ");

  char *fixedFileName = (char*) malloc( ( strlen(token) + 4) * sizeof(char) );

  int i;
  for (i = 0; i < strlen(token); i++) {
    fixedFileName[i] = token[i];
  }
  fixedFileName[i] = '.';
  fixedFileName[i + 1] = extention[0];
  fixedFileName[i + 2] = extention[1];
  fixedFileName[i + 3] = extention[2];

  strcpy(fileName, fixedFileName);
}

void openFile(char *fileName) {
  // if the file is already open
  if (fp && strcmp(fileName, currentlyOpenFileName) == 0) {
    printf("Error: File system image already open.\n");
  }
  else {
    // deletes previous file name if new one
    if (currentlyOpenFileName) free(currentlyOpenFileName);

    fp = fopen( fileName, "r");

    if (fp == NULL) {
      perror("Error opening file");
    }
    else {
      currentlyOpenFileName = (char*) malloc( strlen(fileName) * sizeof(char) );
      strcpy(currentlyOpenFileName, fileName);
      getInfo();
    }
  }

  return;
}

void closeFile() {
  /*Closes file and set file pointer to null that way no other operation can take place*/
  if (fp) {
    fclose(fp);
    free(currentlyOpenFileName);
    fp = NULL;
  }
  else {
    printf("Error: File system image must be opened first.\n");
  }
}

int findIndexOfFile(int clusAddress, char *fileName) {
  /* finds file given clusAddress or returns NULL if file or directory does not exist in that cluster */
  if (!clusAddress) return -1;

  readDirectory(clusAddress);

  fileName = makeIntoDirName(fileName);

  int i;
  for (i = 0; i < NUMBER_OF_ENTRIES; i++) {
    char dirName[12];
    memcpy(dirName , dir[i].DIR_Name, 11);
    dirName[11] = '\0';

    if (strcmp(fileName, dirName) == 0) {
      return i;
    }
  }

  return -1;
}

void printStat(char *fileName) {
  /*prints info about a file or directory in the current cluster address given the file name*/
  if (!fp) return;

  int index = findIndexOfFile(currentClusAddress, fileName);
  if (index != -1) {
    printf("Attribute: %d\n", dir[index].Dir_Attr);
    printf("Size: %d\n", dir[index].DIR_FileSize);
    printf("Starting Cluster Number: %d\n", dir[index].DIR_FirstClusterLow);
  }
  else printf("Error: File not found\n");
}

void getFile(char *fileName) {
  /*Finds file in current cluster address then copies it's character content to the directory outside of the fat32 img*/
  if (!fp) return;

  int fileNameLength = strlen(fileName);
  int isDirectory = fileName[fileNameLength - 4] == '.' ? 0 : 1;

  if (isDirectory) {
    printf("Error: Cannot get directory\n");
    return;
  }

  int fileIndex = findIndexOfFile(currentClusAddress, fileName);

  if (fileIndex == -1) {
    printf("Error: File not found\n");
    return;
  }

  int lowClusterNumber = dir[fileIndex].DIR_FirstClusterLow;
  int fileSize = dir[fileIndex].DIR_FileSize;

  char *newFileName = (char*) malloc( sizeof(char) * 12);
  memcpy(newFileName , dir[fileIndex].DIR_Name, 11);
  newFileName[11] = '\0';

  makeIntoProperName(newFileName);

  int offset = LBAToOffset(lowClusterNumber);

  fseek(fp, offset, SEEK_SET);
  FILE *newFp = fopen(newFileName, "w+");

  char buffer[512];

  while (fileSize >= 512) {

    fread(buffer, 512, 1, fp);
    fwrite(buffer, 512, 1, newFp);
    fileSize -= 512;

    lowClusterNumber = nextLB(lowClusterNumber);

    if (lowClusterNumber == -1) break;

    offset = LBAToOffset(lowClusterNumber);
    fseek(fp, offset, SEEK_SET);

  }

  // if there is a remainder
  if (fileSize > 0) {
    fread(buffer, fileSize, 1, fp);
    fwrite(buffer, fileSize, 1, newFp);
  }

  fclose(newFp);

}

void readFile(char *fileName, char* position, char* numberOfBytes) {
  /*Reads the hex content of the file given the offset, file name and number of bytes*/
  if (!fp) return;

  int fileIndex = findIndexOfFile(currentClusAddress, fileName);

  if (fileIndex == -1) {
    printf("Error: File not found\n");
    return;
  }

  int postionInt = atoi(position);
  int numberOfBytesInt = atoi(numberOfBytes);

  int lowClusterNumber = dir[fileIndex].DIR_FirstClusterLow;
  int fileSize = dir[fileIndex].DIR_FileSize;

  int offset = LBAToOffset(lowClusterNumber);

  fseek(fp, offset, SEEK_SET);

  uint8_t buffer[postionInt + numberOfBytesInt];

  fread(&buffer, postionInt + numberOfBytesInt, 1, fp);

  int i;

  for (i = postionInt; i < postionInt + numberOfBytesInt; i++) {
    printf("%x ", buffer[i]);
  }
  printf("\n");

}

void listFiles() {
    if (!fp) return;
    readDirectory(currentClusAddress);
    printFilesInDir();
}

void changeDirectory(char *directoryName) {
  /*changes to directory in current cluster address if the directory exists.
    When going back, it assumes that the parent of the directory is the entry in position 1 of the dir array*/
  if (!fp) return;

  if (strcmp(directoryName, "..") == 0) {

    if (currentClusAddress == RootClusAddress) {
      return;
    }

    readDirectory(currentClusAddress);
    int parentClusterLow = dir[1].DIR_FirstClusterLow;

    if (parentClusterLow == 0) {
      currentClusAddress = RootClusAddress;
    }
    else {
      currentClusAddress = LBAToOffset(parentClusterLow);
    }

    return;

  }

  int directoryIndex = findIndexOfFile(currentClusAddress, directoryName);

  if (directoryIndex == -1) {
    printf("Error: Directory not found\n");
    return;
  }

  int lowClusterNumber = dir[directoryIndex].DIR_FirstClusterLow;

  currentClusAddress = LBAToOffset(lowClusterNumber);
}

void printVolumeName() {
  /*prints the volume label of the fat32 image using offset, size and array from getInfo() and beginning declarations*/
  if (!fp) return;

  if (Volume_Name[0])
    printf("Volume Name is '%s'\n", Volume_Name);
  else
    printf("Error: volume name not found.\n");

}

int main() {

  char * cmd_str = (char*) malloc( MAX_COMMAND_SIZE );

  while( 1 )
  {
    // Print out the msh prompt
    printf ("mfs> ");

    // Read the command from the commandline.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something since fgets returns NULL when there
    // is no input
    while( !fgets (cmd_str, MAX_COMMAND_SIZE, stdin) );
    /* Parse input */
    char *token[MAX_NUM_ARGUMENTS];

    int   token_count = 0;

    // Pointer to point to the token
    // parsed by strsep
    char *arg_ptr;

    char *working_str  = strdup( cmd_str );

    // we are going to move the working_str pointer so
    // keep track of its original value so we can deallocate
    // the correct amount at the end
    char *working_root = working_str;

    // Tokenize the input stringswith whitespace used as the delimiter
    while ( ( (arg_ptr = strsep(&working_str, WHITESPACE ) ) != NULL) &&
              (token_count<MAX_NUM_ARGUMENTS)) {

      token[token_count] = strndup( arg_ptr, MAX_COMMAND_SIZE );
      if( strlen( token[token_count] ) == 0 ) {
        token[token_count] = NULL;
      }
        token_count++;
    }

    if (token[0]) {
      //checks for open command
      if ( strcmp(token[0], "open") == 0 ) {
        //checks if there's a file name then calls the openFile function
        if (token[1]) openFile(token[1]);
      }

      //checks for close command
      else if ( strcmp(token[0], "close") == 0 ) closeFile();

      //checks for info command
      else if (strcmp(token[0], "info") == 0) printInfo();

      //checks for stat command
      else if (strcmp(token[0], "stat") == 0){
        //checks if there's a file name first then calls the printStat function
          if (token[1]) printStat(token[1]);
      }

      //checks for get command
      else if (strcmp(token[0], "get") == 0) {
        //checks if there's a file name first then calls the getFile function
        if (token[1]) getFile(token[1]);
      }

      //checks for read command
      else if (strcmp(token[0], "read") == 0) {
        //checks if there's a file name first then calls the readFile function
        if (token[1] && token[2] && token[3]) readFile(token[1], token[2], token[3]);
      }
      //checks for ls command
      else if (strcmp(token[0], "ls") == 0) listFiles();

      //checks for change directory command
      else if (strcmp(token[0], "cd") == 0) {
        //checks if there's a file name first then calls the listFiles function
        if (token[1]) changeDirectory(token[1]);
      }

      //checks volume command
      else if (strcmp(token[0], "volume") == 0) printVolumeName();

      //checks exist command
      else if (strcmp(token[0], "exit") == 0 || strcmp(token[0], "quit") == 0) break;

    }

  }

  return 0;
}
