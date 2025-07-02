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


#include "main.h"
#include "util.h"
#include "blockmnt.h"

#define DEBUG_BLOCKMNT 0

/* finds roms, and creates a list of them into `roms` and upadtes `romsCount`*/
bool search_roms(void){
  if (find_and_mount() == false)
    return false;

  /* scan custom path*/

  /* scan mounted stuff */

  mount_list_t *mountedSCSI = get_mounted_partitions();

  free_mount_list(mount_list_t);

  /* scan home */

  roms;
  romCount;


  return true;
}

/* -------------------------------------------------------------- */

/* gets file system string for a partion path 
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
  struct stat st;
  char devicePath[] = "/sys/block/sda\0sda1";
  char *pathSeperator = &devicePath[strlen(devicePath)]; /* positon on where the path seperator will be*/
  char *deviceLetter = &devicePath[strlen(devicePath)-1]; /* this is the 'a' in sda */
  char *partionNum = &devicePath[strlen(devicePath) + strlen(pathSeperator+1)]; /* this is the '1' in sda1 */
  char *secondDeviceLetter = partionNum - 1; /* this is the 'a' in the second sda */


  char devPath[] = "/dev/sda1";
  char *devPartitonNum = &devPath[strlen(devPath)-1];
  char *devDeviceLetter = devPartitonNum - 1;


  mount_list_t *alreadyMounted = get_mounted_partitions();

  puts("done, doing other sh1t");

  /* find all external partitions */
  for (;*deviceLetter <= 'z';(*deviceLetter)++){

    if (stat(devicePath, &st) != 0 || S_ISDIR(st.st_mode) == false){ /* is not a dir */
      continue;
    }

    *devDeviceLetter = *deviceLetter;
    *secondDeviceLetter = *deviceLetter;
    *pathSeperator = '/';

    for (*partionNum = '1'; *partionNum <= '9'; (*partionNum)++){

      /* check if exists */
      if (stat(devicePath, &st) != 0 || S_ISDIR(st.st_mode) == false){ /* is not a dir */
        break;
      }

      /* check if already mounted */
      bool isMounted = false;
      for (mount_list_t *current = alreadyMounted; current != NULL; current = current->next){
        if (strcmp(devicePath,current->device) == 0){
          isMounted = true;
          break;
        }
      }
      if (isMounted)
        continue;

      if (sizeof(devicePath) + heapIndex >= heapSize){ /* expand memory, if we have too little */
        if (EXPAND(heap,heapSize) == false){
          puts("Not enogh free Memory!");
          LOGR("Clean: MOUNT HEAP",-1);
          return false;
        }
      }
      
      *devPartitonNum = *partionNum;

      strcpy((uint8_t*)(heap+heapIndex),devPath); /* add to list of partitions to mount */
      heapIndex += sizeof(devPath);

    } /* end partion for loop */

    *pathSeperator = '\0';
  } /* end device letter loop */


  /* we not have a list of strings in heap, which ends at heapIndex.
    each string tells us a partition to mount
  */

  puts("\n\nfound unmounted SCSI block Devices:");
  for (int i = 0; i < heapIndex; i++){
    if (heap[i] == '\0')
      putchar('\n');
    else
      putchar(heap[i]);
  }
  puts("\n\n");

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

    if (stat(mountPoint, &st) == 0 || S_ISDIR(st.st_mode) == true){
      
      char* fs = get_partition_filesystem(HEAP_STRING);
      if (fs != NULL){
        if (mount(HEAP_STRING, mountPoint, fs, 0, NULL) != 0){
          printf("Error: can not mount %s to %s as %s ",HEAP_STRING,mountPoint,fs);
          perror("Errno");
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
