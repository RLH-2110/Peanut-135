# Peanut-135

Peanut-135 is a Game Boy Emulator for the STM32MPU135F-DK, based on [Peanut-GB](https://github.com/deltabeard/Peanut-GB) by [deltabeard](https://github.com/deltabeard)

# Installation

This is meant to work with Yocto, The next push will include a .bb file that you can move a directory up from the GitHub project.
Then just add the package to your image. Read the Yocto or Bitbake documentation if you need help.

If you make an own Recipe, then don't forget to pass `all` to the Makefile, I remember having issues when not doing so.

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

# Dependencies and 3rd party

Dependencies (stored in .gitmodules):  
[iniparser](https://gitlab.com/iniparser/iniparser.git)  
  
3rd Party code (directly integrated):
[DRM Doc](https://github.com/ascent12/drm_doc)

# Footnotes

[^1]: There seems to be a bug there one line is not used, so this might be 320 X 271


