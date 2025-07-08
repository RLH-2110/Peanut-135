#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>

#include "ram.h"
#include "forktest/test_ram.h"
#include "peanut.h"
#include "util.h"
#include "main.h"

#define SAVE_FILE_MAX_LENGTH 100 
#define SAVE_FILE_EXTENSION ".sav"
#define SAVE_FILE_USABLE_LENGH SAVE_FILE_MAX_LENGTH - strlen(SAVE_FILE_EXTENSION) - 1

char saveFileName[SAVE_FILE_MAX_LENGTH]; 

#define FS_RETIES 500 /* determins how often we try again if a read or write writes less bytes than required */

/* for tests */
bool testCartridgeRamOverwriteOn = false;
int testCartridgeRamOverwrite = 0;
bool testNeverLoadFromFileOverwrite = false;



/*writes a byte to the RAM at the given address
  gb: unused
  addr: address to write data to
  returns: byte at address in RAM
*/
void gb_cart_ram_write(struct gb_s* gameboy, const uint_fast32_t addr, const uint8_t data){
  if (cartRamData == NULL)
    return;
  if (addr >= cartRamSize)
    return;

  cartRamData[addr] = data;
}
/*Returns a byte from the RAM at the given address
  gb: unused
  addr: address to get data from
  returns: byte at address in RAM
*/
uint8_t gb_cart_ram_read(struct gb_s* gameboy, const uint_fast32_t addr){
  if (cartRamData == NULL)
    return 0;
  if (addr >= cartRamSize)
    return 0;

  return cartRamData[addr];
}

char* get_save_name(const char *const romName){
  
  char* fullPath = realpath(romName,NULL);
  LOGR("Alloc: fullPath",1);
  if (fullPath == NULL){
    perror("Error: Could not get path of ROM. Errno");
    return NULL;
  }

  size_t romNameLen = strlen(fullPath);
  size_t fileExtOffset = romNameLen;

  size_t copyLength = romNameLen;
  
  /* find last . */
  for (size_t i = romNameLen - 1; i > 0;i--){
    if (fullPath[i] == '.'){
      fileExtOffset = i;
      break;    
    }
  }

  /* exlude text after last . */
  if (copyLength > fileExtOffset)
    copyLength = fileExtOffset;

  /* bounds check */
  if (copyLength > SAVE_FILE_USABLE_LENGH){
    puts("Error: File Path too long!");
    free(fullPath); fullPath == NULL;
    LOGR("Clean: fullPath",-1);
    return NULL;   
  }

  memcpy(saveFileName               ,fullPath             ,copyLength);                       /* copy file name */
  memcpy(saveFileName+copyLength    ,SAVE_FILE_EXTENSION  ,strlen(SAVE_FILE_EXTENSION) + 1);  /* add SAVE_FILE_EXTENSION and \0 */

  free(fullPath); fullPath == NULL;
  LOGR("Clean: fullPath",-1);
  return saveFileName;
}

