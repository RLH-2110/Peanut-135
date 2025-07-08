/* standalone file, only meant to be included in tests.c*/

#include "../peanut.h"
#include "../ram.h"
#include "test_ram.h"

bool initalize_cart_ram__ram_size_overwrite(struct gb_s *gameboy, const char *const romName, int ramSize, bool forceInitToZero){
  testCartridgeRamOverwriteOn = true;
  testCartridgeRamOverwrite = ramSize;
  testNeverLoadFromFileOverwrite = forceInitToZero; 

  if (initalize_cart_ram(gameboy,romName) == false)
    return false;

  testNeverLoadFromFileOverwrite = false;
  testCartridgeRamOverwriteOn = false;
  return true;
}

#define PUTS_RETURN(testNum,str) {printf("Test %d failed: %s\n",testNum, str); return false;}
bool test_ram(int testNumber){


  #define TEST_CART_RAM 4096
  if(initalize_cart_ram__ram_size_overwrite(NULL, "",TEST_CART_RAM,true) == false)
    PUTS_RETURN(testNumber,"initalize_cart_ram__ram_size_overwrite failed");
  


  if (cartRamData == NULL)
    PUTS_RETURN(testNumber,"cartRamData == NULL");
  if (cartRamSize != TEST_CART_RAM)
    PUTS_RETURN(testNumber,"cartRamSize != TEST_CART_RAM");

  for (int i = 0; i < cartRamSize;i++)
    if (cartRamData[i] != 0)
      PUTS_RETURN(testNumber,"cartRamData contains non 0 byte after init to 0!");
  
  return true;
}

