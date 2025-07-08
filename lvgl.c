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
#include <stdlib.h>

#include "lcd.h"
#include "drm.h"
#include "blockmnt.h"
#include "util.h"

#define LVGL_DESIRED_TICK_INCREASE_MS 1
#define MICROSECOND_TO_MILISECOND_RATE 1000

lv_display_t *disp;
lv_indev_t *lvMouse;

lv_obj_t *pathLabel = NULL;
lv_obj_t *fileList = NULL; 
lv_obj_t *showAllFilesCheckbox = NULL;
lv_obj_t *fileNameBox = NULL;
lv_obj_t *sidePanelHomeLbl = NULL;
lv_obj_t *sidePanelRootLbl = NULL; 
lv_obj_t *okBtn = NULL;
lv_obj_t *exitBtn = NULL;

char *fileBuffer = NULL;  

pthread_t tickThread;
void *tick_thread(void* data);

volatile sig_atomic_t lvDone = false; /* gives controll back to normal main, if set to true*/


bool goto_path(char* path);

/* callbaks*/
void lvgl_sidepanel_clicked_cb(lv_event_t *e);
void lvgl_ok_exit_clicked_cb(lv_event_t *e);
void lvgl_checkbox_toggle_cb(lv_event_t *e);
void lvgl_printme_cb(lv_event_t *e);
void lvgl_change_dir_cb(lv_event_t *e);
void lvgl_path_dotdot_cb(lv_event_t *e);


/* ------------------------------ */

bool lvgl_main(void){
  lv_init();  
  LOGR("Alloc: LVGL",1);

  if (pthread_create( &tickThread, NULL, tick_thread,NULL) != 0){
    puts("Error: could not create tick thread");
    lv_deinit();
    return false;
  }
  LOGR("Thread: TICKTHREAD",1);
  
  disp = lv_display_create(H_RES, V_RES);
  if (disp == NULL){
    puts("Display error!");
    lvDone = 1;
    pthread_join(tickThread,NULL);
    lv_deinit();
    return false;
  }
  LOGR("Alloc: LVDISPLAY",1);

  lv_display_set_flush_cb(disp, flush_cb);

  /* macro defined in lcd.h */
  static uint8_t displayBuff1[DISPLAY_BUFF_SIZE];
  static uint8_t displayBuff2[DISPLAY_BUFF_SIZE];
  lv_display_set_buffers(disp, displayBuff1, displayBuff2, DISPLAY_BUFF_SIZE, LV_DISPLAY_RENDER_MODE_FULL);

  lvMouse = lv_evdev_create(LV_INDEV_TYPE_POINTER, "/dev/input/event0");
  LOGR("Alloc: LVMOUSE",1);


  char* home = getenv("HOME");


  /* STYLES */

  static lv_style_t smallFont;
  lv_style_init(&smallFont);
  lv_style_set_text_font(&smallFont,&lv_font_montserrat_10);

  /* GUI HERE */

  /* just some test gui stuff for now */
  static lv_obj_t *screen; screen = lv_obj_create(NULL);
  lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_ROW);
  lv_obj_add_style(screen,&smallFont,0);
  
  int sideButtonSize = 0;

  lv_obj_t *sidePanel = lv_obj_create(screen);
  lv_obj_t *sidePanelRootBtn = lv_btn_create(sidePanel);
  sidePanelRootLbl = lv_label_create(sidePanelRootBtn);
  lv_obj_center(sidePanelRootLbl);
  lv_obj_t *sidePanelHomeBtn = NULL;
  sidePanelHomeLbl = NULL;
  if (home != NULL){
    sidePanelHomeBtn = lv_btn_create(sidePanel);
    sidePanelHomeLbl = lv_label_create(sidePanelHomeBtn);
  }

  /*side pannel stuff */
  
    lv_obj_set_width(sidePanel, 100);
    lv_obj_set_height(sidePanel, LV_PCT(100));
    lv_obj_set_style_pad_all(sidePanel, 5, 0);
    lv_obj_set_flex_flow(sidePanel, LV_FLEX_FLOW_COLUMN);
      
    lv_label_set_text(sidePanelRootLbl,"/");
    if (sidePanelHomeLbl != NULL)
      lv_label_set_text(sidePanelHomeLbl,"home");

    lv_obj_add_event_cb(sidePanelRootBtn, lvgl_sidepanel_clicked_cb,LV_EVENT_CLICKED,NULL); 
    if (sidePanelHomeBtn != NULL)
      lv_obj_add_event_cb(sidePanelHomeBtn, lvgl_sidepanel_clicked_cb,LV_EVENT_CLICKED,NULL); 

    if (sidePanelHomeBtn != NULL){
      lv_obj_update_layout(sidePanelHomeBtn);
      sideButtonSize = lv_obj_get_width(sidePanelHomeBtn); /* every side button will be as big as the home button */
    }

    /* for dynamically addings buttons */

