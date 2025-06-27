#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdint.h>

#include "rom.h"
#include "peanut.h"
#include "util.h"
#include "main.h"


/*Returns a byte from the ROM file at the given address
  gb: unused
  addr: address to get data from
  returns: byte at address in ROM
*/
uint8_t gb_rom_read(struct gb_s *gb, const uint_fast32_t addr)
{
  if (romData == NULL){
    puts("Warning: tried to read from ROM, while ROM was not mapped!");
    return 0x00;
  }

  if (addr >= romSize){
    puts("Warning: Tried to read outside of ROM mapping!");
    return 0x00;
  }

  return romData[addr];
}


/* unmaps the ROM
  uses global state: romData, romSize
  romData: pointer to the ROM we want to unmap
  romSize: size of the rom to unmap
  returns: 0 if succes. other values on failure. sets Errno on failure. (currently uses munmap)
 */
int unmap_rom(){
  if (romData != NULL){
    return munmap(romData,romSize); romData = NULL; romSize = 0;
  }
}

/* loads rom file, returns true on success, false on error 
  uses global state: romData, romSize
  path: path to the rom
  romData, will get set to a pointer to the memory mapped area of the ROM (set to NULL if mmap was unsuccesfull or did not happen)
  romSize, will get set to the size of the ROM. (set to 0 if function failed before getting the file size)

  returns: true if the ROM was mapped correctly, false if it failed.
*/
bool load_rom_file(const char * const path){

  romData = NULL; /* default value, in case of early return */

  /* check filesize */
  struct stat statStruct;
  if ( stat(path, &statStruct) != 0){
    fputs("Error: ",stdout);

    switch(errno){
      case EACCES:
        printf("Could not access %s\n", path); break;
      case ENAMETOOLONG:
        printf("Could not access %s, path is too long\n", path); break;
      case ENOTDIR:
      case EBADF:
      case ENOENT:
        printf("%s does not exist\n", path); break;
      case EOVERFLOW:
        printf("%s is too big\n", path); break;
      case ELOOP:
        puts("Too many symbolic links"); break;
      default:
        printf("Could not read information about %s\n",path); break;
    }  
    romSize = 0;
    return false;
  }

  romSize = statStruct.st_size;
  
  /* check file size */
  if (statStruct.st_size == 0){
    puts("Error: ROM file is empty!");
    return false;
  }

  if (BYTES_TO_MEGABYTES(statStruct.st_size) > 32){

    if( choice("Warning: The ROM is rather large for GameBoy ROMS, are you sure you want to load the %ld MiB File? (Y/N)",BYTES_TO_MEGABYTES(statStruct.st_size)) 
    == false)
      return false;
    
  } 

  /* open and map file */
  int fd = open(path, O_RDONLY);
  if (fd < 0){
    if (errno == EACCES)
      printf("Error: Read Permission denied for %s\n",path);
    else
      printf("error while trying open %s\n",path);
    return false;
  }
  LOGR("ALLOC: ROMFILE",1);

  romData = mmap(NULL, statStruct.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  
  if (romData == MAP_FAILED){
    romData = NULL;

    fputs("Error: ",stdout);

    switch(errno){
      case EACCES:
        puts("Access error when mapping ROM!"); break;
      case ENOMEM:
        puts("Not enough memory!"); break;
      case ENODEV:
        puts("Memory maping not supported by file system"); break;
      default:
        perror("Memory map failed! mmap errno");
    }

    return false;
  }

  LOGR("ALLOC: ROMDATA",1);

  if (close(fd) != 0){
    perror("Error: close error after mmaping memory! close errno");
    /* does not return on error here, since mmap was still successfull */
  }else
    LOGR("CLEAN: ROMFILE",-1);

  return true;
}



