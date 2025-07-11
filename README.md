# Peanut-135

Peanut-135 is a Game Boy Emulator for the STM32MPU135F-DK, based on [Peanut-GB](https://github.com/deltabeard/Peanut-GB) by [deltabeard](https://github.com/deltabeard)  
It is assumed that this Program runs on [**OpenSTLinux**](https://wiki.st.com/stm32mpu/wiki/STM32MPU_Distribution_Package#Installing_the_OpenSTLinux_distribution), this program may fail if it runs anywhere else.

> [!Warning]
> Opening this program with a Weston terminal will result in Weston and the Program being terminated. You may currently only launch this programm via a SystemD, Serial Connection, SSH, or any other Non-Weston method

# Limitations

This emulator can mount SCSI devices for you, but only when it starts, it does not hot plug them!  
This Emulator only supports SDA-SDZ and only mounts the first 9 partitions.

> [!Note]
> See if I forgot some

# Installation

This is meant to work with Yocto, This project includes a .bb file that you can move a directory up from the GitHub project.
Then just add the package to your image. Read the Yocto or Bitbake documentation if you need help.

If you make an own Recipe, then don't forget to pass `all` to the Makefile, I remember having issues when not doing so.

# Compile time Settings

The Most important setting is `FULL_CONTROLL` in headers/main.h, if set to 1, the Emulator will assume it's the only thing that needs to run. You won't be able to exit it normally any more, instead it shuts d    own. It will also try to load a specific Device tree that it would like to have to get access to BTN_2.
> [!Note]
> The loaded device tree will disable internet! This is because I reused this device tree from another project, where this was a side Effect. This will not be fixed, as I will not be able to update/fix this soon, because I only have this hardware temporarily.


headers/main.h LOG_RESOURCES - if set to 1, it will log "Resource" usage. Where Resource is anything that had been done, that needs to be undone (like free/malloc open/close, ...)  
blockmnt.c DEBUG_BLOCKMNT - if set to 1, it will print debug information for functions in this file
input.c DEBUG_INPUTS - if set to 1 or higher, it will print debug information for functions in this and included files. There are multiple levels here, from 0 to 4  


> [!Note]
> This section is TODO

# Exiting the Emulator

If FULL_CONTROLL is set to 0, you can exit the emulator by pressing BTN_2 (can be remapped) and selecting exit in the file menu, or you can press CTRL+C.
If FULL_CONTROLL is set to 1, you can put these commands into the console: 
`top | grep peanut135` - to get the PID
`kill -2 [PID]` - to kill the emulator
`systemctl disable peanut135.service` - (Optional) Disable autostarting the emulator

If CTRL+C is pressed a total of 3 times or more, then the program will forcefully exit, without trying to clean up.

# Controls

This emulator assumes you are using OpenSTLinux, and that BTN_1 is `User button 2`. (This can be remapped, and should BTN_2 be set in the Device tree, it will be used too)  

By default, BTN_1 is mapped to `A`, and BTN_2 is mapped to `B`.  
On the OpenSTLinux that I use for the STM32MPU135F-DK only one of the user buttons works by default, it is the lower button next to the LCD.  
Since there are only 2 user buttons, where only one is usable by default, the other controls are handled via the touchscreen.  

![Image: Touchscreen controls](img/touchscreen%20button%20map.png)

Here again in table form:

| Touchscreen Region | Mapped Button |
|--------------------|---------------|
| Left / Top Left    | Left          |
| Top Centre         | Up            |
| Right / Top Right  | Right         |
| Centre             | B             |
| Bottom Left        | Select        |
| Bottom Centre      | Down          |
| Bottom Right       | Start         |

The controls can be remapped at compile time in: [button_map.h](headers/button_map.h)

# Configuration File

This emulator looks for a configuration file in `/.config/peanut135/config.ini` and `/etc/peanut135/config.ini`.
The config in the `.config` directory takes Precedence over the one in `etc`.

There are multiple settings, like if the Emulator should start automatically if there is only one ROM found, and configurations about where the Emulator should look for ROMs.  
There are also multiple display settings:
| Setting | Resolution    | Caveat                                                              |
|---------|---------------|---------------------------------------------------------------------|
| Default | 160 x 144     |                                    -                                |
| Wide    | 320 x 144     |                                    -                                |
| Full_y  | 320 x 272[^1] | The first and last 8 lines are displayed at the original resolution |
| Cut_y   | 320 x 272     | The first and last 4 lines are cut off                              |

# Restoring the old device Tree

If FULL_CONTROLL is 1, then the device tree may have been replaced;
to restore the original device tree, either run swap.sh (if present) or delete `/boot/stm32mp135f-dk.dtb` and rename `/boot/stm32mp135f-dk.dtb.peanut135_renamed_this` to `/boot/stm32mp135f-dk.dtb`
then just reboot (make sure you disabled peanut135 in systemctl, else it will just undo your change again)

# Dependencies and 3rd party

Dependencies (stored on the local machine)  
libdrm, util-linux

Dependencies (stored in .gitmodules):  
[iniparser](https://gitlab.com/iniparser/iniparser.git)  
  
3rd Party code (directly integrated):  
[DRM Doc](https://github.com/ascent12/drm_doc)

# FAQ

### Q: After I exited the Emulator, Weston did not restart.

A: restart the emulator and close it again, or try to run `mkdir -p $XDG_RUNTIME_DIR`and  `weston &` It seems to be rare that it fails to restart, but trying it again seems to fix it.

# Footnotes

[^1]: There seems to be a bug there one line is not used, so this might be 320 X 271


