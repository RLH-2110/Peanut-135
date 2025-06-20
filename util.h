#ifndef INCLUDED_UTIL_H
#define INCLUDED_UTIL_H

#define NOT_IMPLEMENTED puts("THIS IS NOT IMPLEMENTED!");

#define BYTES_TO_MEGABYTES(bytes) (bytes / (1024 * 1024))


/* calls pritnf with the given parameters, and then gives the user the choice of yes or no.
  parameters: same as printf
  returns: true if user selected Y
           false if user selected N
*/
bool choice(const char * const formatStr, ...);

/* INCLUDED_UTIL_H */
#endif
