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

/* checks if a path or file descripor is a directory
  
  path: if its a Non-NULL value, then is_dir checks if the path is a dir
  fd: ignroed when path is not NULL. if the path is NULL, then we check if the fd is a directory.

  returns: false if stat failed, or if its not a directory
           true if its a directory

*/
bool is_dir(char const * const path, int fd);

/* checks if a file in a path is a dir. differs from is_dir, because this one builds the path. 
  
  errno is cleared if the function encounters no errors. 

  path: path leading to the file
  file: the file in the path

  returns: false if path or file are null, or if the combined path is bigger than UTIL_MAX_PATH_SIZE or if its not a directoy
           true if its a directoy

  errno:
    EINVAL: path or file where NULL
    ENAMETOOLONG: path + file was too long
    
    Errno can also be set by the stat function here, see errno documentation of stat
*/
bool is_dir_path(char const * const path, char const * const file);

/* expands a buffer by multipling its size by the golden ratio 
    ptr: pointer to the buffer to expand
    currSize: pointer to the variable that stores the size of the buffer

    changes ptr and currSize and updates them to new values on success, and to NULL and 0 on failure

    returns true if realloc was successfull, returns false if not.

    original pointer is freed on error
*/

/* constucts the path of path + file into buff 

  buff: Buffer to write the new path to
  buffSize: Maximum bytes that can be written
  path: path to the file/dir
  file: the file/dir 

  returns true if no error occured, false if buff, path or file is NULL or if path+file is too long
*/
bool path_construct(char* buff, size_t buffSize, char const * const path, char const * const file);
 

bool expand(void **ptr, size_t *currSize);
/* same as expand, but it takes the direct types insted of pointers to them, it does this all automatically*/
#define EXPAND(ptr, size) expand((void **)(&ptr), (&size)) 

/* INCLUDED_UTIL_H */
#endif
