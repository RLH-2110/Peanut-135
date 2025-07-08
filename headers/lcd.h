#ifndef INCLUDED_LCD_H
#define INCLUDED_LCD_H


#include "peanut.h"

#define H_RES (480)
#define V_RES (272)

extern uint16_t *pallet;

void lcd_draw_line(struct gb_s *gb, const uint8_t *pixels, const uint_fast8_t line);

/* pallets */

extern uint16_t SHADES565_GRAY[4];
extern uint16_t SHADES565_DARK_GREEN[4];
extern uint16_t SHADES565_LIGHT_GREEN[4];

/* INCLUDED_LCD_H */
#endif
