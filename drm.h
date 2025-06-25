#ifndef INCLUDED_DRM_H
#define INCLUDED_DRM_H

#include "peanut.h"

#include <signal.h>
#include <stdint.h>
#include <stdbool.h>

extern volatile sig_atomic_t stop; /* programms runs as long as this is unset */
extern uint8_t line;

#define DROP_FRAMES 0 /* set to 1 to drop frames that would be send while the screen is drawing, instead of busy waiting for them*/
#define LOG_DROPPED_FRAMES 1

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

/* INCLUDED_DRM_H */
#endif
