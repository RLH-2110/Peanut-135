#include <stdarg.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>

#define UTIL_MAX_PATH_SIZE (512)

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

/* checks if a path or file descripor is a directory
  
  clears errno, if there was no error.

  path: if its a Non-NULL value, then is_dir checks if the path is a dir
  fd: ignroed when path is not NULL. if the path is NULL, then we check if the fd is a directory.

  returns: false if stat failed, or if its not a directory
           true if its a directory

  errno:
    EINVAL: if path is NULL and fd is -1
    
    Errno can also be set by the stat function here, see errno documentation of stat

*/
bool is_dir(char const * const path, int fd){
  struct stat st;

  if (path != NULL){
    if (stat(path, &st) != 0)
      return false;
  }
  else{ /* path == NULL*/
    if (fd == -1){
      puts("Error: fd can not be -1 in is_dir, if path is NULL!"); 
      errno = EINVAL;
      return false;
    }
    if (fstat(fd, &st) != 0)
      return false;
  }

  errno = 0;
  return S_ISDIR(st.st_mode);
}

/* constucts the path of path + file into buff 

  buff: Buffer to write the new path to
  buffSize: Maximum bytes that can be written
  path: path to the file/dir
  file: the file/dir 

  returns true if no error occured, false if buff, path or file is NULL or if path+file is too long
*/
bool path_construct(char* buff, size_t buffSize, char const * const path, char const * const file){
  
  if (buff == NULL || path == NULL || file == NULL)
    return false;  

  if (strlen(path) + strlen(file) + 2 >= buffSize) /* +2 because NULL terminator and space to add a missing "/" */ 
    return false;

  strcpy(buff,path);

  size_t buffLen = strlen(buff);
  if (buff[buffLen -1] != '/'){ /* if path does not end in "/", then append it */
    buff[buffLen + 0] = '/';
    buff[buffLen + 1] = '\0';
  }

  strcpy(buff+strlen(buff),file);

  return true; 
}

/* checks if a file in a path is a dir. differs from is_dir, because this one builds the path. 
  
  errno is cleared if the function encounters no errors. 

  path: path leading to the file
  file: the file in the path

  returns: false if path or file are null, or if the combined path is bigger than UTIL_MAX_PATH_SIZE or if its not a directoy
           true if its a directoy

  errno:
    EINVAL: path or file where NULL or path is an empty string
    ENAMETOOLONG: path + file was too long
    
    Errno can also be set by the stat function here, see errno documentation of stat
*/
bool is_dir_path(char const * const path, char const * const file){
  if (path == NULL || file == NULL  || path[0] == '\0'){
    errno = EINVAL;
    return false;
  }

//  if (strlen(path) + strlen(file) + 2 >= UTIL_MAX_PATH_SIZE){ /* +2 because NULL terminator and space to add a missing "/" */ 
/*    errno = ENAMETOOLONG;
    return false;
  }*/

  char buff[UTIL_MAX_PATH_SIZE];
  if (path_construct(buff,UTIL_MAX_PATH_SIZE,path,file) == false){
    errno = ENAMETOOLONG;
    return false;
  }

/*
  strcpy(buff,path);

  size_t buffLen = strlen(buff);
  if (buff[buffLen -1] != '/'){*/ /* if path does not end in "/", then append it */
/*    buff[buffLen + 0] = '/';
    buff[buffLen + 1] = '\0';
  }

  strcpy(buff+strlen(buff),file);
*/

  errno = 0;
  return is_dir(buff,-1);
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
