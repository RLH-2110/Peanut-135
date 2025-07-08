#ifndef INCLUDED_RAM_H
#define INCLUDED_RAM_H


#include "peanut.h"


/* shared variables for the RAM */
extern uint8_t *cartRamData;
extern size_t cartRamSize;


/*writes a byte to the RAM at the given address
  gb: unused
  addr: address to write data to
  returns: byte at address in RAM
*/
void gb_cart_ram_write(struct gb_s* gameboy, const uint_fast32_t addr, const uint8_t data);


/*Returns a byte from the RAM at the given address
  gb: unused
  addr: address to get data from
  returns: byte at address in RAM
*/
uint8_t gb_cart_ram_read(struct gb_s* gameboy, const uint_fast32_t addr);


/* loads ram file, if one exist, otherwhise loads Cartride ram with 0. returns true on success, false on error
  uses global state: cartRamData, cartRamSize
  romName: path to the ROM file
  cartRamData, will get set to a pointer to the allocated memory for the RAM (set to NULL if function is unsuccesfull)
  cartRamSize, will get set to the size of the RAM.

  returns: true if the RAM was set up correctly, false if it failed.
*/
bool initalize_cart_ram(struct gb_s *gameboy, const char *const romName);

/* saves ram to the save file, if one exist it is overwritten. returns true on success, false on error
  uses global state: cartRamData, cartRamSize
  romName: path to the ROM file

  returns: true if the savefile was written correctly false if it failed.
*/
bool save_cart_ram(struct gb_s *gameboy, const char *const romName);

/* gets path of savefile for a rom path */
char* get_save_name(const char *const romName);

/* INCLUDED_RAM_H */
#endif
