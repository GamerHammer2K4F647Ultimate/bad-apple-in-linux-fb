#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <pthread.h>
#define MA_NO_RUNTIME_LINKING
#define MA_NO_ALSA
#define MA_OSS
#define MA_NO_JACK
#define MA_NO_PULSEAUDIO
#define MA_NO_SDL
#define MA_NO_PIPEWIRE
#define MA_NO_AUDIOUNIT
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

void *play_sound(void *args)
{
	const char *fn = (const char*)args;
	ma_result result;
	ma_engine engine;
	
	result = ma_engine_init(NULL, &engine);
	if (result != MA_SUCCESS) {
		fprintf(stderr, "failed to init audio engine\n");
		return NULL;
	}
	
	ma_engine_play_sound(&engine, fn, NULL);
	//while(ma_engine_is_playing_sound(&engine, fn)) {
	//	ma_sleep(100);
	//}
	
	ma_engine_uninit(&engine);
	return NULL;
}

unsigned char *load_ppm(const char *filename, int *width, int *height) 
{
    FILE *f = fopen(filename, "rb");
    if (!f) {
        perror("fopen");
        return NULL;
    }

    char header[3];
    int maxval;
    fscanf(f, "%2s", header); 
    if (strcmp(header, "P6") != 0) {
        fprintf(stderr, "Not a P6 PPM file.\n");
        fclose(f);
        return NULL;
    }

    int c;
    do { c = fgetc(f); } while (c == '#'); 
    ungetc(c, f);
    fscanf(f, "%d %d %d\n", width, height, &maxval); 

    int img_size = (*width) * (*height) * 3;
    unsigned char *data = malloc(img_size);
    fread(data, 1, img_size, f);
    fclose(f);
    return data;
}

int main(int argc, char **argv) 
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <folder> <audio>\n", argv[0]);
        return 1;
    }

    int fb = open("/dev/fb0", O_RDWR);
    if (fb == -1) {
        perror("open fb");
        return 1;
    }

    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;
    ioctl(fb, FBIOGET_FSCREENINFO, &finfo);
    ioctl(fb, FBIOGET_VSCREENINFO, &vinfo);

    size_t screensize = finfo.line_length * vinfo.yres;
    uint8_t *fbp = mmap(NULL, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0);

    if (fbp == MAP_FAILED) {
        perror("mmap");
        close(fb);
        return 1;
    }

    int bytes_per_pixel = vinfo.bits_per_pixel / 8;
    int x_offset = 100, y_offset = 100;
	
	pthread_t sound_thrd;
	if(!pthread_create(&sound_thrd, NULL, play_sound, argv[2])) {
		return 1;
	}
    for (int idx = 0; idx < 6572; ++idx) { // where 6572 is the amount of frames
		int img_w, img_h;
		char fn[1024];
		snprintf(fn, sizeof(fn),"./%s/%04d.ppm", argv[1], idx+1);
    	unsigned char *img = load_ppm(fn, &img_w, &img_h);
    	if (!img) continue;
		for (int y = 0; y < img_h; y++) {
			for (int x = 0; x < img_w; x++) {
				int fb_x = x + x_offset;
				int fb_y = y + y_offset;

				if (fb_x >= vinfo.xres || fb_y >= vinfo.yres)
					continue;

				long loc = fb_x * bytes_per_pixel + fb_y * finfo.line_length;
				unsigned char r = img[(y * img_w + x) * 3 + 0];
				unsigned char g = img[(y * img_w + x) * 3 + 1];
				unsigned char b = img[(y * img_w + x) * 3 + 2];

				fbp[loc + 0] = b;
				fbp[loc + 1] = g;
				fbp[loc + 2] = r;
				fbp[loc + 3] = 0;
			}
		}
		free(img);
		usleep(33333);
    }

    munmap(fbp, screensize);
    close(fb);

	printf("executing /sbin/init after exiting this...");
	usleep(1000000);
	
	pthread_join(sound_thrd, NULL);
	
	printf("\033c"); // clear screen after

    return 0;
}

