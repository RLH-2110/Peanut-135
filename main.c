
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
#include <sys/stat.h>
#include <ctype.h>
/* peanut */
#define PEANUT_GB_IS_LITTLE_ENDIAN 1

#include "headers/peanut_gb.h"

/* inilib */
#include "iniparser/src/iniparser.h"

/* own includes */
#include "headers/rom.h"
#include "headers/ram.h"
#include "headers/lcd.h"
#include "headers/drm.h"
#include "headers/input.h"
#include "headers/main.h"
#include "headers/util.h"
#include "headers/blockmnt.h"

#include "westonkill.c"

#if FULL_CONTROLL >= 1
#include "device_tree.c"
#endif

/* GLOAL VARIBLES, EVEN OUTSID THIS FILE! */
    /* from rom.h */
uint8_t *romData = NULL;
size_t romSize;

    /* from ram.h */
uint8_t *cartRamData;
size_t cartRamSize;

   /* from main.h */
int resource_count = 0;
volatile sig_atomic_t romSelect = 0;
char* showLvglErrorMsg = NULL; /* if not null, lvgl will show this string after setup and reset it to NULL */

   /* from drm.h*/
display_mode_t displayMode;

   /* from blockmnt.h */
char *roms = NULL;
size_t romsIndex = 0;
size_t romsSize = 0;

/* note: there is a thread safe varaible `stop` in drm.h thats global. if set to 1, the program will eventually terminate */



/* Other variables, only in main.c */

char* romFile = NULL;

#define MAX_NAME 80 
char customSearchPath[MAX_NAME] = { '\0' };
bool searchHome;
bool searchExternal;
bool autoLoad;

bool reinitGameboy = false;

volatile sig_atomic_t signalStop = 0;

void cleanup_and_exit(int exitCode);
void soft_cleanup(void); /* for cleanup up the emulator, so we can restart it*/
void gb_error(struct gb_s *gameboy, const enum gb_error_e gbError, const uint16_t addr);

/* loads the config file using load_conf_vals (defined later)
  useSystem: if false, loads the user conf, and calls itself with this as true on failue
             if true, loads system wide conf, and uses default values on error
*/
void load_conf(bool useSystem); 


/* signal handler */
void on_termination(int signal){
  
  static int i = 1;
  
  
  if (i >= 3){ /* user wants to kill us, then lets die */
    LOGR("RESOURCE LOG: GOT TERMINATION SIGNAL THREE TIMES! HONOURING THE REQUEST FOR DEATH...",0);
    exit(EXIT_FAILURE); /* failure, because we could not clean up */
  }
    

  LOGR("RESOURCE LOG: GOT TERMINATION SIGNAL!",0);
  signalStop = 1;
  stop = 1;
  i++;
}

/* macro, so we call the right function in each mode we are in */
#if FULL_CONTROLL == 0
#define MAIN_CLEANUP(status) cleanup_and_exit(status)
#else
#define MAIN_CLEANUP(status) { soft_cleanup(); goto main_restart_point; }
#endif

#include "lvgl.c"

