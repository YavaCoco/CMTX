#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/joystick.h>
#include <linux/uinput.h>

/*
    What the program does:
        - It creates a fake joystick, and sets Axis X to be an image of 
*/



static void setup_key(int fd, int btn)
{
    if(ioctl(fd, UI_SET_KEYBIT, btn))
    {
        fprintf(stderr, "Couldn't setup button: %d, error(%d)\n", btn, errno);
        exit(4);
    }
    
}

static void setup_abs(int fd, int abs, int min, int max, int flat)
{
    if(ioctl(fd, UI_SET_ABSBIT, abs))
    {
        fprintf(stderr, "Couldn't setup abs: %d, error(%d)", abs, errno);
        exit(5);
    }
    struct uinput_abs_setup abs_s;
    memset(&abs_s, 0, sizeof(abs_s));
    abs_s.code = abs;
    abs_s.absinfo.minimum = min ;
    abs_s.absinfo.maximum = max ;
    abs_s.absinfo.flat    = flat;
    
    if(ioctl(fd, UI_ABS_SETUP, &abs_s))
    {
        fprintf(stderr, "Couldn't setup abs boundaries: %d(%d, %d, %d), error(%d)", abs, min, max, flat, errno);
        exit(6);
    }
}

int main(int argc, char** argv)
{
    /* Check args */
    if(argc != 3 && argc != 4)
    {
        fprintf(stderr, "Usage: %s <motion_sensor_even_file> <joystick_event_file> [<uinput_file>]\n", argv[0]);
        exit(1);
    }
    /* Open file descriptors */
    int msfd;
    int jefd;
    int uifd;

    msfd = open(argv[1], O_RDONLY | O_NONBLOCK);
    if(msfd < 0)
    {
        fprintf(stderr, "Couldn't open file %s\n", argv[1]);
        exit(2);
    }
    jefd = open(argv[2], O_RDONLY | O_NONBLOCK);
    if(jefd < 0)
    {
        fprintf(stderr, "Couldn't open file %s\n", argv[2]);
        exit(2);
    }
    char* uinput_path = argc == 3 ? "/dev/uinput" : argv[3];
    uifd = open(uinput_path, O_WRONLY | O_NONBLOCK);
    if(uifd < 0)
    {
        fprintf(stderr, "Couldn't open file %s\n", uinput_path);
        exit(2);
    }
    /* Setup virtual controller */
    fprintf(stdout, "Setting up virtual controller\n");
    struct uinput_setup usetup;
    
#define UI_IOCTL_1(a) { if(ioctl(uifd, a)) { fprintf(stderr, "Error in ioctl for: " #a ": %d\n", errno); exit(4); }} while(0)
#define UI_IOCTL(a, b) { if(ioctl(uifd, a, b)) { fprintf(stderr, "Error in ioctl for: " #a ":" #b ": %d\n", errno); exit(4); }} while(0)

    /* Syn */
    UI_IOCTL(UI_SET_EVBIT, EV_SYN);
    /* Axis */
    UI_IOCTL(UI_SET_EVBIT, EV_ABS);

    setup_abs(uifd, ABS_X    , 0, 255, 15);
    setup_abs(uifd, ABS_Y    , 0, 255, 15);
    setup_abs(uifd, ABS_Z    , 0, 255, 15);
    setup_abs(uifd, ABS_RX   , 0, 255, 15);
    setup_abs(uifd, ABS_RY   , 0, 255, 15);
    setup_abs(uifd, ABS_RZ   , 0, 255, 15);
    setup_abs(uifd, ABS_HAT0X, -1, 1, 0);
    setup_abs(uifd, ABS_HAT0Y, -1, 1, 0);
    /* Btns */
    UI_IOCTL(UI_SET_EVBIT, EV_KEY);

    setup_key(uifd, BTN_SOUTH );
    setup_key(uifd, BTN_EAST  );
    setup_key(uifd, BTN_NORTH );
    setup_key(uifd, BTN_WEST  );
    setup_key(uifd, BTN_TL    );
    setup_key(uifd, BTN_TR    );
    setup_key(uifd, BTN_TL2   );
    setup_key(uifd, BTN_TR2   );
    setup_key(uifd, BTN_SELECT);
    setup_key(uifd, BTN_START );
    setup_key(uifd, BTN_MODE  );
    setup_key(uifd, BTN_THUMBL);
    setup_key(uifd, BTN_THUMBR);
    /* Msc */
    UI_IOCTL(UI_SET_EVBIT, EV_MSC);

    UI_IOCTL(UI_SET_MSCBIT, MSC_SCAN);
    /* ff */
    // UI_IOCTL(UI_SET_EVBIT, EV_FF);

    // UI_IOCTL(UI_SET_FFBIT, FF_RUMBLE  );
    // UI_IOCTL(UI_SET_FFBIT, FF_PERIODIC);
    // UI_IOCTL(UI_SET_FFBIT, FF_SQUARE  );
    // UI_IOCTL(UI_SET_FFBIT, FF_TRIANGLE);
    // UI_IOCTL(UI_SET_FFBIT, FF_SINE    );
    // UI_IOCTL(UI_SET_FFBIT, FF_GAIN    );

    memset(&usetup, 0, sizeof(usetup));
    usetup.id.bustype = BUS_USB;
    usetup.id.vendor = 0x1234;
    usetup.id.product = 0x5678;
    strcpy(usetup.name, "Cmtx Virtual Controller");

    UI_IOCTL(UI_DEV_SETUP, &usetup);
    UI_IOCTL_1(UI_DEV_CREATE);

    fprintf(stdout, "Created virtual device\n");
    /* Loop */
    struct input_event msev; // motion sensor events
    struct input_event jev ; // joystick events
    struct input_event fev ; // fake events

    while(1)
    {    
        char msevb = 1, jevb = 1;
        if(read(msfd, &msev, sizeof(msev)) < 0)
        {
            if(errno != EWOULDBLOCK)
            {
                fprintf(stderr, "Error reading motion sensor event file: %d\n", errno);
                exit(3);
            }
            else msevb = 0;
        }
        if(read(jefd, &jev, sizeof(jev)) < 0)
        {
            if(errno != EWOULDBLOCK)
            {
                fprintf(stderr, "Error reading joystick event file: %d\n", errno);
                exit(3);
            }
            else jevb = 0;
        }
        if(jevb)
        {
            if(jev.type == EV_ABS && jev.code == ABS_X)
            {
                fprintf(stdout, "\r  ABSX(%d)", jev.value);
            }
            fev.time.tv_sec = 0;
            fev.time.tv_usec = 0;
            fev.type = jev.type;
            fev.code = jev.code;
            fev.value = jev.value;
            if(write(uifd, &fev, sizeof(fev)) < 0)
            {
                fprintf(stderr, "Error writing to file: %d\n", errno);
                exit(7);
            }
        }
        fev.time.tv_sec = 0;
        fev.time.tv_usec = 0;
        fev.type = EV_ABS;
        fev.code = ABS_X;

        if(msevb)
        {
            if(msev.type == EV_ABS && msev.code == ABS_X)
                fev.value = -(msev.value * 128 / 8000) + 130;
        }
	    else
		    fev.value = 130;
        if(write(uifd, &fev, sizeof(fev)) < 0)
        {
            fprintf(stderr, "Error writing to file: %d\n", errno);
            exit(7);
        }

    }

}
