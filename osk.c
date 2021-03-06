
/*
 * osk.c : framebuffer on-screen keyboard
 * 	   Might be usefull at boot time for accessibility.
 * 
 * Copyright (C) 2014 Rafael Ignacio Zurita <rafaelignacio.zurita@gmail.com>
 *
 * Based on : http://thiemonge.org/getting-started-with-uinput
 * and www.cubieforums.com/index.php?topic=33.0
 * and fb_drawimage() from https://svn.mcs.anl.gov/repos/ZeptoOS/trunk/BGP/packages/busybox/src/miscutils/fbsplash.c 
 *
 *   osk.c is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *    You should have received a copy of the GNU Library General Public
 *    License along with this software (check COPYING); if not, write to the Free
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */



#include <fcntl.h>
#include <linux/fb.h>
#include <linux/input.h>
#include <linux/ioctl.h>
#include <linux/uinput.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>



/* key codes reference in /usr/include/linux/input-event-codes.h */
unsigned char abc[] = {KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G, KEY_H, KEY_I, KEY_J, KEY_K, KEY_L, KEY_M, KEY_N, KEY_O, KEY_P, KEY_Q, KEY_R, KEY_S, KEY_T, KEY_U, KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z, KEY_ENTER};

extern uint8_t _binary_image_ppm_start[];
extern uint8_t _binary_image_ppm_end;
extern uint8_t _binary_image_ppm_size;



/*
 *	Draw image from PPM file
 */
static void fb_drawimage(FILE *theme_file, char *fbp, int xo, int yo, int bpp, int length)
{
	char head[256];
	char s[80];
	unsigned char *pixline;
	unsigned i, j, width, height, line_size;

	memset(head, 0, sizeof(head));

	/* parse ppm header */
	while (1) {
		if (fgets(s, sizeof(s), theme_file) == NULL) {
			printf("bad PPM file");
			exit(EXIT_FAILURE);
		}

		if (s[0] == '#')
			continue;

		if (strlen(head) + strlen(s) >= sizeof(head)) {
			printf("bad PPM file");
			exit(EXIT_FAILURE);
		}

		strcat(head, s);
		if (head[0] != 'P' || head[1] != '6') {
			printf("bad PPM file");
			exit(EXIT_FAILURE);
		}

		/* width, height, max_color_val */
		if (sscanf(head, "P6 %u %u %u", &width, &height, &i) == 3)
			break;
		/* TODO: i must be <= 255! */
	}

	line_size = width*3;

	long int location = 0;
	pixline = malloc(line_size);
	for (j = 0; j < height; j++) {
		unsigned char *pixel = pixline;

		if (fread(pixline, 1, line_size, theme_file) != line_size) {
			printf("bad PPM file");
			exit(EXIT_FAILURE);
		}

		for (i = 0; i < width; i++) {
			pixel += 3;
			location = (i+xo) * (bpp/8) + (j+yo) * length;

			*(fbp + location) = (((unsigned)pixel[0]));
			*(fbp + location + 1) = (((unsigned)pixel[1]));
			*(fbp + location + 2) = (((unsigned)pixel[2]));
			*(fbp + location + 3) = 2;      /* No transparency */
		}
	}
	free(pixline);
	fclose(theme_file);
}



int send_event(int fd, __u16 type, __u16 code, __s32 value)
{
    struct input_event event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.code = code;
    event.value = value;

    if (write(fd, &event, sizeof(event)) != sizeof(event)) {
        fprintf(stderr, "Error on send_event");
        return -1;
    }

    return 0;
}



