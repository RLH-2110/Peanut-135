#ifndef INCLUDED_BLOCKMNT_H
#define INCLUDED_BLOCKMNT_H

extern char **roms;
extern size_t romCount;

/* finds roms, and creates a list of them into `roms` and upadtes `romsCount`*/
bool search_roms(void); 

typedef struct mount_list{
  char *device;
  char *mountPoint;
  struct mount_list *next;
} mount_list_t;

mount_list_t* get_mounted_partitions(void);
bool find_and_mount(void);

void free_mount_list(mount_list_t *list);

/* INCLUDED_BLOCKMNT_H */
#endif
