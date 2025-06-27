#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include <signal.h>
#include <libgen.h>
#include <fcntl.h>
#include <linux/input.h>

/* peanut */
#define PEANUT_GB_IS_LITTLE_ENDIAN 1

#include "peanut_gb.h"


/* own includes */
#include "rom.h"
#include "ram.h"
#include "lcd.h"
#include "drm.h"
#include "input.h"
#include "main.h"

#include "westonkill.c"

/* GLOAL VARIBLES, EVEN OUTSID THIS FILE! */
    /* from rom.h */
uint8_t *romData = NULL;
size_t romSize;

    /* from ram.h */
uint8_t *cartRamData;
size_t cartRamSize;

   /* from main.h */
int resource_count = 0;

/* note: there is a thread safe varaible `stop` in drm.h thats global. if set to 1, the program will eventually terminate */

/* renames */
#define ROM_PATH argv[1]


void cleanup_and_exit(int exitCode);
void gb_error(struct gb_s *gameboy, const enum gb_error_e gbError, const uint16_t addr);

/* signal handler */
void on_termination(int signal){
  LOGR("RESOURCE LOG: GOT TERMINATION SIGNAL!",0);
  stop = 1;
}


int main(int argc, char **argv){
  if (argc != 2) {
    printf("Error, %s needs only 1 argument!\n%s GB-FILE\n",argv[0],argv[0]);
    return EXIT_FAILURE;
  }
  
  if (init_input() == false)
    return EXIT_FAILURE;

  struct gb_s gameboy;

  signal(SIGINT,on_termination);
  signal(SIGTERM,on_termination);

  if (load_rom_file(ROM_PATH) == false)
    return EXIT_FAILURE;

  if (kill_weston() == false)
    goto exit_early;

  if (setup_drm() == false)
    goto exit_early;



  struct tm* localTime = NULL; 
  time_t currentTime = time(NULL);
  localTime = localtime(&currentTime);
  if (currentTime == 0)
    puts("could not get time! using the epoch time");


  
  int gbInitErr = gb_init(&gameboy, gb_rom_read, gb_cart_ram_read, gb_cart_ram_write, gb_error, NULL);

  if (gbInitErr != 0){
    fputs("Init Error: ",stdout);

    switch(gbInitErr){
      case GB_INIT_CARTRIDGE_UNSUPPORTED:
        puts("Cartdrige unsupported!"); goto exit_cleanup;
      case GB_INIT_INVALID_CHECKSUM:
        puts("Cartdrige checksum fail!"); goto exit_cleanup;
      default:
        puts("Unkown error!"); goto exit_cleanup;
    }    
  }



  gb_init_lcd(&gameboy,lcd_draw_line);

  if (localTime != NULL)
    gb_set_rtc(&gameboy, localTime);
  else
    puts("could not get local time!");

  if(initalize_cart_ram(&gameboy, basename(argv[1])) == false)  
    goto exit_cleanup;
  


  while(!stop){
    gb_run_frame(&gameboy);
    display_frame(); 

    /* handle input */
    gameboy.direct.joypad = get_input();    
 }


exit_cleanup:
  cleanup_drm();
exit_early:
  cleanup_and_exit(EXIT_SUCCESS);
}



void cleanup_and_exit(int exitCode){
  if (romData != NULL){
    if (unmap_rom(romData,romSize) != 0) /* just an munmap, but I can easily switch that if I decide on a different approach */    
      perror("Error: Failed to unmap rom! errno");
    romData = NULL;
    romSize = 0;
    LOGR("CLEAN: ROMDATA",-1);
  }

  if (cartRamData != NULL){
    free(cartRamData); cartRamData = NULL;
    LOGR("CLEAN: RAMDATA",-1);
  }

  cleanup_input();
 
  restart_weston();

  LOGR("PROGRAM TERMINATION...",0);
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
