/* not a standalone file! included directly in main.c */

#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#include "headers/main.h"

/* pkill gives exit code 1, if the process is already dead. */
#define WESTON_ALEADY_DEAD_CODE 1 

/* stops weston, returns false on error, true on success */
bool kill_weston(void){

#if FULL_CONTROLL >= 1
  puts("waiting 10 secs...");
  sleep(10); /* 10 seconds (Waiting for weston to get ready, so we can cleanly kill it) */
  puts("wait done. systemctl weston away");
  system("systemctl stop weston-graphical-session.service");
#endif

  puts("killing weston...");
  int code = system("pkill weston");

  if (code != 0 && WEXITSTATUS(code) != WESTON_ALEADY_DEAD_CODE){
    printf("could not kill weston | code: %d, aborting\n",code);
    return false;
  }
  LOGR("KILL: WESTON",1);
    
  int i;
  for (i = 0; i < 300; ++i) { /* Wait up to ~30 seconds */
    if (system("pidof weston > /dev/null") != 0) {
      break; /* weston is gone */
    }
    usleep(100 * 1000); /* sleep 100ms */
  }
  if (i == 50) {
    printf("weston did not terminate in time, aborting\n");
    return false;
  }
  return true;
}




/* restarts weston */
void restart_weston(void){
  system("mkdir -p $XDG_RUNTIME_DIR");
  system("weston &");
  LOGR("START: WESTON",-1);
}
