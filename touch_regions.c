/* not a standalone file! included directly into input.c !*/

#include <stdint.h>
#include "peanut.h"
#include "button_map.h"

typedef enum touchr {
  touchr_center,
  touchr_center_up,
  touchr_center_down,
  touchr_bottom_left,
  touchr_bottom_right,
  touchr_right,
  touchr_left,
  
  touchr_invalid
} touchr_t;

#define LEFT_SIDE_X 120
#define RIGHT_SIDE_X 360

#define UP_SIDE_Y 90
#define DOWN_SIDE_Y 180

touchr_t get_touch_region(uint_fast16_t x, uint_fast16_t y){

  if (x <= LEFT_SIDE_X){
    
    if (y < DOWN_SIDE_Y)
      return touchr_left;
    else
      return touchr_bottom_left;
  }


  if (x >= RIGHT_SIDE_X){
    
    if (y < DOWN_SIDE_Y)
      return touchr_right;
    else
      return touchr_bottom_right;
  }

  /* we are in the center */

  if (y <= UP_SIDE_Y)
    return touchr_center_up;

  if (y < DOWN_SIDE_Y) /* exluding, because the border pixel belongs to the area below the center */
    return touchr_center;

  return touchr_center_down;

}

int translate_touch_region(touchr_t region){

  switch (region){
    case touchr_center:
      return TOUCH_CENTER;
    case touchr_center_up:
      return TOUCH_UP;
    case touchr_center_down:
      return TOUCH_DOWN;
    case touchr_bottom_left:
      return TOUCH_BOTTOM_LEFT;
    case touchr_bottom_right:
      return TOUCH_BOTTOM_RIGHT;
    case touchr_right:
      return TOUCH_RIGHT;
    case touchr_left:
      return TOUCH_LEFT;
    default:
      printf("Error: Invalid region %d in translate_touch_region\n",region);

  }



}

int get_button_from_touch(uint_fast16_t x, uint_fast16_t y){
  return translate_touch_region(get_touch_region(x,y));
}

#ifdef DEBUG_INPUTS
char* touch_region_to_string(touchr_t region){

  switch (region){
    case touchr_center:
      return "touchr_center"; 
    case touchr_center_up:
      return "touchr_center_up";
    case touchr_center_down:
      return "touchr_center_down"; 
    case touchr_bottom_left:
      return "touchr_bottom_left";
    case touchr_bottom_right:
      return "touchr_bottom_right";
    case touchr_right:
      return "touchr_right";
    case touchr_left:
      return "touchr_left";
    case touchr_invalid:
      return "touchr_invalid";
    default:
      return "UNKNOWN";

  }

}
#endif
