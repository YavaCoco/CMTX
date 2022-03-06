This program creates a virtual joystick, and sets Axis X to be an image of a physical controller's X rotation

Other buttons are simply mirrored from the physical controller to the virtual one

Usage: `./cmtx <motion_sensor_event_file> <joystick_event_file> [<uinput_file>]`

the motion sensor event file and joystick event file are likely to be in /dev/input/eventN

use `cat /proc/bus/input/devices` to get a full list of available devices, you'll find the event files there

The uinput file is set to /dev/uinput by default, and should probably stay like that in most systems

You may need to `modprobe uinput` before running this program, since that module is usually not inserted by default

You probably also need to run the program as root