CC      = gcc
CFLAGS  = -mtune=generic -O2 -pipe -fstack-protector-strong -fstack-check -fPIC -fPIE -s -Wall -Wextra -Wpedantic -Werror=pedantic

all:
	$(CC) $(CFLAGS) -o fbkb -Wl,--format=binary -Wl,image.ppm -Wl,--format=default fbkb.c
	$(CC) $(CFLAGS) -o key key.c
	$(CC) $(CFLAGS) -o userland_keystrokes userland_keystrokes.c
	$(CC) $(CFLAGS) -o osk -Wl,--format=binary -Wl,image.ppm -Wl,--format=default osk.c
	$(CC) $(CFLAGS) -o osk_mouse -Wl,--format=binary -Wl,image_mouse.ppm -Wl,--format=default osk_mouse.c

clean:
	rm -f fbkb key userland_keystrokes osk