/* loads ram file, if one exist, otherwhise loads Cartride ram with 0. returns true on success, false on error
  uses global state: cartRamData, cartRamSize
  romName: path to the ROM file
  cartRamData, will get set to a pointer to the allocated memory for the RAM (set to NULL if function is unsuccesfull)
  cartRamSize, will get set to the size of the RAM.

  returns: true if the RAM was set up correctly, false if it failed.
*/
bool initalize_cart_ram(struct gb_s *gameboy, const char *const romName){

  /*initalize in case of early return*/
  cartRamData = NULL;

  if (testCartridgeRamOverwriteOn == false){
    if (gameboy == NULL){
      puts("Error: gameboy can not be NULL in initalize_cart_ram");
      return false;
    }

    if (gb_get_save_size_s(gameboy, &cartRamSize) != 0){
      puts("Error: could not get cartridge ram size!");
      cartRamSize = 0; 
    }

  }else
    cartRamSize = testCartridgeRamOverwrite;

  if (cartRamSize == 0)
    return true; /* no cartrige ram needed */

  if (romName == NULL){
    puts("Error: Please provie the romName in initalize_cart_ram, to get the save file name");
    return false;
  }

  if (testNeverLoadFromFileOverwrite)
    goto file_does_not_exit;

  if (SAVE_FILE_USABLE_LENGH <= 3){
    printf("please increase the size of SAVE_FILE_MAX_LENGTH in RAM.H!\n\t Curent useable size: %ld\n",SAVE_FILE_USABLE_LENGH);  
    puts("save file can not be read!");
    goto file_does_not_exit;
  }

  if (get_save_name(romName) == NULL){
    printf("No save file found at %s\n",saveFileName);
    goto file_does_not_exit;
  }

  bool fileExists = true;
  size_t saveFileSize = 0;

  /* check filesize, and if file exists  */
  struct stat statStruct;
  if ( stat(saveFileName, &statStruct) != 0){
    fputs("Error: ",stdout);

    if (errno == EISDIR) { /* file is a directory */
      puts("Please do not name a folder after a potential save file in the save directory :(");
      return false;
    }
    if (errno == ENOENT || errno == ENOTDIR) /* file not found, or path to file is broken*/
      fileExists = false;
    else    
      fileExists = true; /* assume it exist, but we got another error */
  }

  if (fileExists){
    saveFileSize = statStruct.st_size;

    /* check if file size does not match */
    if (cartRamSize > saveFileSize){
      if (choice("Error: The Savefile is smaller than the needed cartrige RAM, want to pad the data to %ld Byte (Y/N)",cartRamSize)
      == false)
        return false;
    } 
    if (cartRamSize < saveFileSize){
      if (choice("Error: The Savefile is bigger than the needed cartrige RAM, want to trim the data to %ld Byte (Y/N)",cartRamSize)
      == false)
        return false;

    } 

    /* check if file is big */
    if (BYTES_TO_MEGABYTES(saveFileSize) > 4){
      if (choice("Warning: The cartrige RAM is rather large for the GameBoy, are you sure you want to load the %ld MiB File? (Y/N)",BYTES_TO_MEGABYTES(saveFileSize))
      == false)
        return false;
    }


    /* read the file, at most cartRamSize bytes */

    cartRamData = calloc(cartRamSize,sizeof(uint8_t));
    if (cartRamData == NULL){
      puts("ERROR: Not enough memory for the cartrige RAM!");
      return false;
    }
    LOGR("ALLOC: RAMDATA",1);
    
    int saveFile = open(saveFileName,O_RDONLY);
    if (saveFile < 0){
      if (choice("Error: could not open Savefile, want to continue anyway? (Y/N)")
      == false)
        return false;

      memset(cartRamData,0, cartRamSize);
      return true;
    }
    LOGR("ALLOC: SAVEFILE",1);

    size_t readSize = 0; 
    size_t maxRead;
    int tries = FS_RETIES;

    if (saveFileSize <= cartRamSize)
      maxRead = saveFileSize; 
    else
      maxRead = cartRamSize; 

    while(readSize < maxRead && tries > 0 && readSize != -1){
      readSize += read(saveFile,cartRamData+readSize,maxRead-readSize);
      tries--;
    }

    if (readSize < cartRamSize && readSize < saveFileSize){
      if (readSize == -1)
        perror("Error: Failed to read file! errno:");
      else
        puts("Error: Failed to read entire file!");

      free(cartRamData); cartRamData = NULL;
      LOGR("CLEAN: RAMDATA",-1);
      goto file_does_not_exit;
    }

    if (cartRamSize > saveFileSize)
      memset(cartRamData+saveFileSize,0, cartRamSize - saveFileSize);

    if (close(saveFile) != 0)
      perror("Error: could not close Savefile! errno");
    else
      LOGR("CLEAN: SAVEFILE",-1);
 
    if (readSize < cartRamSize){
      printf("Error: could not read file!\n\tRead %d out of %d bytes!",readSize,cartRamSize);
 
      free(cartRamData); cartRamData = NULL;
      LOGR("CLEAN: RAMDATA",-1);
      goto file_does_not_exit;
    }
    return true;
  } 

file_does_not_exit:
  /* file does not exist or we got an error while opening it */
  
  cartRamData = calloc(cartRamSize,sizeof(uint8_t));
  if (cartRamData == NULL){
    puts("ERROR: Not enough memory for the cartrige RAM!");
    return false;
  }
  LOGR("ALLOC: RAMDATA",1);

  return true;
}



/* --------------------------- */


bool save_cart_ram(struct gb_s *gameboy, const char *const romName){

  if (cartRamSize == 0)
    return true; /* no cartrige ram, so no save file */

  if (romName == NULL){
    puts("Error: Please provie the romName in initalize_cart_ram, to get the save file name");
    return false;
  }

  char *homeDir = getenv("HOME");
  if (homeDir == NULL){
    puts("Error: $HOME is not set! Savefiles can not be read!");
    return false;    
  }

  
  if (SAVE_FILE_USABLE_LENGH <= 3){
    printf("please increase the size of SAVE_FILE_MAX_LENGTH in RAM.H!\n\t Curent useable size: %ld\n",SAVE_FILE_USABLE_LENGH);  
    puts("save file can not be written!");
    return false;
  }


  if (get_save_name(romName) == NULL)
    return false;

   
  int saveFile = open(saveFileName,O_WRONLY | O_TRUNC | O_CREAT, 0x600 );
  if (saveFile < 0){
    puts("Error: Could not open save file!");
    return false;
  }
  LOGR("ALLOC: SAVEFILE",1);

  size_t written = 0;
  int tries = FS_RETIES;
  
  while(written < cartRamSize && tries > 0 && written != -1){
    written += write(saveFile,(uint8_t*)(cartRamData + written), cartRamSize-written);
    tries--;
  }

  if (close(saveFile) != 0)
    perror("Error: could not close Savefile! errno");
  else
    LOGR("CLEAN: SAVEFILE",-1);

  if (written == -1){
    perror("Error: Could not write file! Errno");
    return false;
  }

  if (written < cartRamSize){
    printf("Error: could not save file!\n\tSaved %d out of %d bytes!",written,cartRamSize);
    return false;
  }

  return true;
}