#define MAX_LVGL_SCSI 32 /* I want to see the person that mounts more than 32 partitions when there are only 4 usb slots */
    lv_obj_t *scsiButtons[MAX_LVGL_SCSI + 1] = { 0 }; /* plus one, so we always have a NULL terminator */
    lv_obj_t *scsiLabels[MAX_LVGL_SCSI + 1] = { 0 };  
    static char scsiStrings[MAX_LVGL_SCSI+1][sizeof("sda1")] = { 0 }; 

    mount_list_t* mountedScsi = get_mounted_partitions();
    mount_list_t* current = mountedScsi;

    /* add all SCSI partitions */
    for (int i = 0; current != NULL && i < MAX_LVGL_SCSI; i++, current = current->next){
      scsiButtons[i] = lv_btn_create(sidePanel);
      scsiLabels[i] = lv_label_create(scsiButtons[i]);
      
      /* copy the "sda1" part from "/dev/sda1" */
      memcpy( scsiStrings[i], current->device + strlen("/dev/"), strlen("sda1"));
      scsiStrings[i][sizeof("sda1")] = '\0';

      lv_label_set_text(scsiLabels[i],    scsiStrings[i]);
      lv_obj_add_event_cb(scsiButtons[i], lvgl_sidepanel_clicked_cb,LV_EVENT_CLICKED,NULL);

      if (sideButtonSize != 0) /* if we have a known size, set this button to that size*/
        lv_obj_set_width(scsiButtons[i],sideButtonSize);
    }
    free_mount_list(mountedScsi);
  
    if (sideButtonSize == 0 && scsiButtons[0] != NULL) { /* the home button does not exist, so we cant use its size, get the size from sda1, since its the biggest thing after the home button */
      lv_obj_update_layout(scsiButtons[0]);
      sideButtonSize = lv_obj_get_width(scsiButtons[0]);
    }
    lv_obj_set_width(sidePanelRootBtn,sideButtonSize);
    

  lv_obj_t *mainPanel = lv_obj_create(screen);
  lv_obj_set_height(mainPanel, LV_PCT(100));
  lv_obj_set_flex_flow(mainPanel, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_grow(mainPanel, 1);
  lv_obj_set_scroll_dir(mainPanel, LV_DIR_NONE);


    lv_obj_t *pathBox = lv_obj_create(mainPanel);
    lv_obj_set_width(pathBox, LV_PCT(100));
    lv_obj_set_flex_flow(pathBox, LV_FLEX_FLOW_ROW);
    lv_obj_t *pathExplainLbl = lv_label_create(pathBox);
    lv_label_set_text(pathExplainLbl,"Path: ");
    pathLabel = lv_label_create(pathBox); 
    lv_label_set_text(pathLabel, "/");
    lv_obj_set_height(pathBox, LV_SIZE_CONTENT);



    fileList = lv_list_create(mainPanel);
    lv_obj_set_height(fileList, 150); 
    lv_obj_set_width(fileList, LV_PCT(100));
    lv_obj_set_scroll_dir(fileList, LV_DIR_VER);



    lv_obj_t *bottomRow = lv_obj_create(mainPanel);
    lv_obj_set_width(bottomRow, LV_PCT(100));
    lv_obj_set_flex_flow(bottomRow, LV_FLEX_FLOW_COLUMN);

    
      lv_obj_t *fileRow = lv_obj_create(bottomRow);
      lv_obj_set_width(fileRow, LV_PCT(100));
      lv_obj_set_flex_flow(fileRow, LV_FLEX_FLOW_ROW);
      lv_obj_set_height(fileRow, LV_SIZE_CONTENT);
      lv_obj_set_flex_align(fileRow,LV_FLEX_ALIGN_START,LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_START);

        lv_obj_t *fileLabel = lv_label_create(fileRow);
        lv_label_set_text(fileLabel, "file:");

        lv_obj_t *fileNameFakeBox = lv_obj_create(fileRow);
        lv_obj_set_width(fileNameFakeBox, LV_PCT(100));
        lv_obj_set_height(fileNameFakeBox, LV_SIZE_CONTENT);
        lv_obj_set_style_pad_all(fileNameFakeBox,2,0);
        lv_obj_set_flex_grow(fileNameFakeBox, 1); /* Take remaining space */

          fileNameBox = lv_label_create(fileNameFakeBox);
          lv_obj_set_height(fileNameBox, LV_SIZE_CONTENT);
          lv_label_set_text(fileNameBox, "");


      lv_obj_t *buttonRow = lv_obj_create(bottomRow);
      lv_obj_set_width(buttonRow, LV_PCT(100));
      lv_obj_set_flex_flow(buttonRow, LV_FLEX_FLOW_ROW);
      lv_obj_set_height(buttonRow, LV_SIZE_CONTENT);
      lv_obj_set_flex_align(buttonRow,LV_FLEX_ALIGN_START,LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_START);

        okBtn = lv_btn_create(buttonRow);
        lv_obj_t *okLabel = lv_label_create(okBtn);
        lv_label_set_text(okLabel, "OK");
        lv_obj_add_event_cb(okBtn, lvgl_ok_exit_clicked_cb,LV_EVENT_CLICKED,NULL);

        exitBtn = lv_btn_create(buttonRow);
        lv_obj_t *exitLabel = lv_label_create(exitBtn);
        lv_label_set_text(exitLabel, "Exit");
        lv_obj_add_event_cb(exitBtn, lvgl_ok_exit_clicked_cb,LV_EVENT_CLICKED,NULL);
        
        showAllFilesCheckbox = lv_checkbox_create(buttonRow);
        lv_checkbox_set_text(showAllFilesCheckbox, "Show all files");
        lv_obj_add_event_cb(showAllFilesCheckbox, lvgl_checkbox_toggle_cb, LV_EVENT_VALUE_CHANGED, NULL);


      lv_obj_set_height(bottomRow, LV_SIZE_CONTENT);


  lv_obj_set_style_border_width(fileRow, 0, 0);
  lv_obj_set_style_border_width(buttonRow, 0, 0);

  lv_obj_set_style_pad_all(mainPanel, 2, 0);
  lv_obj_set_style_pad_all(fileList, 2, 0);
  lv_obj_set_style_pad_all(pathBox, 2, 0);
  lv_obj_set_style_pad_all(bottomRow, 2, 0);
  lv_obj_set_style_pad_all(fileRow, 0, 0);
  lv_obj_set_style_pad_all(buttonRow, 0, 0);

  lv_screen_load_anim(screen,LV_SCR_LOAD_ANIM_NONE,0,0,true);

  /* GUI END */

  // set innital path 
  bool success = false;
  if (home != NULL)
    success = goto_path(home);
  else
    success = goto_path("/");

  if (success == false)
    lvDone = true;

  while(!stop && !lvDone){
    uint32_t timeTillNext = lv_timer_handler();
    usleep(timeTillNext*MICROSECOND_TO_MILISECOND_RATE );
  }
 
  puts("Info closing LVGL...");
 
  pthread_join(tickThread,NULL);
  LOGR("Clean: TICKTHREAD",-1);
  lv_indev_delete(lvMouse);
  LOGR("Clean: LVMOUSE",-1);
  lv_disp_remove(disp);
  LOGR("Clean: LVDISPLAY",-1);
  lv_deinit();
  LOGR("Clean: LVGL",-1);


  if (fileBuffer != NULL){
    free(fileBuffer);
    LOGR("Clean: FILEBUFFER",-1);
  }

  puts("Info: LVGL closed!");
}


