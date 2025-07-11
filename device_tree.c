/* this is not a standalone file, it is only meant to be directly included in main.c */

#include "headers/main.h"
#include "headers/util.h"

#define EXPECTED_DEVICE_TREE "f0ec23626b4aaa7f1d734db9fc54f73b"

#define IS_RIGHT_DEVICE_TREE_ERROR -1
#define IS_RIGHT_DEVICE_TREE_TRUE   1
#define IS_RIGHT_DEVICE_TREE_FALSE  0

/* check if we have the right device tree 

  returns 1 : its the correct device tree
          0 : its the wrong device tree
          -1: an error occured
*/
int is_right_device_tree(void){

  char hash[sizeof(EXPECTED_DEVICE_TREE)]; 
  FILE* fd = popen("md5sum /boot/stm32mp135f-dk.dtb","r");

  if (fd == NULL){
    puts("Error: could not fopen md5sum  /boot/stm32mp135f-dk.dtb");
    return -1; 
  }
  LOGR("Open: DEVICE_TREE_HASH",1);

  if (fread(hash, 1, 32, fd) != 32) {
    puts("Error: Failed to read full hash\n");

    if (pclose(fd) == -1)
      puts("Error: Could not close DEVICE_TREE_HASH");
    else
      LOGR("Clean: DEVICE_TREE_HASH",-1);

    return -1;
  }

  if (pclose(fd) == -1)
    puts("Error: Could not close DEVICE_TREE_HASH");
  else
    LOGR("Clean: DEVICE_TREE_HASH",-1);

  hash[sizeof(hash) - 1] = '\0';

  if (strcmp(hash,EXPECTED_DEVICE_TREE) == 0){
    puts("found correct device tree!");
    return 1;
  }

  return 0;
}


/* forces the right device tree to be present */
bool force_device_tree(void){

#if FULL_CONTROLL == 0
  return false; /* nah, stop early */
#endif

  puts("Checking device tree...");

  /* use md5sum to see if we have the correct Device Tree */
  int ret =  is_right_device_tree();
  if (ret == IS_RIGHT_DEVICE_TREE_ERROR)
    return false;
  if (ret == IS_RIGHT_DEVICE_TREE_TRUE){
    puts("Correct Device tree found!");
    return true;
  }

  /* wrong device tree */
  bool used_fallback = false;

  if ( is_file("/boot/swap.sh", -1 )){ /* do we have swap.sh installed to swap the dtb files? */ 

    puts("/boot/swap.sh");
    system("/boot/swap.sh"); 

  }else if (is_file("/usr/bin/swap.sh", -1)){ /* okay no, but what about the installer? */
    
    puts("/usr/bin/dts_install.sh");
    system("/usr/bin/dts_install.sh");

    if (is_file("/boot/swap.sh", -1 )){
      puts("/boot/swap.sh");
      system("/boot/swap.sh"); // huzza, the installer worked
    }
    else
      goto device_tree_swap_fallback; // it did not work :(
    
  }else{  /* swap.sh is not installed, but we have our internal stuff */

device_tree_swap_fallback:
    used_fallback = true;
    /* just assume they work, we are suppoed to be root. we will see if we failed later */
    puts("mv /boot/stm32mp135f-dk.dtb /boot/stm32mp135f-dk.dtb.peanut135_renamed_this");
    system("mv /boot/stm32mp135f-dk.dtb /boot/stm32mp135f-dk.dtb.peanut135_renamed_this");
    puts("cp /etc/peanut135/stm32mp135f-dk.dtb /boot/stm32mp135f-dk.dtb");
    system("cp /etc/peanut135/stm32mp135f-dk.dtb /boot/stm32mp135f-dk.dtb");
  }

  ret =  is_right_device_tree();
  if (ret == IS_RIGHT_DEVICE_TREE_TRUE){
    // now apply the new device tree
    puts("New device tree is in use, restarting to apply it...");
    sleep(2); /*2 seconds*/
    system("reboot");

    // and exit
    stop = 1;
    return true;
  }

  if (used_fallback == false){
    puts("Could not get the rigth device tree... using a fallback method now...");
    goto device_tree_swap_fallback;
  }

  puts("Error: Could not get right device tree!");
  return false;
}
