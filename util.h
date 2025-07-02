#ifndef INCLUDED_UTIL_H
#define INCLUDED_UTIL_H

#define NOT_IMPLEMENTED puts("THIS IS NOT IMPLEMENTED!");

#define BYTES_TO_MEGABYTES(bytes) (bytes / (1024 * 1024))

#include <stdbool.h>
#include <stdarg.h>

/* calls pritnf with the given parameters, and then gives the user the choice of yes or no.
  parameters: same as printf
  returns: true if user selected Y
           false if user selected N
*/
bool choice(const char * const formatStr, ...);

/* calls stat to get filesize, returns -1 on error 
  fd: file descriptor
  returns: filesize, or -1 on error.
*/
int get_file_size(int fd);

/* expands a buffer by multipling its size by the golden ratio 
    ptr: pointer to the buffer to expand
    currSize: pointer to the variable that stores the size of the buffer

    changes ptr and currSize and updates them to new values on success, and to NULL and 0 on failure

    returns true if realloc was successfull, returns false if not.

    original pointer is freed on error
*/
bool expand(void **ptr, size_t *currSize);
/* same as expand, but it takes the direct types insted of pointers to them, it does this all automatically*/
#define EXPAND(ptr, size) expand((void **)(&ptr), (&size)) 

/* INCLUDED_UTIL_H */
#endif
