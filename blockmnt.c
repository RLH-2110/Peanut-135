#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include <blkid/blkid.h>
#include <sys/mount.h>
#include <dirent.h>


#include "main.h"
#include "util.h"
#include "blockmnt.h"

#define DEBUG_BLOCKMNT 0

/* returns the frist rom in roms or NULL */
char* get_first_from_roms(void){
  if (roms == NULL)
    return NULL;
  
  if (romsIndex == 0)
    return NULL;

  return roms;
}

/* -------------------------------------------------------------- */

/*gets the amount of roms in roms */
unsigned int get_roms_count(void){
  if (roms == NULL){
    puts("Error: ROMS is NULL, cant get ROM count!");
    return 0;
  }
  if (romsIndex == 0)
    return 0;

  size_t localRomsIndex = 0;
  unsigned int count = 0;

  while(localRomsIndex < romsIndex){
    count++;
    printf("found rom: %s\n",roms + localRomsIndex);
    localRomsIndex += strlen(roms + localRomsIndex) + 1;
  }

  return count;
}

/* -------------------------------------------------------------- */

/* modifies heapBuffer, heapBuffSize and heapBuffIndex! */
void scan_path(char* path, char* heapBuffer, size_t *heapBuffSize, size_t *heapBuffIndex, bool onlyGb, bool excludeDirs, bool prefixPath ){
  if (path == NULL || heapBuffer == NULL || heapBuffSize == NULL || heapBuffIndex == NULL)
    return;

#if DEBUG_BLOCKMNT 
  printf("scanning %s\n",path);
#endif

  DIR *dir = opendir(path);
  if (dir == NULL){
    printf("could not open directory %s\n",path);
    return;
  }
  LOGR("Open: Directory",1);
  
  /* search all file entries for .gb files */
  struct dirent *dirEntry;
  while(true){
    dirEntry = readdir(dir);
    if (dirEntry == NULL)
      break;

    size_t len = strlen(dirEntry->d_name);

    if (strcmp (dirEntry->d_name, ".") == 0) /* filter out . */
      continue;
    if (strcmp (dirEntry->d_name, "..") == 0) /* filter out .. */
      continue;


    if (excludeDirs && is_dir_path(path,dirEntry->d_name)) /* exclude dirs, if nessesary */
      continue;

    if (onlyGb && is_dir_path(path,dirEntry->d_name) == false){ /* if its a directory skip the check. if its not a directory, then only do this is the filter for only gb files */
      if (len < sizeof(".gb") || strcmp (dirEntry->d_name + len - strlen(".gb"), ".gb") != 0){  /*check of the last 3 chars are ".gb" */
        continue; /* not a gb file */
      }

      /* a gb file! */
    }
 
    if (prefixPath)
      len += strlen(path);

    /* check if buffer is big enough */
    if (*heapBuffSize - *heapBuffIndex < len + 1 ){
      if (EXPAND(heapBuffer,*heapBuffSize) == false){
        puts("Not enogh free Memory!");
        *heapBuffSize = 0; 
        LOGR("Clean: ROMS",-1);
        return;
      }
    } 

    if (prefixPath)
      path_construct(heapBuffer + *heapBuffIndex,*heapBuffSize - *heapBuffIndex, path, dirEntry->d_name); // save the full path
    else
      strcpy(heapBuffer + *heapBuffIndex,dirEntry->d_name); // save filename only 

    *heapBuffIndex += strlen(heapBuffer + *heapBuffIndex) + 1;
  }


  if(closedir(dir) != 0)
    perror("could not close directory! Errno");
  else
    LOGR("Clean: Directory",-1);
}

/* -------------------------------------------------------------- */

