#ifndef INCLUDED_ROM_H
#define INCLUDED_ROM_H

#include "peanut.h"


/* shared variables for the ROM */
extern uint8_t *romData;
extern size_t romSize;

/*Returns a byte from the ROM file at the given address
  gb: unused
  addr: address to get data from
  returns: byte at address in ROM
*/
uint8_t gb_rom_read(struct gb_s *gb, const uint_fast32_t addr);

/* loads rom file, returns true on success, false on error                                                                                                                                                         uses global state: romData, romSize                                                                                                                                                                              path: path to the rom                                                                                                                                                                                            romData, will get set to a pointer to the memory mapped area of the ROM (set to NULL if mmap was unsuccesfull or did not happen)                                                                                 romSize, will get set to the size of the ROM. (set to 0 if function failed before getting the file size)                                                                                                                                                                                                                                                                                                                          returns: true if the ROM was mapped correctly, false if it failed.                                                                                                                                             */
bool load_rom_file(const char * const path);

 /* unmaps the ROM
  uses global state: romData, romSize
  romData: pointer to the ROM we want to unmap
  romSize: size of the rom to unmap
  returns: 0 if succes. other values on failure. sets Errno on failure. (currently uses munmap)
 */
int unmap_rom();

/* INCLUDED_ROM_H */
#endif