/* --------------------------------- */

bool goto_path(char* path){

  if (pathLabel == NULL){
    puts("Error: pathLabel not set!");
    return false;
  }
  if (fileList == NULL){
    puts("Error: fileList not set!");
    return false;
  }
  if (showAllFilesCheckbox == NULL){
    puts("Error: showAllFilesCheckbox not set!");
    return false;
  }
  
  size_t fileBufferSize = 1024;
  size_t fileBufferIndex = 0;

  /* either alloc a buffer, or re-use an old one*/
  if (fileBuffer == NULL){
    fileBuffer = malloc(fileBufferSize);  
    if (fileBuffer == NULL){
      puts("Error: Out of memory!");
      return false;
    }
    LOGR("Alloc FILEBUFFER",1);
  }
  
  bool showAllFiles = (lv_obj_get_state(showAllFilesCheckbox) & LV_STATE_CHECKED) != 0;
  scan_path(path, fileBuffer, &fileBufferSize, &fileBufferIndex, !showAllFiles,false,false);

  /* KILL ALL THE CHILDREN !!! */
  lv_obj_clean(fileList);

  if (strcmp(path,"/") != 0){ /* if not "/"  */
    lv_obj_t *button = lv_list_add_button(fileList, NULL, ".."); 
    lv_obj_add_event_cb(button, lvgl_path_dotdot_cb,LV_EVENT_CLICKED,NULL);
  }

  for (int i = 0; i < fileBufferIndex; i += strlen(fileBuffer + i) + 1){

    if (is_dir_path(path,fileBuffer + i)){
      lv_obj_t *button = lv_list_add_button(fileList, LV_SYMBOL_DIRECTORY, fileBuffer + i); 
      lv_obj_add_event_cb(button, lvgl_change_dir_cb,LV_EVENT_CLICKED,NULL); /* directory */
    }else{
      lv_obj_t *button = lv_list_add_button(fileList, LV_SYMBOL_FILE, fileBuffer + i); 
      lv_obj_add_event_cb(button, lvgl_printme_cb,LV_EVENT_CLICKED,NULL); /* file */
    }


  }

  lv_label_set_text(pathLabel,path);
puts("done");
  return true;
}