/* finds roms, and creates a list of them into `roms` and upadtes `romsSize` and `romsIndex` */
bool search_roms(char* customSearchPath){

  romsSize = 0xFF;
  romsIndex = 0;
  roms = malloc(romsSize);
  if (roms == NULL){
    puts("Not enough memory!");
    return false;
  }
  LOGR("Alloc: ROMS",1);

  if (find_and_mount() == false){
    free(roms); roms = NULL; romsSize = 0;
    LOGR("Clean: ROMS",-1);
    return false;
  }


  /* scan custom path*/
  if (customSearchPath != NULL){
    scan_path(customSearchPath,roms,&romsSize, &romsIndex, true,true,true);
  }  

  /* scan mounted stuff */

  mount_list_t *mountedSCSI = get_mounted_partitions();
  mount_list_t *current = mountedSCSI;

  while(current != NULL){
    scan_path(current->mountPoint,roms,&romsSize, &romsIndex, true, true,true);
    current = current->next;
  }
  free_mount_list(mountedSCSI);

  /* scan home */
  char *home = getenv("HOME");
  if (home == NULL)
    puts("Warning: Can't find home directory!");
  else
    scan_path(home,roms,&romsSize, &romsIndex, true, true, true);

  printf("--\nromsIndex: %d\nromsSize: %d\nroms:\n",romsIndex,romsSize);
  for (int i = 0; i < romsIndex; i++){
    if (roms[i] == '\0')
      printf("\n");
    else
      putchar(roms[i]);
  }
  puts("--");

  return true;
}

/* -------------------------------------------------------------- */

/* gets file system string for a partion path (like "vfat") 
    devicePath: a path like /dev/sda1
    returns: pointer to internal static buffer, that will be overwritten with each call to this function.
             or returns NULL on error.
*/
char* get_partition_filesystem(char* devicePath){

  static char fileSystem[32];
  blkid_probe probe = 0;
  int ret;
  char const *type;

  probe = blkid_new_probe_from_filename(devicePath);
  if (probe == 0){
    printf("Error: Could not create probe for %s\n",devicePath);
    return NULL;
  }

  ret = blkid_do_probe(probe);
  if (ret != 0) {
    printf("Error: Could not probe %s\n",devicePath);
    return NULL;
  }

  ret = blkid_probe_lookup_value(probe,"TYPE",&type, NULL);
  if (ret != 0 || type == NULL){
    printf("Error: Could look up TYPE for %s\n",devicePath);
    return NULL;
  }

  if (strlen(type) > sizeof(fileSystem)){
    printf("Error: %s is too big to fit in the %s byte buffer!",type,sizeof(fileSystem));
    return NULL;
  }
  
  strcpy(fileSystem,type);  
  return fileSystem;
}

