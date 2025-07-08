#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>

#include "../peanut.h"
#include "../main.h"
#include "../drm.h"
#include "../rom.h"
#include "../ram.h"
#include "../input.h"
#include "../lcd.h"


#include "test_ram.c"
#include "test_rom.c"


#if RLH_TEST == 1
int main(int argc, char **argv){ /* } (for vim matching these: {} */
#else
int test_main(int argc, char **argv){
#endif

  /* ram test */

  if (test_ram(1) == false)
    goto exit_cleanup;

  if (test_rom(2) == false)
    goto exit_cleanup;

  puts("Test: There seems to be no errors!");

exit_cleanup:
  if (cartRamData != NULL){
    free(cartRamData); cartRamData = NULL;
    LOGR("CLEAN: RAMDATA",-1);
  }
  if (romData != NULL){
    unmap_rom(); romData = NULL;
    LOGR("CLEAN: ROMDATA",-1);
  }


} 



