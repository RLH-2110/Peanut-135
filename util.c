#include <stdarg.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdio.h>




/* calls pritnf with the given parameters, and then gives the user the choice of yes or no. 
  parameters: same as printf
  returns: true if user selected Y
           false if user selected N
*/
bool choice(const char * const formatStr, ...){

  va_list args;
  va_start (args, formatStr);

  if (formatStr != NULL)
    printf(formatStr,args);

  va_end(args);

  char userChoice = ' ';
  while(userChoice != 'Y' && userChoice != 'N'){
    userChoice =  toupper(getchar());
  }

  if (userChoice == 'N')
    return false;
  return true;
  
}