/* --------------------------------- */
void lvgl_sidepanel_clicked_cb(lv_event_t *e){

  /* if any of them are NULL*/
  if (!sidePanelHomeLbl || !sidePanelRootLbl)
    return;

  lv_obj_t *label = lv_obj_get_child(lv_event_get_target_obj(e),0);

  char* path = NULL;

  if (label == sidePanelHomeLbl)
    path = getenv("HOME");
  else if (label == sidePanelRootLbl)
    path = "/";
  else if (strlen(lv_label_get_text(label)) == 4) { /* for sda1 and stuff*/
    char devicePath[] = "/dev/\0da1";
    char *devicePathSdStart = devicePath + strlen(devicePath);

    strcpy(devicePathSdStart,lv_label_get_text(label)); 
    path = find_mount_point(devicePath);    
    printf("searched for '%s' and found '%s'\n",devicePath,path);

  }else{
    puts("Error: Invalid quick access link!");
  }

  if (path == NULL)
    return;
  
  goto_path(path);
}

/* --------------------------------- */
void lvgl_printme_cb(lv_event_t *e){

  if (fileNameBox == NULL)
    return;

  lv_obj_t *label = lv_obj_get_child(lv_event_get_target_obj(e),1);

  /*copy the name */
  static char staticBuff[LV_FS_MAX_PATH_LENGTH];

  if (strlen(lv_label_get_text(label)) >= LV_FS_MAX_PATH_LENGTH){
    printf("Error: %s is too long in lvgl_printme_cb!\n");
    return;
  }
  strcpy(staticBuff, lv_label_get_text(label));

  lv_label_set_text(fileNameBox, staticBuff);
}
/* --------------------------------- */
void lvgl_change_dir_cb(lv_event_t *e){

  if (pathLabel == NULL)
    return;

  lv_obj_t *label = lv_obj_get_child(lv_event_get_target_obj(e),1);

  char* path = lv_label_get_text(pathLabel);
  char* dir  = lv_label_get_text(label);

  if (strlen(path) + strlen(dir) + 2 >= LV_FS_MAX_PATH_LENGTH) /* +2 because NULL terminator and space to add a missing "/" */ 
    return;
  


  /*change the dir */

  char buff[LV_FS_MAX_PATH_LENGTH];
  strcpy(buff, path);

  size_t buffLen = strlen(buff);
  if (buff[buffLen -1] != '/'){ /* if path does not end in "/", then append it */
    buff[buffLen + 0] = '/';
    buff[buffLen + 1] = '\0';
  }

  strcpy(buff+strlen(buff),dir);
  
  goto_path(buff);
}

