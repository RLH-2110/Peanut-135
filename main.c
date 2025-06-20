#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>


/* peanut */
#define PEANUT_GB_IS_LITTLE_ENDIAN 1
#define ENABLE_LCD 0

#include "peanut_gb.h"


/* own includes */
#include "rom.h"
#include "ram.h"

/* GLOAL VARIBLES, EVEN OUTSID THIS FILE! */
    /* from rom.h */
uint8_t *romData = NULL;
size_t romSize;

    /* from ram.h */
uint8_t *cartRamData;
size_t cartRamSize;

/* renames */
#define ROM_PATH argv[1]


void cleanup_and_exit(int exitCode);
void gb_error(struct gb_s *gameboy, const enum gb_error_e gbError, const uint16_t addr);

int main(int argc, char **argv){
  if (argc != 2) {
    printf("Error, %s needs only 1 argument!\n%s GB-FILE\n",argv[0],argv[0]);
    return EXIT_FAILURE;
  }

  
  if (load_rom_file(ROM_PATH) == false)
    return EXIT_FAILURE;

  struct gb_s gameboy;

  gb_init(&gameboy, gb_rom_read, gb_cart_ram_read, gb_cart_ram_write, gb_error, NULL);
  
  cleanup_and_exit(EXIT_SUCCESS);
}



void cleanup_and_exit(int exitCode){
  if (romData != NULL){
    if (unmap_rom(romData,romSize) != 0) /* just an munmap, but I can easily switch that if I decide on a different approach */    
      perror("Error: Failed to unmap rom! errno");
    romData = NULL;
    romSize = 0;
  }

  if (cartRamData != NULL){
    free(cartRamData); cartRamData = NULL;
  }

  exit(exitCode);
}


const char const *gbErrorStr[GB_INVALID_MAX] = {
  "Unknown error",
  "Invalid opcode",
  "Invalid read",
  "Invalid write",
  "Infinite halt" /* not sure, but this error code is depricated, so it does not really matter */
};

void gb_error(struct gb_s* gameboy, const enum gb_error_e gbError, const uint16_t addr){

  printf("Error: %s at address %04X\n",gbErrorStr[gbError],addr);  
  cleanup_and_exit(EXIT_FAILURE);

}