#if RLH_TEST == 0
int main(int argc, char **argv){  /* } (for vim matching these: {} */
#else
int run_main(int argc, char **argv){
#endif
    
  struct gb_s gameboy = { 0 };

  signal(SIGINT,on_termination);
  signal(SIGTERM,on_termination);
  
#if FULL_CONTROLL >= 1
  force_device_tree();
  if (stop == 1)
    return 0;
#endif

  /* the first few cant use MAIN_CLEANUP, since the first setup stage must be successfull for it to work */

  load_conf(false);
  if( search_roms(customSearchPath,searchExternal,searchHome) == false || roms == NULL)
    cleanup_and_exit(EXIT_FAILURE);

  if (kill_weston() == false)
    cleanup_and_exit(EXIT_FAILURE); 

  if (setup_drm() == false)
    cleanup_and_exit(EXIT_FAILURE);

  puts("first setup stage was sucessfull!");

  /* get rom from either arguments, autoLoad or lvgl */
  if (argc >= 2) {
    romFile = argv[1];
  }else{
    if (autoLoad == false){
      lvgl_main();
    }else{
      if (get_roms_count() == 1)
        romFile = get_first_from_roms();
      else{
main_restart_point:
        lvgl_main();
      }
    }
  }

#if FULL_CONTROLL >= 1
  if ((void*)romFile == (void*)ADDR_SHUTDOWN){ /* some ugly hack so we can shut down when FULL_CONTROL is on */
    puts("shutdown requested");
    system("shutdown now");
    cleanup_and_exit(EXIT_SUCCESS);
  }
#endif

  printf("Selected rom file: %s\n",romFile);

  clear_screen();

  if (romFile == NULL || *romFile == '\0')
    MAIN_CLEANUP(EXIT_SUCCESS); /* this can only happen if the user exited lvgl without selecting a ROM, so the user did not select a ROM on purpose, and we can exit */

  if (load_rom_file(romFile) == false){
    showLvglErrorMsg = "Could not load ROM!";
    MAIN_CLEANUP(EXIT_FAILURE);
  }

  if (init_input() == false)
    cleanup_and_exit(EXIT_FAILURE); /* hard exit, because we need input */

  struct tm* localTime = NULL; 
  time_t currentTime = time(NULL);
  localTime = localtime(&currentTime);
  if (currentTime == 0)
    puts("could not get time! using the epoch time");


  memset(&gameboy, 0, sizeof(gameboy)); /* clean up the stuct. is usefull for when we restart the gameboy with a new ROM */
  int gbInitErr;
  gbInitErr = gb_init(&gameboy, gb_rom_read, gb_cart_ram_read, gb_cart_ram_write, gb_error, NULL);

  if (gbInitErr != 0){
    fputs("Init Error: ",stdout);

    switch(gbInitErr){
      case GB_INIT_CARTRIDGE_UNSUPPORTED:
        puts("Cartdrige unsupported!"); 
        showLvglErrorMsg = "Cartridge unsupported!";
        MAIN_CLEANUP(EXIT_FAILURE);
      case GB_INIT_INVALID_CHECKSUM:
        puts("Cartdrige checksum fail!");
        showLvglErrorMsg = "Cartridge checksum fail!";
        MAIN_CLEANUP(EXIT_FAILURE);
      default:
        puts("Unkown error!");
        showLvglErrorMsg = "Unkown initialisation error";
        MAIN_CLEANUP(EXIT_FAILURE);
    }    
  }



  gb_init_lcd(&gameboy,lcd_draw_line);

  if (localTime != NULL)
    gb_set_rtc(&gameboy, localTime);
  else
    puts("could not get local time!");

  if(initalize_cart_ram(&gameboy, romFile) == false)  
    cleanup_and_exit(EXIT_FAILURE);

  gameboy.direct.joypad = 0xFF;

  while(!stop && !romSelect){
    gb_run_frame(&gameboy);
    display_frame(); 

    /* handle input */
    gameboy.direct.joypad = get_input();
 }

  if (cartRamSize == 0){
    puts("No Savefile created");
  }else{
    if (save_cart_ram(&gameboy, romFile) == true){
      printf("Saved game to %s\n",get_save_name(romFile));
    }
  }

#if FULL_CONTROLL
  if (signalStop == 0){
    soft_cleanup();
    goto main_restart_point; /* got to file menu and wait for new roms */
  }

#endif
  cleanup_and_exit(EXIT_SUCCESS);
}



void cleanup_and_exit(int exitCode){

  puts("exit requested");
  stop = 1;

  cleanup_drm();

  if (roms != NULL){
    free(roms); roms = NULL; romsSize = 0;
    LOGR("Clean: ROMS",-1);
  }

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

/* for cleanup up the emulator, so we can restart it*/
void soft_cleanup(void){

  if (signalStop) /* if we actually want to exit */
    cleanup_and_exit(EXIT_SUCCESS);

  romSelect = 1;
  reinitGameboy = true;

  if (roms != NULL){
    free(roms); roms = NULL; romsSize = 0;
    LOGR("Clean: ROMS",-1);
  }

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

  romSelect = 0; 
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






/* other functions */

/* used by load_conf and load_sys_config to load the config cals with rini */
void load_conf_vals( dictionary *ini ){

  char const *ret = iniparser_getstring(ini, "launch:customSearchPath", "");
  if (strlen(ret) > MAX_NAME - 1)
    puts("Error: launch:customSearchPath is too long! Using Default value");
  else
    strcpy(customSearchPath,ret);

  searchHome = iniparser_getboolean(ini, "launch:searchHome", true);
  searchExternal = iniparser_getboolean(ini, "launch:searchExternal", true);
  autoLoad = iniparser_getboolean(ini, "launch:autoLoad", false);
  

  char const *dispModeReadOnly = iniparser_getstring(ini,"display:mode","default"); 
  char *dispMode = malloc(strlen(dispModeReadOnly)+1);
  if (dispMode == NULL){
    puts("Error: Out of memory when reading display:mode! Using default value.");
    displayMode = display_mode_default;
    return;
  }
  LOGR("ALLOC: DISPMODE",1);
  strcpy(dispMode,dispModeReadOnly);

  for(int i = 0; dispMode[i] != '\0'; i++)
    dispMode[i] = toupper(dispMode[i]);

  if (strcmp(dispMode, "DEFAULT") == 0)
    displayMode = display_mode_default;
  else
  if (strcmp(dispMode, "WIDE")    == 0)
    displayMode = display_mode_wide;
  else
  if (strcmp(dispMode, "FULL_Y")  == 0)
    displayMode = display_mode_full_y;
  else
  if (strcmp(dispMode, "CUT_Y")   == 0)
    displayMode = display_mode_cut_y;
  else{
    printf("Warning: %s is not a valid display mode! using default value...\n",dispModeReadOnly);
    displayMode = display_mode_default;
  }
 
  free(dispMode); dispMode = NULL;
  LOGR("CLEAN: DISPMODE",-1);
}


/* loads the config file using load_conf_vals
  useSystem: if false, loads the user conf, and calls itself with this as true on failue
             if true, loads system wide conf, and uses default values on error
*/
void load_conf(bool useSystem){ 
  
#define MAX_USER_CONF_PATH_SIZE 64
  char *home = getenv("HOME");
  char *usrConfR = "/.config/peanut135/config.ini";
  char usrConf[MAX_USER_CONF_PATH_SIZE] = { '\0' };
  char *sysConf  = "/etc/peanut135/config.ini";

  char *path;

  if (useSystem == false){
  
    if (strlen(home) + strlen(usrConfR) + 1 >= MAX_USER_CONF_PATH_SIZE){
      printf("Warning: can't load local config, File Path is logner than %d bytes!\n",MAX_USER_CONF_PATH_SIZE);
      goto load_conf_error;
    }

    strcpy(usrConf,home);
    strcpy(usrConf + strlen(home), usrConfR);
    
    path = usrConf;
  }else{
    path = sysConf;
  }

  dictionary *ini = iniparser_load(path); 
  if (ini == NULL){
    /* printf("Info: could not load ini file at %s\n",path); already printed by iniparser */
    goto load_conf_error;
  }
  LOGR("ALLOC: INIPARSER",1);

  printf("Info: Loading config file %s...\n",path);
  load_conf_vals(ini);

  iniparser_freedict(ini);
  LOGR("CLEAN: INIPARSER",-1);
  return;

 /* only reachable via goto for cleanup */

load_conf_error:
  if (useSystem == false)
    load_conf(true); /* try again with the system conf, if we dont already have */
  else
    puts("Warning: No config file could be loaded, using default values!");
  return;
}

