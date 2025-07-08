#ifndef INCLUDED_TEST_RAM_H
#define INCLUDED_TEST_RAM_H

#include "../peanut.h"
#include "../ram.h"



extern bool testCartridgeRamOverwriteOn;
extern int testCartridgeRamOverwrite;
extern bool testNeverLoadFromFileOverwrite;

bool initalize_cart_ram__ram_size_overwrite(struct gb_s *gameboy, const char *const romName, int ramSize, bool forceInitToZero);

#define PUTS_RETURN(testNum,str) {printf("Test %d failed: %s\n",testNum, str); return false;}
bool test_ram(int testNumber);


/* INCLUDED_TEST_RAM_H */
#endif
