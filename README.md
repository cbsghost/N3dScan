# N3dScan
N3dScan is a open 3D Scanner firmware for Lego Mindstorm NXT device. The core of the firmware is based on nxtOSEK, consisting of device driver of leJOS NXJ C/Assembly source code, TOPPERS/ATK (Automotive Kernel, formerly known as TOPPERS/OSEK) and TOPPERS/JSP Real-Time Operating System source code that includes ARM7 (ATMEL AT91SAM7S256) specific porting part, and glue code to make them work together.

This version is one of my independent studies. The code may not easy to understand.

### Demo Reel
There is a demo video for the project.

[![NXT 3D Scanner (N3dScan) - Demo Reel](http://img.youtube.com/vi/g6dGGE-Eptw/0.jpg)](http://www.youtube.com/watch?v=g6dGGE-Eptw "NXT 3D Scanner (N3dScan) - Demo Reel")

### How to use
You need to setup the [nxtOSEK SDK][nxtOSEK_download] first, then modify the last line of the Makefile in the firmware directory to link to the SDK.

I only run the firmware on top of NXT BIOS, so flashing the firmware replacement is highly recommended. You can find the instruction [here][nxtOSEK_upload].

### Version
0.0.1

### Tech
N3dScan uses some open source projects to work properly:

* [nxtOSEK] - ANSI C/C++ with OSEK/μITRON RTOS for LEGO MINDSTORMS NXT.

### Development
Want to contribute? Great!
Contact me if you having problems of the projects.

### Todos
 - Rewrite the code to module-by-module.
 - Optimized the stablility of each task, especially GUI.
 - Find out the crash problem during booting while pressing the buttons.
 - Figure out the Bluetooth bug in the nxtOSEK core.

License
----

GPLv3

[//]: #link
   [nxtOSEK]: <http://lejos-osek.sourceforge.net>
   [nxtOSEK_download]: <http://lejos-osek.sourceforge.net/download.htm?group_id=196690>
   [nxtOSEK_upload]: <http://lejos-osek.sourceforge.net/howtoupload.htm#UploadtoFlash>
