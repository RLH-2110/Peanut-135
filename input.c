#include <fcntl.h>
#include <linux/input.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <signal.h>
#include <poll.h>

#include "headers/input.h"
#include "headers/peanut.h"
#include "headers/drm.h" /* defines volitile sig_atomic_t stop */
#include "headers/main.h"
#include "headers/button_map.h"

#define DEBUG_INPUTS 0

#include "touch_regions.c" /* for debug stuff needs to be after the define */

int buttonsFd = -1;
int touchFd = -1;

bool buttonThreadStarted = false;
pthread_t buttonThread;
void *button_thread(void* data);
bool touchThreadStarted = false;
pthread_t touchThread;
void *touch_thread(void* data);

pthread_mutex_t inputLock = PTHREAD_MUTEX_INITIALIZER;

volatile uint_fast8_t button_presses = 0; 
volatile uint_fast8_t button_presses_released = 0; 

bool init_input(void){
  buttonsFd = open("/dev/input/event1", O_RDONLY | O_NONBLOCK);
  if (buttonsFd < 0){
    puts("Error: Failed to get button input!");
    cleanup_input();
    return false;
  }
  LOGR("ALLOC: BUTTONSFD",1);

  touchFd = open("/dev/input/event0", O_RDONLY | O_NONBLOCK);
  if (touchFd < 0){
    puts("Error: Failed to get touchpad input!");
    cleanup_input();
    return false;
  }
  LOGR("ALLOC: TOUCHFD",1);

  if (pthread_create( &buttonThread, NULL, button_thread,NULL) != 0){
    puts("Error: could not create button thread");
    cleanup_input();
    return false;
  }
  buttonThreadStarted = true;

  if (pthread_create( &touchThread, NULL, touch_thread,NULL) != 0){
    puts("Error: could not create touch thread");
    cleanup_input();
    return false;
  }
  touchThreadStarted = true;


  return true;
}


uint8_t get_input(void){

  uint8_t inputs = 0;  

  pthread_mutex_lock(&inputLock); 

#if DEBUG_INPUTS >= 2
  bool logThis = false;
  if (button_presses != 0 || button_presses_released != 0){
    logThis = true;
    printf("pre  button_presses  : %X\n",button_presses);
    printf("pre  button_presses_r: %X\n",button_presses_released);
  }
#endif

  inputs = button_presses; /* get pressed buttons */
  button_presses ^= button_presses_released; /* remove released buttons, we still count them, since the user can not time their inputs precisly enough for each ms we check for button presses */
  button_presses_released = 0;

#if DEBUG_INPUTS >= 2
  if (logThis){
    printf("post button_presses  : %X\n",button_presses);
    printf("post button_presses_r: %X\n",button_presses_released);
  }
#endif
  pthread_mutex_unlock(&inputLock);

#if DEBUG_INPUTS
  if (inputs & JOYPAD_A)
    puts("A is pressed");
  if (inputs & JOYPAD_B)
    puts("B is pressed");
  if (inputs & JOYPAD_SELECT)
    puts("SELECT is pressed");
  if (inputs & JOYPAD_START)
    puts("START is pressed");
  if (inputs & JOYPAD_UP)
    puts("UP is pressed");
  if (inputs & JOYPAD_RIGHT)
    puts("RIGHT is pressed");
  if (inputs & JOYPAD_DOWN)
    puts("DOWN is pressed");
  if (inputs & JOYPAD_LEFT)
    puts("LEFT is pressed");
#endif

  return ~inputs; /* invert, because for the gameboy: 0 = pressed | 1 = not pressed */
}

void cleanup_input(void){

  if (buttonThreadStarted){
    pthread_join(buttonThread,NULL);
    LOGR("END THREAD: BUTTON",-1);
    buttonThreadStarted = false;
  }

  if (touchThreadStarted){
    pthread_join(touchThread,NULL);
    LOGR("END THREAD: TOUCH",-1);
    touchThreadStarted = false;
  }

  if (buttonsFd >= 0){
    close(buttonsFd); LOGR("CLEAN: BUTTONFD",-1); buttonsFd = -1;
  }
  if (touchFd >= 0){
    close(touchFd); LOGR("CLEAN: TOUCHFD",-1); touchFd = -1;
  }
}



/* touchscreen stuff */

#include "finger.c"

/* threads */ 


