/* standalone file, only meant to be included in tests.c*/

#include "../peanut.h"
#include "../rom.h"

#define PUTS_RETURN(testNum,str) {printf("Test %d failed: %s\n",testNum, str); return false;}

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>

bool test_rom(int testNum) {
    const char* path = "/tmp/gb_rom_test.txt";
#define size (64 * 1024)

    char buff[size];
    for (size_t i = 0; i < size; ++i)
      buff[i] = rand() % 256;

    if (size > SSIZE_MAX)
      PUTS_RETURN(testNum,"SSIZE_MAX");

    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0x0606);
    if (fd < 0)
      PUTS_RETURN(testNum,"file io");

    size_t written = 0;

    while (written < size) {

        written += write(fd, (char*)(buff + written),size - written);

        if (written < 0) {
            perror("write");
            close(fd);
            return 1;
        }

    }

    close(fd);

  if (load_rom_file(path) == false)
    PUTS_RETURN(testNum,"load_rom_file error");

  if (romData == NULL)
    PUTS_RETURN(testNum,"romData NULL");
    
  if (romSize != size)
    PUTS_RETURN(testNum,"romSize incorrect");

  for (int i = 0; i < size; i++){
    if (buff[i] != romData[i])
      PUTS_RETURN(testNum, "romData incorrect!");
  }

  return true;
}
