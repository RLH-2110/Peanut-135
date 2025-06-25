#ifndef PEANUT_WORKAROUND_H_INCLUDED
#define PEANUT_WORKAROUND_H_INCLUDED

/* this file is for exposing peanut_gb.h stuff, without including that file twice. */

struct gb_s;
uint_fast32_t gb_get_save_size(struct gb_s *gb);

/* There are 154 scanlines. LY < 154. */
#define LCD_VERT_LINES      154
#define LCD_WIDTH           160
#define LCD_HEIGHT          144

/* PEANUT_WORKAROUND_H_INCLUDED */
#endif
