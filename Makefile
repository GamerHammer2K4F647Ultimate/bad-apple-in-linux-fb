CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c11 -ggdb
LDFLAGS = -static

SRC = ./badapple_fb.c
OBJ = $(SRC:.c=.o)
VIDEO = ./badapple.mp4
FRAMES_FOLDER = ./frames

all: badapple_fb frames

.PHONY: clean help frames

badapple_fb: $(OBJ)
	$(CC) $(LDFLAGS) $^ -o $@

frames: $(VIDEO)
	mkdir -pv $(FRAMES_FOLDER)
	ffmpeg -i $^ ./$(FRAMES_FOLDER)/%04d.ppm
	@du -h $(FRAMES_FOLDER)

help:
	@echo "badapple in framebuffer makefile help"
	@echo "	all		build all"
	@echo "	clean		cleans the build"
	@echo "	frames		generate frames"
	@echo "	help		this"
	@echo "variables:"
	@echo "	VIDEO		video file (default = ./badapple.mp4)"
	@echo "	FRAMES_FOLDER	output frames folder (default = ./frames)"

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf ./badapple_fb $(OBJ) $(FRAMES_FOLDER)
