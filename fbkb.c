
/*
 * Example for drawing on framebuffer images
 * 2014 - Rafael Zurita
 * Based on : www.cubieforums.com/index.php?topic=33.0
 * and fb_drawimage() from https://svn.mcs.anl.gov/repos/ZeptoOS/trunk/BGP/packages/busybox/src/miscutils/fbsplash.c 
 *
 */

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>



extern uint8_t _binary_image_ppm_start[];
extern uint8_t _binary_image_ppm_end;
extern uint8_t _binary_image_ppm_size;



/**
 *	Draw image from PPM file
 */
static void fb_drawimage(FILE *theme_file, char *fbp, int xo, int yo, int bpp, int length) 
{
	char head[256];
	char s[80];
	unsigned char *pixline;
	unsigned i, j, width, height, line_size;

	memset(head, 0, sizeof(head));

	// parse ppm header
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

		// width, height, max_color_val
		if (sscanf(head, "P6 %u %u %u", &width, &height, &i) == 3)
			break;
// TODO: i must be <= 255!
	}

	line_size = width*3;
//	if (width > G.scr_var.xres)
//		width = G.scr_var.xres;
//	if (height > G.scr_var.yres)
//		height = G.scr_var.yres;

     long int location = 0;
	pixline = malloc(line_size);
	for (j = 0; j < height; j++) {
		unsigned char *pixel = pixline;

		if (fread(pixline, 1, line_size, theme_file) != line_size) {
			printf("bad PPM file");
			exit(EXIT_FAILURE);
		}

		for (i = 0; i < width; i++) {
			unsigned thispix;
			thispix = (((unsigned)pixel[0] << 8) & 0xf800)
				| (((unsigned)pixel[1] << 3) & 0x07e0)
				| (((unsigned)pixel[2] >> 3));
			pixel += 3;
             location = (i+xo) * (bpp/8) +
                        (j+yo) * length;

                 //*(fbp + location) = (((unsigned)pixel[0] << 8) & 0xf800);        // Some blue
                 //*(fbp + location + 1) = (((unsigned)pixel[1] << 3) & 0x07e0);     // A little green
                 //*(fbp + location + 2) = (((unsigned)pixel[2] >> 3));;    // A lot of red
                 *(fbp + location) = (((unsigned)pixel[0]));        // Some blue
                 *(fbp + location + 1) = (((unsigned)pixel[1]));     // A little green
                 *(fbp + location + 2) = (((unsigned)pixel[2]));    // A lot of red
                 *(fbp + location + 3) = 2;      // No transparency
		}
	}
	free(pixline);
	fclose(theme_file);
}




 int main()
 {
     int fbfd = 0;
     struct fb_var_screeninfo vinfo;
     struct fb_fix_screeninfo finfo;
     long int screensize = 0;
     char *fbp = 0;
     int x = 0, y = 0;
     long int location = 0;

     // Open the file for reading and writing
     fbfd = open("/dev/fb0", O_RDWR);
     if (fbfd == -1) {
         perror("Error: cannot open framebuffer device");
         exit(EXIT_FAILURE);
     }
     printf("The framebuffer device was opened successfully.\n");

     // Get fixed screen information
     if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo) == -1) {
         perror("Error reading fixed information");
         exit(EXIT_FAILURE);
     }

     // Get variable screen information
     if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
         perror("Error reading variable information");
         exit(EXIT_FAILURE);
     }

     printf("%dx%d, %dbpp\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);

     // Figure out the size of the screen in bytes
     screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;

     // Map the device to memory
     fbp = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED,
                        fbfd, 0);
     if (fbp == MAP_FAILED) {
         perror("Error: failed to map framebuffer device to memory");
         exit(EXIT_FAILURE);
     }
     printf("The framebuffer device was mapped to memory successfully.\n");

     x = 300; y = 100;       // Where we are going to put the pixel

    FILE *theme_file=fmemopen(_binary_image_ppm_start, (size_t)&_binary_image_ppm_size, "r");
    fb_drawimage(theme_file, fbp, vinfo.xoffset, vinfo.yoffset, vinfo.bits_per_pixel, finfo.line_length);

	int j, m=0;
	while(1) {
		m++;
		if (m==31) m=1;
		sleep(1);
		/* for (j=1;j<31;j++) { */
		for (j=0;j<54;j=j+2) {
			// printf("%i\n",j);
     // Figure out where in memory to put the pixel
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
                 *(fbp + location + 3) = (unsigned char) 100; // No transparency
             } else  { //assume 16bpp
                 int b = 10;
                 int g = (x-100)/6;     // A little green
                 int r = 31-(y-100)/16;    // A lot of red
                 unsigned short int t = r<<11 | g << 5 | b;
                 *((unsigned short int*)(fbp + location)) = t;
             }

         }

	}
	}
     munmap(fbp, screensize);
     close(fbfd);
     return 0;
 }