void *touch_thread(void* data){
  LOGR("THREAD CREATE: TOUCH",1);

#if DEBUG_INPUTS
  puts("touch_thread stated");
#endif
  struct pollfd pollStruct = {touchFd, POLLIN, 0}; 
  struct input_event ev;

  get_inital_finger_data();
  activeFingerArrId = -1;
  activeFingerId = -1;

  while(!stop){

    if (poll(&pollStruct,1,500) < 0){
      perror("Warning: Touch Thread Poll Error");
      continue;
    }

    int readBytes = 0;
    if ( pollStruct.revents & POLLIN ) {
      readBytes = read(touchFd, &ev, sizeof(ev));
      if (readBytes != sizeof(ev)){
        printf("only read %d out of %d bytes!",readBytes, sizeof(ev));
        continue;
      }

#if DEBUG_INPUTS >= 3
{
    if (ev.type != EV_ABS)
      printf("touch type %hu code %hu value %d\n", ev.type, ev.code, ev.value);

    else if (ev.type == EV_ABS){
      if (ev.code != ABS_MT_TRACKING_ID && ev.code != ABS_MT_POSITION_X && ev.code != ABS_MT_POSITION_Y && ev.code != ABS_MT_SLOT)
        printf("touch type EV_ABS code %d value %d\n", ev.code, ev.value);
      else{
        char *code[] = { "ABS_MT_TRACKING_ID", "ABS_MT_POSITION_X", "ABS_MT_POSITION_Y", "ABS_MT_SLOT" };
        int id = 0;
        switch(ev.code){
          case ABS_MT_TRACKING_ID: id = 0; break;
          case ABS_MT_POSITION_X: id = 1; break;
          case ABS_MT_POSITION_Y: id = 2; break;
          case ABS_MT_SLOT: id = 3; break;
        }
        printf("touch type EV_ABS code %s value %d\n", code[id], ev.value);
      } 

    }
}
#endif

      if (ev.type == EV_ABS){
      
        switch(ev.code){

          case ABS_MT_TRACKING_ID:
            if (ev.value != -1)
              switch_finger(ev.value,activeFingerOsSlot);
            else if (activeFingerArrId != -1){
              fingers[activeFingerArrId].released = true;
            }
            continue;        
          
          case ABS_MT_POSITION_X:
            if (activeFingerArrId != -1)
              update_finger(&fingers[activeFingerArrId],ev.value,-1);
            continue;

          case ABS_MT_POSITION_Y:
            if (activeFingerArrId != -1)
              update_finger(&fingers[activeFingerArrId],-1,ev.value);
            continue;
          
          case ABS_MT_SLOT:
            activeFingerOsSlot = ev.value;
            switch_finger(-1,activeFingerOsSlot);

        }

      }else if (ev.type == EV_SYN){
#if DEBUG_INPUTS >= 3
        puts("commiting inputs!");
#endif       

        /* commit finger info! */
        uint_fast8_t pressed_stuff = 0;
        uint_fast8_t released_stuff = 0;

        for (int i = 0; i < activeFingers; i++){ /* for all used fingers */

          touch_info_t *finger = &fingers[i];

#if DEBUG_INPUTS >= 3
          print_finger(finger);          
#endif

          if (finger->slot_used == false)
            continue;

          int mask = get_button_from_touch(finger->x, finger->y); /* register current finger button and update finger if released */


          if (finger->released == true){

            released_stuff |= mask;
            finger->slot_used = false;
          }else{
            pressed_stuff |= mask;
          }

          if (finger->region != finger->region_previous){  /* unset buttons where the user moved their finger off them */
            released_stuff |= translate_touch_region(finger->region_previous);
            finger->region_previous = finger->region;
          }

        } /* end of loop for every finger */

#if DEBUG_INPUTS >= 3
        printf("activeFingers before clean: %d\n",activeFingers);
#endif

        /* clean up the activeFingers count */
        while (activeFingers - 1 >= 0 && fingers[activeFingers - 1].slot_used == false)
          activeFingers--;

#if DEBUG_INPUTS >= 3
        printf("activeFingers after clean:  %d\n",activeFingers);
#endif

        /* set the inputs */

        /* fix up released regions, that are still pressed by other fingers */
        released_stuff &= ~pressed_stuff; /* release buttons that are not pressed by other fingers */

#if DEBUG_INPUTS >= 3
        printf("pressed  stuff: %X\n",pressed_stuff);
        printf("released stuff: %X\n",released_stuff);
#endif

        pthread_mutex_lock(&inputLock); 
        button_presses |= pressed_stuff;
        button_presses_released |= released_stuff; 
        pthread_mutex_unlock(&inputLock);
  
      }else{
        if (ev.type != EV_KEY)
          printf("Warning: unexpected ev.type %d\n\tExpected %d or %d\n",ev.type, EV_ABS, EV_SYN);
      } 

    }
  }
#if DEBUG_INPUTS
  puts("touch_thread ended");
#endif
 
}



//////////////////////////////////////////////////////////////////////


void *button_thread(void* data){
  LOGR("THREAD CREATE: BUTTON",1);

#if DEBUG_INPUTS
  puts("button_thread stated");
#endif
  struct pollfd pollStruct = {buttonsFd, POLLIN, 0}; 
  struct input_event ev;

  while(!stop){

    if (poll(&pollStruct,1,500) < 0){
      perror("Warning: Button Thread Poll Error");
      continue;
    }

    if ( pollStruct.revents & POLLIN && read(buttonsFd, &ev, sizeof(ev)) == sizeof(ev)) {


#if DEBUG_INPUTS >= 3
    printf("button type %hu code %hu value %d\n", ev.type, ev.code, ev.value);
#endif

      if (ev.type == EV_KEY){

        /* A */
        if (ev.code == BTN_1){
          if (ev.value == 1){  /* pressed */
            pthread_mutex_lock(&inputLock); 
            button_presses |= BTN_1_KEY; /* add to presses */
            button_presses_released = button_presses_released & (~BTN_1_KEY); /* remove from released */
            pthread_mutex_unlock(&inputLock);
          }
          else{ /* released */
            pthread_mutex_lock(&inputLock); 
            button_presses_released |= BTN_1_KEY; /* add to released */
            pthread_mutex_unlock(&inputLock);
          }  
        }

        /* B */
        if (ev.code == BTN_2){
          if (ev.value == 1){  /* pressed */
            pthread_mutex_lock(&inputLock); 
            button_presses |= BTN_2_KEY; /* add to presses */
            button_presses_released = button_presses_released & (~BTN_2_KEY); /* remove from released */
            pthread_mutex_unlock(&inputLock);
          }
          else{ /* released */
            pthread_mutex_lock(&inputLock); 
            button_presses_released |= BTN_2_KEY; /* add to released */
            pthread_mutex_unlock(&inputLock);
          }  
        }



      }
    }
  }
#if DEBUG_INPUTS
  puts("button_thread ended");
#endif
 
}
