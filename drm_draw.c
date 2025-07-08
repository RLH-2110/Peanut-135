/* this file is not a sandalone file, it is only meant to be included in drm.c */

#define FB_BUFFER_SIZE (V_RES * fb0.pitch)
#define DEBUG_OOB_CHECKS 1
#define FULLSCREEN_LINES_CUT ((LCD_HEIGHT*2-V_RES) / 2) /*(/2, so we have it in gameboy LCD lines)*/
#define FULLSCREEN_CUT_TOP_AND_BOTTOM FULLSCREEN_LINES_CUT / 2 /* should be 4 */
#define FULLSCREEN_SKIP_DOUBLE_TOP_AND_BOTTOM FULLSCREEN_CUT_TOP_AND_BOTTOM * 2 /* should be 8 */

#include "lvgl/lvgl.h"

void clear_screen(){

    while (waitForFlip != 0)
      ;

    memset(back_fb_data, 0x00, V_RES * fb0.pitch);
    memset(front_fb_data, 0x00, V_RES * fb0.pitch); /* screen tearing be dammed, we are gonna clear the screen */

}

/* LVGL callback */
void flush_cb(lv_display_t * display, const lv_area_t * area, uint8_t * px_map)
{
    if (waitForFlip != 0)
    {
#if LOG_DROPPED_FRAMES
      printf("dropped frame at %d\n",time(NULL));
#endif
      lv_display_flush_ready(display);
      return;
    }

    uint8_t * buff8 = px_map; /* 16 bit (RGB565) */
    int32_t area_width = (area->x2 - area->x1 + 1) * BYTES_PER_PIXEL;
    int32_t x,y;

    for(y = area->y1; y <= area->y2; y++) {
        uint8_t *dest = back_fb_data + (fb0.pitch * y) + (area->x1 * BYTES_PER_PIXEL);
        memcpy( dest , buff8, area_width);
        buff8 += area_width;
    }
    waitForFlip = 1;

    if (drmModePageFlip(  drm_fd, crtc, back_fb, DRM_MODE_PAGE_FLIP_EVENT, NULL) != 0){
      perror("PageFLip in flush_cb");
      waitForFlip = 0;
    }

    lv_display_flush_ready(display);

}

uint8_t line = 0;


void display_mode_default_func(uint16_t pixels[LCD_WIDTH]){

  while(waitForFlip)
    ;

  uint8_t * buff8 = (uint8_t*) pixels; /* 16 bit (RGB565) */
  int32_t area_width = LCD_WIDTH * BYTES_PER_PIXEL;
  int32_t x = (H_RES - LCD_WIDTH) / 2; /* try to center the output */
  int32_t y = (V_RES - LCD_HEIGHT) / 2 + line; /* try to center the output*/

  uint8_t *dest = back_fb_data + (fb0.pitch * y) + (x * BYTES_PER_PIXEL);

# if DEBUG_OOB_CHECKS
    if (dest + area_width >= back_fb_data + FB_BUFFER_SIZE)
    printf("ERROR on part 1 of line %d! %p > %p\n",line, dest + area_width, back_fb_data + FB_BUFFER_SIZE);
# endif

  memcpy( dest , buff8, area_width);

  line++;

}

void display_mode_wide_func(uint16_t pixels[LCD_WIDTH]){

  while(waitForFlip)
    ;

  uint8_t * buff8 = (uint8_t*) pixels; /* 16 bit (RGB565) */
  int32_t area_width = LCD_WIDTH * 2 * BYTES_PER_PIXEL;
  int32_t x = (H_RES - LCD_WIDTH*2) / 2; /* try to center the output */
  int32_t y = (V_RES - LCD_HEIGHT) / 2 + line; /* try to center the output*/

  uint8_t *dest = back_fb_data + (fb0.pitch * y) + (x * BYTES_PER_PIXEL);
  //memcpy( dest , buff8, area_width);
  for (int x = 0; x < area_width;x += 2){

#   if DEBUG_OOB_CHECKS
      if (dest + x + 1 >= back_fb_data + FB_BUFFER_SIZE)
        printf("ERROR on part 1 of line %d! %p > %p\n",line, dest + x + 1, back_fb_data + FB_BUFFER_SIZE);
#   endif

    dest[x]   = buff8[x / 2];
    dest[x+1] = buff8[x / 2];
  }

  line++;

}