/* -------------------------------------------------------------- */
bool find_and_mount(void){
  size_t heapSize = 1024;
  size_t heapIndex = 0;
  uint8_t *heap = malloc(heapSize);
  if (heap == NULL){
    puts("Not enogh free Memory!");
    return false;
  }
  LOGR("Alloc: MOUNT HEAP",1);

  /* see img/code/devicePath.png for a visual explenation */
  char devicePath[] = "/sys/block/sda\0sda1";
  char *pathSeperator = &devicePath[strlen(devicePath)]; /* positon on where the path seperator will be*/
  char *deviceLetter = &devicePath[strlen(devicePath)-1]; /* this is the 'a' in sda */
  char *partionNum = &devicePath[strlen(devicePath) + strlen(pathSeperator+1)]; /* this is the '1' in sda1 */
  char *secondDeviceLetter = partionNum - 1; /* this is the 'a' in the second sda */


  char devPath[] = "/dev/sda1";
  char *devPartitonNum = &devPath[strlen(devPath)-1];
  char *devDeviceLetter = devPartitonNum - 1;


  mount_list_t *alreadyMounted = get_mounted_partitions();

  puts("finished getting the mount list");

  /* find all external partitions */
  for (;*deviceLetter <= 'z';(*deviceLetter)++){

    if (is_dir(devicePath,-1) == false){ /* is not a dir */
      continue;
    }

    *devDeviceLetter = *deviceLetter;
    *secondDeviceLetter = *deviceLetter;
    *pathSeperator = '/';

    for (*partionNum = '1'; *partionNum <= '9'; (*partionNum)++){

      *devPartitonNum = *partionNum;

      /* check if exists */
      if (is_dir(devicePath, -1)  == false){ /* is not a dir */
        break;
      }

      /* check if already mounted */
      bool isMounted = false;
      for (mount_list_t *current = alreadyMounted; current != NULL; current = current->next){
#if DEBUG_BLOCKMNT
        printf("comparing %s and %s to see if its already mounted\n",devPath,current->device);
#endif
        if (strcmp(devPath,current->device) == 0){
#if DEBUG_BLOCKMNT
          puts("alredy mounted");
#endif
          isMounted = true;
          break;
        }
      }
      if (isMounted)
        continue;
#if DEBUG_BLOCKMNT
      puts("not mounted");
#endif

      if (sizeof(devicePath) + heapIndex >= heapSize){ /* expand memory, if we have too little */
        if (EXPAND(heap,heapSize) == false){
          puts("Not enogh free Memory!");
          LOGR("Clean: MOUNT HEAP",-1);
          return false;
        }
      }

      strcpy((uint8_t*)(heap+heapIndex),devPath); /* add to list of partitions to mount */
      heapIndex += sizeof(devPath);

    } /* end partion for loop */

    *pathSeperator = '\0';
  } /* end device letter loop */


  /* we not have a list of strings in heap, which ends at heapIndex.
    each string tells us a partition to mount
  */

  if (heapIndex != 0) /* only print string, if we found any */
    puts("\nfound unmounted SCSI block Devices:");

  for (int i = 0; i < heapIndex; i++){
    if (heap[i] == '\0')
      putchar('\n');
    else
      putchar(heap[i]);
  }
  puts("\n");

  /* create mount directories and mount */

#define HEAP_STRING ((char*)heap + localHeapIndex)
  size_t localHeapIndex = 0;
  while (localHeapIndex < heapIndex){
#if DEBUG_BLOCKMNT
    printf("ITTERATION:\n\theap str: %s\n\tlocalHeapIndex: %d\n\theapIndex: %d\n",HEAP_STRING,localHeapIndex,heapIndex);
#endif
    char* mountPoint = strdup(HEAP_STRING); 
    if (mountPoint == NULL){
      printf("Error: strdup failure!\n\t%s\n",HEAP_STRING);
      localHeapIndex += strlen(HEAP_STRING) + 1;
      continue;
    }

    memcpy(mountPoint,"/mnt",strlen("/mnt"));
    
    if (mkdir(mountPoint, 0x777) != 0 && errno != EEXIST) /* create dir, and ignore if the error is that it already exists */
      printf("Error: could not create mount dir %s\n",mountPoint);

    if (is_dir(mountPoint, -1) == true){ 
      
      char* fs = get_partition_filesystem(HEAP_STRING);
      if (fs != NULL){
        if (mount(HEAP_STRING, mountPoint, fs, 0, NULL) != 0){
          printf("Error: can not mount %s to %s as %s\n",HEAP_STRING,mountPoint,fs);
          perror("\tErrno");
        }
      }
      
    }else{
      printf("Error: can not mount %s into %s, because %s id not a directory!\n",HEAP_STRING,mountPoint,mountPoint);
    } 

    free(mountPoint);
    localHeapIndex += strlen(HEAP_STRING) + 1;
  }

  free_mount_list(alreadyMounted);

 
  LOGR("Clean: MOUNT HEAP",-1);
  free (heap); heap = NULL;
  return true;
}


/* -------------------------------------------------------------- */

