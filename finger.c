/* not a standalone file! its included directly into input.c ! */

typedef struct touch_info{

  bool slot_used;
  int osSlot;
  int fingerId;
  uint_fast16_t x;
  uint_fast16_t y;
  bool released;
  touchr_t region; /* which button region the finger is in*/
  touchr_t region_previous;
  bool initial_x; /* if set to true, then the next x update will also update region_previous */
  bool initial_y;

} touch_info_t;

#define SUPPORTED_FINGERS 3 /* values above 3 can lead to weird behavior when fingers get too close! */
touch_info_t fingers[SUPPORTED_FINGERS] = {0};
int activeFingers = 0;
int activeFingerId = -1;
int activeFingerArrId = -1;
int activeFingerOsSlot;
int lastX;
int lastY;


#ifdef DEBUG_INPUTS
void print_finger(touch_info_t *finger);
#endif


/* gets current os slot, and x,y */
void get_inital_finger_data(void){
  struct input_absinfo absInfo;

  /* Get current slot */
  if (ioctl(touchFd, EVIOCGABS(ABS_MT_SLOT), &absInfo) == 0) {
    activeFingerOsSlot = absInfo.value;
#if DEBUG_INPUTS
    printf("Initial OS slot: %d\n", activeFingerOsSlot);
#endif
  } else {
    perror("Error: Could not get initial OS slot");
    activeFingerOsSlot = 0; /* Default to slot 0 */
  }

  /* Get current x */
  if (ioctl(touchFd, EVIOCGABS(ABS_X), &absInfo) == 0) {
    lastX = absInfo.value;
#if DEBUG_INPUTS
    printf("Initial X: %d\n", lastX);
#endif
  } else {
    perror("Error: Could not get initial X");
    lastX = 0; /* fallback */
  }


  /* Get current y */
  if (ioctl(touchFd, EVIOCGABS(ABS_Y), &absInfo) == 0) {
    lastY = absInfo.value;
#if DEBUG_INPUTS
    printf("Initial Y: %d\n", lastY);
#endif
  } else {
    perror("Error: Could not get initial Y");
    lastY = 0; /* fallback */
  }


}

/* returns the id for the fingers array via the fingerId, or -1 on error */
int find_finger_with_id(int fingerId){

  for (int i = 0; i < activeFingers && i < SUPPORTED_FINGERS; i++){ /* search all active fingers, that are used */
    if (fingers[i].fingerId == fingerId && fingers[i].slot_used != false){
      return i; /* and return the array id */
    }
  }
  return -1;
}
/* returns the id for the fingers array via the os slot, or -1 on error */
int find_finger_with_slot(int osSlot){

  for (int i = 0; i < activeFingers && i < SUPPORTED_FINGERS; i++){ /* search all active fingers, that are used */
    if (fingers[i].osSlot == osSlot && fingers[i].slot_used != false){
      return i;
    }
  }
  return -1;
}

/* creates a new finger in a new slot

  return: true: finger created
          false: finger not created
*/
bool init_finger(int fingerId, int osSlot){

  int slot = -1;
  for (int i = 0; i < activeFingers; i++){ /* find free slot */
    if (fingers[i].slot_used == false){
      slot = i;
      break;
    }
  }

  if (slot == -1){ /* no free slot */
    if (activeFingers >= SUPPORTED_FINGERS){
      printf("More Fingers than Supported! Max fingers: %d\n",SUPPORTED_FINGERS);
      return false;
    }
    slot = activeFingers;
    activeFingers++;
  }


  touch_info_t *finger = &fingers[slot];

  finger->slot_used = true;
  finger->osSlot = osSlot;
  finger->fingerId = fingerId;
  finger->x = lastX;
  finger->y = lastY;
  finger->released = false;
  finger->region = get_touch_region(lastX, lastY);
  finger->region_previous = finger->region;
  finger->initial_x = true;
  finger->initial_y = true;


  activeFingerId = fingerId;
  activeFingerArrId = slot;

#if DEBUG_INPUTS >= 4

  puts("inital finger config:");
  print_finger(finger);
  puts("---");

#endif

  return true;
}


/*  selects or creates the new finger, updates activeFingerId and activeFingerArrId

    fingerId: if -1, then fings a finger by osSlot, if not found, then NO finger is created
              if any other value, then finds by fingerId, and if its not found, it will be created

    osSlot: used either when fingerId is -1 to find a finger. or to create a finger when id is != -1
*/
void switch_finger(int fingerId, int osSlot){

  if (fingerId == -1){ /* find by osSlot */
    for(int i = 0; i < activeFingers; i++){
      if (fingers[i].osSlot == osSlot){
        activeFingerArrId = i;
        activeFingerId = fingers[i].fingerId;
        return; /* found */
      }
    }
    activeFingerId = -1;
    activeFingerArrId = -1;
    return; /* not found */
  }

  /* find by fingerId, or create */

  for (int i = 0; i < activeFingers; i++){
    if (fingers[i].fingerId == fingerId){
      activeFingerId = fingerId;
      activeFingerArrId = i;
      return; /* found */
    }
  }

  /* not found */

  if (init_finger(fingerId, osSlot) == false){ /* try to create */
    /* creation error*/
    activeFingerId = -1;
    activeFingerArrId = -1;
  }
}

/* updates the position and region of a finger

  finger: pointer to the finger to modify
  x: new x pos of the finger, old pos is kept, if x is -1
  y: new y pos of the finger, old pos is kept, if y is -1
*/
void update_finger(touch_info_t *finger, uint_fast16_t x, uint_fast16_t y){

  if (finger == NULL){
    puts("Error: NULL Finger in update_finger!");
    return;
  }

  if (x != -1)
    finger->x = x;

  if (y != -1)
    finger->y = y;

  finger->region = get_touch_region(finger->x, finger->y);

  if (finger->initial_x && x != -1){
    finger->initial_x = false;
    finger->region_previous = finger->region;
  }
  if (finger->initial_y && y != -1){
    finger->initial_y = false;
    finger->region_previous = finger->region;
  }

#if DEBUG_INPUTS >= 4
  printf("finger %d got updated!\n\tRegion : %s\n\tRegionP: %s\n",finger->fingerId,touch_region_to_string(finger->region),touch_region_to_string(finger->region_previous));
#endif
}


#ifdef DEBUG_INPUTS
void print_finger(touch_info_t *finger){

  if (finger == NULL){
    puts("touch_info_t: NULL");
  }

  printf("touch_info_t:"
         "\n\tslot_used: %s"
         "\n\tosSlot   : %d"
         "\n\tfingerId : %d"
         "\n\tx    -   : %d"
         "\n\ty    -   : %d"
         "\n\treleased : %s"
         "\n\tregion          : %s"
         "\n\tregion_previous : %s"
         "\n"
         ,
         finger->slot_used == true ? "true" : "false",
         finger->osSlot,
         finger->fingerId,
         finger->x,
         finger->y,
         finger->released == true ? "true" : "false",
         touch_region_to_string(finger->region),
         touch_region_to_string(finger->region_previous)
        );



}
#endif
