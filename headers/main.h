#ifndef INCLUDED_MAIN_H
#define INCLUDED_MAIN_H

#include <signal.h>

#define FULL_CONTROLL 1 /* if set, then the Emulator will not Terminate (unless its via a signal) and it may do other things on the system its running on*/

#define RLH_TEST 0 /* currently unused, wont do anything except renaming the main to run_main */
#define LOG_RESOURCES 0
extern int resource_count;
extern volatile sig_atomic_t romSelect;

extern char* showLvglErrorMsg; /* can be set to any string, and on lvgl start, it will display it, and restet it to NULL */

#if LOG_RESOURCES
#   define LOGR(msg, inc) {\
      resource_count += inc;\
      printf("%s | TOTAL RESOURCES: %d\n",msg,resource_count);\
    }
#else
#   define LOGR(msg, inc) ;
#endif

#define ADDR_SHUTDOWN &resource_count /* this is an address, that we know can not be a valid string! if we return this addres, if a string is expected, then it means we want to shut down. this is an ugly hack.*/

/* INCLUDED_MAIN_H */
#endif
