#include<stdint.h>


#include "headers/lcd.h"
#include "headers/peanut.h"
#include "headers/util.h"
#include "headers/drm.h"

uint16_t SHADES565_GRAY[4] = { 0xffff, 0xad75, 0x6b4d , 0x0000 };
uint16_t SHADES565_DARK_GREEN[4] = { 0xffff, 0x7c02, 0x5bc8 , 0x0000 };
uint16_t SHADES565_LIGHT_GREEN[4] = { 0xffff, 0x0590, 0x04ce , 0x0000 };

uint16_t *pallet = SHADES565_GRAY;

#define SHADE(gbPixel) (gbPixel & 0b00000011)

/*
  gb: unused
  pixels: array for uint8. 0b0011_0000 are the objects, 0b0000_0011 are the shades (00 = white 11 = black);
*/
void lcd_draw_line(struct gb_s *gb, const uint8_t *pixels,	const uint_fast8_t line){

  /* translate pixels */

  uint16_t pixels565[LCD_WIDTH];
  for (int i = 0; i < LCD_WIDTH;i++)
    pixels565[i] = pallet[SHADE(pixels[i])];

  draw_drm_line(pixels565);
}




