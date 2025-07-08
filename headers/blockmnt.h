#ifndef INCLUDED_BLOCKMNT_H
#define INCLUDED_BLOCKMNT_H

extern char *roms;
extern size_t romsIndex;
extern size_t romsSize;

/* limits */
#define MAX_SCSI_DEVICES
#define MAX_PARTITIONS 9

/* finds roms, and creates a list of them into `roms` and upadtes `romsCount`*/
bool search_roms(char* customSearchPath, bool searchExternal, bool searchHome);

typedef struct mount_list{
  char *device;
  char *mountPoint;
  struct mount_list *next;
} mount_list_t;

mount_list_t* get_mounted_partitions(void);
bool find_and_mount(void);

void free_mount_list(mount_list_t *list);

/*gets first rom name saved in roms, or NULL */
char* get_first_from_roms(void);

/*gets amount of roms saved in roms */
unsigned int get_roms_count(void);

/* gets mount point from partition path. so for /dev/sda1 is might find /mnt/foo  
    takes in paths like /dev/sda1 or /dev/sdb8 
    returns: NULL if not found, or mounth path, if found
*/
char* find_mount_point(char* partitionPath);

/* scans a directory and finds all relevant files.
    filters out '.' and '..'
    if onlyGb is set, filters out files that are not .gb!

   path: path to search
   heapBuffer: a buffer allocated on the heap. The found files will be put here. (will be read and written to)
   heapBuffSize: the currentSize of the heap buffer (will be read and written to)
   heapBuffIndex: The current index of the heap buffer (where to start writing) (will be read and written to)
   onlyGb: shows only .gb files if set, otherwhise it will show all files 
   excludeDirs: if set, excludes directories from the search
 */
void scan_path(char* path, char* heapBuffer, size_t *heapBuffSize, size_t *heapBuffIndex, bool onlyGb, bool excludeDirs, bool prefixPath);

/* INCLUDED_BLOCKMNT_H */
#endif
