#include <stdarg.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdint.h>


/* calls pritnf with the given parameters, and then gives the user the choice of yes or no. 
  parameters: same as printf
  returns: true if user selected Y
           false if user selected N
*/
bool choice(const char * const formatStr, ...){

  va_list args;
  va_start (args, formatStr);

  if (formatStr != NULL)
    vprintf(formatStr,args);

  va_end(args);

  char userChoice = ' ';
  while(userChoice != 'Y' && userChoice != 'N'){
    userChoice =  toupper(getchar());
  }

  if (userChoice == 'N')
    return false;
  return true;
  
}

/* calls stat to get filesize, returns -1 on error 
  fd: file descriptor
  returns: filesize, or -1 on error.
*/
int get_file_size(int fd) {
    struct stat st;
    if (fstat(fd, &st) == 0) {
        return st.st_size;
    } else {
        return -1;
    }
}

/* expands a buffer by multipling its size by the golden ratio 
    ptr: pointer to the buffer to expand
    currSize: pointer to the variable that stores the size of the buffer

    changes ptr and currSize and updates them to new values on success, and to NULL and 0 on failure

    returns true if realloc was successfull, returns false if not.

    original pointer is freed on error
*/
bool expand(void **ptr, size_t *currSize){
  if (ptr == NULL || currSize == NULL || *currSize == 0)
    return false;

  uint8_t *orig = *ptr;

  *currSize = *currSize * 1.618; /* expand heap by multiplying it by the golden ratio. I heard thats a good amount to expand a buffer by*/
  *ptr = realloc(*ptr,*currSize);

  if (*ptr == NULL){
    free(orig); orig = NULL;
    *currSize = 0;
    return false;
  }

  return true;
}