int main(int argc, char *argv[]) {

	if( argc != 2 ) {
		printf("Error: Invalid number of arguments.\n");
		printf("Usage: %s /dev/input/event0\n", argv[0]);
		exit(EXIT_FAILURE);
	}

     int fbfd = 0;
     struct fb_var_screeninfo vinfo;
     struct fb_fix_screeninfo finfo;
     long int screensize = 0;
     char *fbp = 0;
     int x = 0, y = 0;
     long int location = 0;

	/* uinput init */
	int i,fd;
	struct uinput_user_dev device;
	memset(&device, 0, sizeof device);

	fd=open("/dev/uinput",O_WRONLY);
	strcpy(device.name,"test keyboard");

	device.id.bustype=BUS_USB;
	device.id.vendor=1;
	device.id.product=1;
	device.id.version=1;

	if (write(fd,&device,sizeof(device)) != sizeof(device))
	{
		fprintf(stderr, "error setup\n");
	}

	if (ioctl(fd,UI_SET_EVBIT,EV_KEY) < 0)
		fprintf(stderr, "error evbit key\n");

	for (i=0;i<27;i++)
		if (ioctl(fd,UI_SET_KEYBIT, abc[i]) < 0)
			fprintf(stderr, "error evbit key\n");

	if (ioctl(fd,UI_SET_EVBIT,EV_REL) < 0)
		fprintf(stderr, "error evbit rel\n");

	for(i=REL_X;i<REL_MAX;i++)
		if (ioctl(fd,UI_SET_RELBIT,i) < 0)
			fprintf(stderr, "error setrelbit %d\n", i);

	if (ioctl(fd,UI_DEV_CREATE) < 0)
	{
		fprintf(stderr, "error create\n");
	}

	sleep(2);
	/* end uinput init */


	/* init framebuffer */
	/* Open the file for reading and writing */
	fbfd = open("/dev/fb0", O_RDWR);
	if (fbfd == -1) {
		perror("Error: cannot open framebuffer device");
		exit(EXIT_FAILURE);
	}
	printf("The framebuffer device was opened successfully.\n");

	/* Get fixed screen information */
	if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo) == -1) {
		perror("Error reading fixed information");
		exit(EXIT_FAILURE);
	}

	/* Get variable screen information */
	if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
		perror("Error reading variable information");
		exit(EXIT_FAILURE);
	}

	printf("%dx%d, %dbpp\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);

	/* Figure out the size of the screen in bytes */
	screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;

	/* Map the device to memory */
	fbp = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED,
                        fbfd, 0);
	if (fbp == MAP_FAILED) {
		perror("Error: failed to map framebuffer device to memory");
		exit(EXIT_FAILURE);
	}
	printf("The framebuffer device was mapped to memory successfully.\n");

	/* end init framebuffer */



	/* init event0 */
	int fdkey;
	fdkey = open(argv[0], O_RDONLY);
	struct input_event evkey;

	int flags = fcntl(fd, F_GETFL, 0);
	int err;
	fcntl(fdkey, F_SETFL, flags | O_NONBLOCK);
	/* end init event0 */


	int j, m=0,k;
	while(1) {
		m++;
		if (m==29) m=0; /* 27 keys, and 2 extra seconds for exit */

		FILE *theme_file=fmemopen(_binary_image_ppm_start, (size_t)&_binary_image_ppm_size, "r");
		fb_drawimage(theme_file, fbp, vinfo.xoffset, vinfo.yoffset, vinfo.bits_per_pixel, finfo.line_length);

		for (j=0;j<54;j=j+2) {
     		for (y = 20; y < 30; y++)
         		for (x = 12*j; x < 12*j+8; x++) {

				location = (x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) +
					(y+vinfo.yoffset) * finfo.line_length;

			if (vinfo.bits_per_pixel == 32) {
				*(fbp + location) = (unsigned char) 100; // Some blue
				if ((j/2)==m) {
					*(fbp + location + 1) = (unsigned char) 200; // A lot of green
					*(fbp + location + 2) = (unsigned char) 100; // A little red
				} else {
					*(fbp + location + 1) = (unsigned char) 100; // A little green
					*(fbp + location + 2) = (unsigned char) 200; // A lot of red
				}
				*(fbp + location + 3) = 100;      // No transparency
			} else  { /* assume 16bpp */
				int b = 10;
				int g = (x-100)/6;     // A little green
				int r = 31-(y-100)/16;    // A lot of red
				unsigned short int t = r<<11 | g << 5 | b;
				*((unsigned short int*)(fbp + location)) = t;
			}

		}
		}


		/* check for key */
		k=0;
		err=-1;
		while ((err==-1) && (k<500)) {
			err = read(fdkey, &evkey, sizeof(struct input_event));
			k++;
			usleep(1000);
		}

		if (err!=-1) {
			if ((evkey.type==1) && (evkey.value==0)) {

				if ((m==27) || (m==28)) break;
				send_event(fd, EV_KEY, abc[m], 1);
				send_event(fd, EV_SYN, SYN_REPORT, 0);
				send_event(fd, EV_KEY, abc[m], 0);
				send_event(fd, EV_SYN, SYN_REPORT, 0);
			} else {
				if (m==0)
					m=27;
				else
					m--;
			}
			printf("%i",m);
		}


	}




	close(fd);




     munmap(fbp, screensize);
     close(fbfd);
     return 0;
}

