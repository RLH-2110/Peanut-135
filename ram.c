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
#include "peanut.h"
#include "util.h"

#define SAVE_FILE_MAX_LENGTH 50 
#define SAVE_LOCATION_FROM_HOME "/gbsaves/"
#define SAVE_FILE_EXTENSION ".sav"
#define SAVE_FILE_USABLE_LENGH SAVE_FILE_MAX_LENGTH - strlen(SAVE_FILE_EXTENSION) - strlen(SAVE_LOCATION_FROM_HOME) - strlen(homeDir) - 1

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

/* loads ram file, if one exist, otherwhise loads Cartride ram with 0. returns true on success, false on error
  uses global state: cartRamData, cartRamSize
  romName: name of the ROM file (without the path)
  cartRamData, will get set to a pointer to the allocated memory for the RAM (set to NULL if function is unsuccesfull)
  cartRamSize, will get set to the size of the RAM.

  returns: true if the RAM was set up correctly, false if it failed.
*/
bool initalize_cart_ram(struct gb_s *gameboy, const char *const romName){

  /*initalize in case of early return*/
  cartRamData = NULL;

  cartRamSize = gb_get_save_size(gameboy);

  if (cartRamSize == 0)
    return true; /* no cartrige ram needed */


  if (romName == NULL)
    return false;

  char *homeDir = getenv("HOME");
  if (homeDir == NULL){
    puts("Error: $HOME is not set! Savefiles can not be read!");
    goto file_does_not_exit;
  }

  
  if (SAVE_FILE_USABLE_LENGH <= 3){
    printf("please increase the size of SAVE_FILE_MAX_LENGTH in RAM.H!\n\t Curent useable size: %ld\n",SAVE_FILE_USABLE_LENGH);  
    puts("save file can not be read!");
    goto file_does_not_exit;
  }

  char saveFileName[SAVE_FILE_MAX_LENGTH]; 
  
  size_t copyLength = strlen(romName);
  if (copyLength > SAVE_FILE_USABLE_LENGH) 
    copyLength = SAVE_FILE_USABLE_LENGH;
    
  strcpy(saveFileName                                                       ,homeDir);                          /* copy home directory */
  strcpy(saveFileName+strlen(homeDir)                                       ,SAVE_LOCATION_FROM_HOME);          /* copy save folder path relative from home */
  memcpy(saveFileName+strlen(SAVE_LOCATION_FROM_HOME) ,romName              ,copyLength);                       /* copy file name */
  memcpy(saveFileName+copyLength                      ,SAVE_FILE_EXTENSION  ,strlen(SAVE_FILE_EXTENSION) + 1);  /* add SAVE_FILE_EXTENSION and \0 */


  bool fileExists;
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

      NOT_IMPLEMENTED return false;

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
    
    int saveFile = open(saveFileName,O_RDONLY);
    if (saveFile < 0){
      if (choice("Error: could not open Savefile, want to continue anyway? (Y/N)")
      == false)
        return false;

      memset(cartRamData,0, cartRamSize);
      return true;
    }

    int readSize;

    if (saveFileSize <= cartRamSize)
      readSize = read(saveFile,cartRamData,saveFileSize);   
    else
      readSize = read(saveFile,cartRamData,cartRamSize);

    if (readSize < cartRamSize && readSize < saveFileSize){
      if (readSize == -1)
        perror("Error: Failed to read file! errno:");
      else
        puts("Error: Failed to read entire file!");

      free(cartRamData); cartRamData = NULL;
      return false;
    }

    if (cartRamSize > saveFileSize)
      memset(cartRamData+saveFileSize,0, cartRamSize - saveFileSize);

    if (close(saveFile) != 0)
      perror("Error: could not close Savefile! errno");
 
    return true;
  } 

file_does_not_exit:
  /* file does not exist or we got an error while opening it */
  
  cartRamData = calloc(cartRamSize,sizeof(uint8_t));
  if (cartRamData == NULL){
    puts("ERROR: Not enough memory for the cartrige RAM!");
    return false;
  }

  return true;
}