/* returns a mount_list with all mounted SCSI partitions, or NULL on error */
mount_list_t* get_mounted_partitions(void){

  mount_list_t* previous = NULL;
  mount_list_t* current = NULL;
  mount_list_t* head = NULL;
#define CHAR_PTR_CURRENT ((char*)current)

#define GMP_BUFF_SIZE 256
  char buff[GMP_BUFF_SIZE];

  FILE *fd = fopen("/proc/mounts","r");
  if (fd < 0){
    puts("Error: Can not fopen /proc/mounts");
    return NULL;
  }
  LOGR("Open: /PROC/MOUNTS",1);

  int entriesRead = 0;
  while(true){

    if (feof(fd))
      break;

    if (fgets(buff, GMP_BUFF_SIZE, fd) == NULL){
      buff[0] = '\0';
      printf("Info: fgets read returned NULL!\n\tread %d entries\n",entriesRead);
      break;
    }
    entriesRead++;
    //printf("(DEBUG): Entry %d: %s\n",entriesRead,buff);

    /* from now out filter out everything that cant be a SCSI disk device */
    
    size_t buffContentSize = strlen(buff);
    if (strlen(buff) < strlen("/dev/sda1 /")) /* if the buffer is to small to hold any SCSI disk devices */
      continue;

  #define GML_BUFF_SPACE_LOCATION strlen("/dev/sda1")
    if (buff[GML_BUFF_SPACE_LOCATION] != ' ') /* if the space character is not where we expect it for SCSI disk devices */
      continue; 

    buff[GML_BUFF_SPACE_LOCATION] = '\0';

    char comparisionBuff[] = "/dev/sda1";
    off_t partitionNumOff = strlen(comparisionBuff)-1; 
    off_t deviceLetterOff = partitionNumOff - 1;

    if (!isdigit(buff[partitionNumOff]))
      continue;
    if (!isalpha(buff[deviceLetterOff]))
      continue;

    comparisionBuff[partitionNumOff] = buff[partitionNumOff];
    comparisionBuff[deviceLetterOff] = buff[deviceLetterOff];

    if (strcmp(buff,comparisionBuff) != 0)
      continue;


    /* we now know that we have a SCSI disk devices */ 

#define GML_SECOND_STR (&buff[GML_BUFF_SPACE_LOCATION + 1])
    off_t mountPointEnd = 0; 
    for (int base = GML_BUFF_SPACE_LOCATION + 1; base < buffContentSize; base++){

      if(buff[base] == ' '){
        buff[base] = '\0';
        break;
      }
      mountPointEnd++;
    }

    /* allocate space for the sturct, and the 2 strings in it*/
    size_t mallocSize = sizeof(mount_list_t) + strlen(buff) + 1 + strlen(GML_SECOND_STR) + 1; 
    current = malloc(mallocSize);
    if ( current == NULL ){
      puts("Error: out of memory!");
      goto get_mounted_partitions_cleanup;
    }
    LOGR("Alloc: MOUNT_LIST ELEMENT",1);

    /* pointers to the string that we allocated with the struct */
    char *mountListDevice = (char*)(CHAR_PTR_CURRENT + sizeof(mount_list_t));
    char *mountListMountPoint = (char*)(CHAR_PTR_CURRENT + sizeof(mount_list_t) + strlen(buff) + 1);

    /* copy from the tempory buffer to the allocated strings */
    strcpy(mountListDevice,buff);
    strcpy(mountListMountPoint,GML_SECOND_STR);

    current->device = mountListDevice;
    current->mountPoint = mountListMountPoint;
    current->next = NULL;
    if(previous != NULL)
      previous->next = current;
    
    previous = current;

    if (head == NULL)
      head = current;
 
    /* debug prints */
#if DEBUG_BLOCKMNT
    printf("debug test:\n\tDevice: %s\n\tmountPoint: %s\n",current->device, current->mountPoint);
    printf("\tall: ");
    for (int i = 0; i < mallocSize;i++)
      if (*(((char*)current)+i) == '\0')
        printf("\\0");
      else
        printf("%c", *(((char*)current)+i));
    puts("");
#endif

  }


  if (fclose(fd) != 0)
    puts("Error: could not close /proc/mounts");
  else {
    LOGR("Clean: /PROC/MOUNTS",-1);
  }

  return head;

/* only reachable with goto, for cleanup*/
get_mounted_partitions_cleanup:
  if (head != NULL){
    free_mount_list(head); head = NULL;
    LOGR("Clean: PARTITION LIST",-1);
  }

  if (fclose(fd) != 0)
    puts("Error: could not close /proc/mounts");
  else{
    LOGR("Clean: /PROC/MOUNTS",-1); 
  }

}


/* -------------------------------------------------------------- */

void free_mount_list(mount_list_t *list){ 

  while(list != NULL){

    mount_list_t *next = list->next; 
    free(list); /* free arena */
    list = next;

    LOGR("Clean: MOUNT_LIST ELEMENT",-1);
  }
}


/* -------------------------------------------------------------- */

/* gets mount point from partition path. so for /dev/sda1 is might find /mnt/foo  */
/* taks in paths like /dev/sda1 or /dev/sdb8 */
/* returns: NULL if not found, or mounth path, if found*/
char* find_mount_point(char* partitionPath){
  mount_list_t *mountedSCSI = get_mounted_partitions();
  mount_list_t *current = mountedSCSI;
  
  char *found = NULL;

  while(current != NULL){
    if (strcmp(partitionPath, current->device) == 0){
      found = current->mountPoint;
      break;
    }
    current = current->next;
  }

  free_mount_list(mountedSCSI);
  return found;
}
