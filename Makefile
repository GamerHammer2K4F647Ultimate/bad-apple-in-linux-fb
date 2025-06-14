CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c11 -ggdb -Wno-implicit-function-declaration
LDFLAGS = -static -lpthread -lm

SRC = ./badapple_fb.c
OBJ = $(SRC:.c=.o)
VIDEO = ./badapple.mp4
FRAMES_FOLDER = ./frames

all: badapple_fb frames audio

.PHONY: clean help frames install

badapple_fb: $(OBJ)
	$(CC) $^ -o $@ $(LDFLAGS)
	@echo "Ensure $@ is statically linked with ldd"

frames: $(VIDEO)
	mkdir -pv $(FRAMES_FOLDER)
	ffmpeg -i $^ $(FRAMES_FOLDER)/%04d.ppm
	@du -h $(FRAMES_FOLDER)

audio: $(VIDEO)
	ffmpeg -i $^ $(FRAMES_FOLDER)/badapple.wav

help:
	@echo "badapple in framebuffer makefile help"
	@echo "	all		build all"
	@echo "	clean		cleans the build"
	@echo "	frames		generate frames"
	@echo "	install		install as an initramfs hook"
	@echo "	help		this"
	@echo "variables:"
	@echo "	VIDEO		video file (default = ./badapple.mp4)"
	@echo "	FRAMES_FOLDER	output frames folder (default = ./frames)"
	
install: badapple_fb frames audio
	cp -r ./$(FRAMES_FOLDER) /usr/share
	cp ./badapple_fb /usr/bin
	chmod +x ./badapple_hook
	chmod +x ./badapple_install
	cp ./badapple_hook /usr/lib/initcpio/hooks/
	cp ./badapple_install /usr/lib/initcpio/install
	mv /usr/lib/initcpio/hooks/badapple_hook /usr/lib/initcpio/hooks/badapple
	mv /usr/lib/initcpio/install/badapple_install /usr/lib/initcpio/install/badapple
	@echo "From now on, installation is in your hands."
	@echo "Edit /etc/mkinitcpio.conf to use the badapple hook (put it at least before fsck)"
	@echo "And edit your boot loader's config to have the 'badapple' kernel param"
	@echo "ALSO: if you changed FRAMES_FOLDER to not be 'frames', you'll have to change it in the initramfs hook"
	
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf ./badapple_fb $(OBJ) $(FRAMES_FOLDER)
