#ifndef INCLUDED_DRM_H
#define INCLUDED_DRM_H

#include "peanut.h"

#include <signal.h>
#include <stdint.h>
#include <stdbool.h>
#include "../lvgl/lvgl.h"

typedef enum display_mode {
  display_mode_default,
  display_mode_wide,
  display_mode_full_y,
  display_mode_cut_y,

  display_mode_size
} display_mode_t;

extern volatile sig_atomic_t stop; /* programms runs as long as this is unset */
extern uint8_t line;
extern display_mode_t displayMode;

# define BYTES_PER_PIXEL 2 /* RGB565 */
# define DISPLAY_BUFF_SIZE H_RES * V_RES * BYTES_PER_PIXEL


/* sets up drm, use cleanup_drm when the program ends */
bool setup_drm(void);

/* cleans up drm */
void cleanup_drm(void);

/* draws a gameboy line to the framebuffer 
    pixels: array of RGB565 data!!
*/
void draw_drm_line(uint16_t pixels[LCD_WIDTH]);

/* displays the drawn frame */
void display_frame();

/* LVGL flush callback */
void flush_cb(lv_display_t * display, const lv_area_t * area, uint8_t * px_map);

void clear_screen();

/* INCLUDED_DRM_H */
#endif
