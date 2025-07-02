/* Do not add to Makefile, this file is included in main.c */

/* trick vim into allowing me to use auto complete options for main.c variables */
#if 0 == 1
#include "main.c"
#endif

#include "lv_conf.h"
#include "lvgl/lvgl.h"
#include "lvgl/src/drivers/evdev/lv_evdev.h"

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#include "lcd.h"
#include "drm.h"

#define LVGL_DESIRED_TICK_INCREASE_MS 1
#define MICROSECOND_TO_MILISECOND_RATE 1000

/* lvgl stuff and threats */
lv_display_t *disp;
lv_indev_t *lvMouse;

pthread_t tickThread;
void *tick_thread(void* data);

volatile sig_atomic_t lvDone = false; /* gives controll back to normal main, if set to true*/

bool lvgl_main(void){
  lv_init();  

  if (pthread_create( &tickThread, NULL, tick_thread,NULL) != 0){
    puts("Error: could not create tick thread");
    lv_deinit();
    return false;
  }
  
  disp = lv_display_create(H_RES, V_RES);
  if (disp == NULL){
    puts("Display error!");
    lvDone = 1;
    pthread_join(tickThread,NULL);
    lv_deinit();
    return false;
  }

  lv_display_set_flush_cb(disp, flush_cb);


//# define BYTES_PER_PIXEL_LV (LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_RGB565))
//# define DISPLAY_BUFF_SIZE H_RES * V_RES * BYTES_PER_PIXEL_LV
  static uint8_t displayBuff1[DISPLAY_BUFF_SIZE];
  static uint8_t displayBuff2[DISPLAY_BUFF_SIZE];
  lv_display_set_buffers(disp, displayBuff1, displayBuff2, DISPLAY_BUFF_SIZE, LV_DISPLAY_RENDER_MODE_FULL);

  lvMouse = lv_evdev_create(LV_INDEV_TYPE_POINTER, "/dev/input/event0");


  /* GUI HERE */

  /* just some test gui stuff for now */
  static lv_obj_t *screen; screen = lv_obj_create(NULL);
  
  lv_obj_t * spinner = lv_spinner_create(screen);
  lv_obj_set_size(spinner, 100, 100);
  lv_spinner_set_anim_params(spinner,1000 ,250); /* 250 is just a random angle. 0 looks ugly*/
  lv_obj_center(spinner);
  lv_screen_load_anim(screen,LV_SCR_LOAD_ANIM_NONE,0,0,true);

  /* GUI END */

  while(!stop && !lvDone){
    uint32_t timeTillNext = lv_timer_handler();
    usleep(timeTillNext*MICROSECOND_TO_MILISECOND_RATE );
  }
 
  puts("Info closing LVGL...");
 
  pthread_join(tickThread,NULL);
  lv_indev_delete(lvMouse);
  lv_disp_remove(disp);
  lv_deinit();

  puts("Info: LVGL closed!");
}

void *tick_thread(void* data) {
  (void) data;
  while(!stop && !lvDone)
  {
    usleep(LVGL_DESIRED_TICK_INCREASE_MS * MICROSECOND_TO_MILISECOND_RATE);
    lv_tick_inc(LVGL_DESIRED_TICK_INCREASE_MS); /*Tell LVGL how many time has eslaped in ms*/
  }
}