/* --------------------------------- */
void lvgl_path_dotdot_cb(lv_event_t *e){
  if (pathLabel == NULL)
    return;

  char* orig = lv_label_get_text(pathLabel);
  /* set pathEnd to the end of the current path*/  
  char* pathEnd = orig + strlen(orig);

  /* go till you find the first '/' or the start of the string */
  while(pathEnd > orig && *pathEnd != '/')
    pathEnd--;

  if (*pathEnd == '\0')
    return;  /* should never happen! best if we dont mess it with further */

  if (pathEnd == orig){ /*we got to the start of the string, just force in "/" */
    orig[0] = '/';
    orig[1] = '\0';
  }else{
    /* we found a '/' thats not at the start! lets replace it with \0 */
    *pathEnd = '\0';
  }

  goto_path(orig);

}

/* --------------------------------- */

void lvgl_checkbox_toggle_cb(lv_event_t *e){
  puts("checkbox toggled");
  if (pathLabel != NULL)
    goto_path(lv_label_get_text(pathLabel)); /* refrech path, it checks showAllFilesCheckbox directly */
}

/* --------------------------------- */

void lvgl_ok_exit_clicked_cb(lv_event_t *e){

  /* if any of them are NULL, return */
  if (!okBtn || !exitBtn || !fileNameBox || !pathLabel)
    return;

  lv_obj_t *button = lv_event_get_target_obj(e);

  if (button == okBtn){

    /* construct absolute path */
    static char constructedPath[LV_FS_MAX_PATH_LENGTH+1] = {0};
    path_construct(constructedPath,LV_FS_MAX_PATH_LENGTH+1,lv_label_get_text(pathLabel),lv_label_get_text(fileNameBox));
    printf("Debug: constucted Path: %s\n",constructedPath);

    romFile = constructedPath; 
    
    if (lv_label_get_text(fileNameBox)[0] == '\0')
      romFile = NULL;

  }else if (button == exitBtn){

    romFile = NULL; 

  }else{
    puts("Error: Invalid exit button!");
    romFile = NULL;
  }

  lvDone = 1;
}

/* --------------------------------- */


void *tick_thread(void* data) {
  (void) data;
  while(!stop && !lvDone)
  {
    usleep(LVGL_DESIRED_TICK_INCREASE_MS * MICROSECOND_TO_MILISECOND_RATE);
    lv_tick_inc(LVGL_DESIRED_TICK_INCREASE_MS); /*Tell LVGL how many time has eslaped in ms*/
  }
}