void display_mode_cut_y_func(uint16_t pixels[LCD_WIDTH]){

  /* cut off the first 4 lines of the output */
  if ( line < FULLSCREEN_CUT_TOP_AND_BOTTOM){
    line++;
    return;
  }
  if (line >= LCD_HEIGHT - FULLSCREEN_CUT_TOP_AND_BOTTOM){ /* cut of last 4 lines aswell */ 
    line++;
    return;
  }


  while(waitForFlip)
    ;

  uint8_t * buff8 = (uint8_t*) pixels; /* 16 bit (RGB565) */
  int32_t area_width = LCD_WIDTH * 2 * BYTES_PER_PIXEL;
  int32_t x = (H_RES - LCD_WIDTH*2) / 2; /* try to center the output */
  int32_t y = (line - FULLSCREEN_CUT_TOP_AND_BOTTOM) * 2;

  uint8_t *dest = back_fb_data + (fb0.pitch * y) + (x * BYTES_PER_PIXEL);
  for (int x = 0; x < area_width;x += 2){

#   if DEBUG_OOB_CHECKS
      if (dest + x + 1 >= back_fb_data + FB_BUFFER_SIZE)
        printf("ERROR on part 1 of line %d! %p > %p\n",line, dest + x + 1, back_fb_data + FB_BUFFER_SIZE);
#   endif

    /* draw line 1 on our screen */
    dest[x]   = buff8[x / 2];
    dest[x+1] = buff8[x / 2];

#   if DEBUG_OOB_CHECKS
      if (dest + x + 1 + fb0.pitch >= back_fb_data + FB_BUFFER_SIZE)
        printf("ERROR on part 2 of line %d! %p > %p\n",line, dest + x + 1 + fb0.pitch, back_fb_data + FB_BUFFER_SIZE);
#   endif

    /* draw duplicated line on your screen */
    dest[x   + fb0.pitch] = buff8[x / 2];
    dest[x+1 + fb0.pitch] = buff8[x / 2];
  }

  line++;
}

/* I think we are missing 1 pixel, because we draw from 0 to 270 */
uint32_t fullY;

void display_mode_full_y_func(uint16_t pixels[LCD_WIDTH]){

  if (line == 0)
    fullY = 0;  

  while(waitForFlip)
    ;

  uint8_t * buff8 = (uint8_t*) pixels; /* 16 bit (RGB565) */
  int32_t area_width = LCD_WIDTH * 2 * BYTES_PER_PIXEL;
  int32_t x = (H_RES - LCD_WIDTH*2) / 2; /* try to center the output */

  uint8_t *dest = back_fb_data + (fb0.pitch * fullY) + (x * BYTES_PER_PIXEL);
  for (int x = 0; x < area_width;x += 2){

#   if DEBUG_OOB_CHECKS
      if (dest + x + 1 >= back_fb_data + FB_BUFFER_SIZE)
        printf("ERROR on part 1 of line %d! %p > %p\n",line, dest + x + 1, back_fb_data + FB_BUFFER_SIZE);
#   endif

    /* draw line 1 on our screen */
    dest[x]   = buff8[x / 2];
    dest[x+1] = buff8[x / 2];
    /* draw duplicated line on your screen */
    if (line > FULLSCREEN_SKIP_DOUBLE_TOP_AND_BOTTOM && line < LCD_HEIGHT - FULLSCREEN_SKIP_DOUBLE_TOP_AND_BOTTOM){ /* skip duplicating the first and last 8 lines, this makes it barly fit into the screen*/

#     if DEBUG_OOB_CHECKS
        if (dest + x + 1 + fb0.pitch >= back_fb_data + FB_BUFFER_SIZE)
          printf("ERROR on part 2 of line %d! %p > %p\n",line, dest + x + 1 + fb0.pitch, back_fb_data + FB_BUFFER_SIZE);
#     endif

      dest[x   + fb0.pitch] = buff8[x / 2];
      dest[x+1 + fb0.pitch] = buff8[x / 2];
    }
  }

  if (line <= FULLSCREEN_SKIP_DOUBLE_TOP_AND_BOTTOM)
    fullY++;
  else if (line < LCD_HEIGHT - FULLSCREEN_SKIP_DOUBLE_TOP_AND_BOTTOM)
    fullY += 2;
  else
    fullY++;


  line++;

}

void draw_drm_line(uint16_t pixels[LCD_WIDTH])
{
  /* select right function. I could also done an array with function pointers, but this should keep things in order if we change the enum, and we have a branch predior. */
  switch (displayMode){
    default:
    case display_mode_default:
      display_mode_default_func(pixels);
      break;
    case display_mode_wide:
      display_mode_wide_func(pixels);
      break;
    case display_mode_full_y:
      display_mode_full_y_func(pixels);
      break;
    case display_mode_cut_y:
      display_mode_cut_y_func(pixels);
      break;
  }
}

void display_frame(){

 while(waitForFlip)
    ;

  frameCount++;
  waitForFlip = 1;

  if (drmModePageFlip(  drm_fd, crtc, back_fb, DRM_MODE_PAGE_FLIP_EVENT, NULL) != 0){
    perror("PageFLip in draw_drm_line");
    waitForFlip = 0;
  }

  line = 0;
}

