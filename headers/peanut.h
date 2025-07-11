#ifndef PEANUT_WORKAROUND_H_INCLUDED
#define PEANUT_WORKAROUND_H_INCLUDED

#include <stddef.h>

/* own thing to detect that we should exit on button press */
#define PEANUT_EXIT_CODE 0x3217 /* any value bigger than 0xff works. we use 3217 because it spells out Ezit */

/* this file is for exposing peanut_gb.h stuff, without including that file twice. */

struct gb_s;
//uint_fast32_t gb_get_save_size(struct gb_s *gb);
int gb_get_save_size_s(struct gb_s *gb, size_t *ram_size);

/* There are 154 scanlines. LY < 154. */
#define LCD_VERT_LINES      154
#define LCD_WIDTH           160
#define LCD_HEIGHT          144

#define JOYPAD_A            0x01
#define JOYPAD_B            0x02
#define JOYPAD_SELECT       0x04
#define JOYPAD_START        0x08
#define JOYPAD_RIGHT        0x10
#define JOYPAD_LEFT         0x20
#define JOYPAD_UP           0x40
#define JOYPAD_DOWN         0x80

/* PEANUT_WORKAROUND_H_INCLUDED */
#endif
