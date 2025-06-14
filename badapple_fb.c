#define __STDC_WANT_LIB_EXT1__ 1
#define _POSIX_C_SOURCE 200809L
#define __USE_GNU

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <pthread.h>

#include <alsa/asoundlib.h>

int fb = 0;
uint8_t *fbp;
size_t screensize;

void signal_handler(int sig)
{
	printf("recived signal %d\n", sig);
	munmap(fbp, screensize);
	close(fb);
	usleep(1000000);
	printf("\033c"); // clear screen after
	exit(sig);
}

static int play_wav_file(const char *fn)
{
	FILE *f = fopen(fn, "rb");
	if(!f) {
		perror("fopen");
		return 1;
	}
	
	char hdr[4];
	fread(hdr, 1, 4, f);
	if(strncmp(hdr, "RIFF", 4) != 0) {
		fprintf(stderr, "not a RIFF file.\n");
		fclose(f);
		return 1;
	}
	
	fseek(f, 22, SEEK_SET);
	short channels;
	fread(&channels, sizeof(short), 1, f);
	
	int sample_rate;
	fseek(f, 24, SEEK_SET);
	fread(&sample_rate, sizeof(int), 1, f);
	
	short bits_per_sample;
	fseek(f, 34, SEEK_SET);
	fread(&bits_per_sample, sizeof(short), 1, f);
	
	fseek(f, 44, SEEK_SET);
	
	snd_pcm_t *pcm;
	if (snd_pcm_open(&pcm, "default", SND_PCM_STREAM_PLAYBACK, 0) < 0) {
		perror("snd_pcm_open");
		fclose(f);
		return 1;
	}
	
	if (snd_pcm_set_params(pcm, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED, channels, sample_rate, 1, 500000) < 0) {
		fprintf(stderr, "snd_pcm_set_params failed\n");
		snd_pcm_close(pcm);
		fclose(f);
		return 1;
	}
	
	const int buffer_size = 4096;
	char buf[4096];
	
	while(!feof(f)) {
		size_t bytes_read = fread(buf, 1, buffer_size, f);
		if (bytes_read > 0) {
			int frames = bytes_read / (bits_per_sample / 8 * channels);
			snd_pcm_writei(pcm, buf, frames);
		}
	}
	
	snd_pcm_drain(pcm);
	snd_pcm_close(pcm);
	fclose(f);
	return 0;
}

void *play_sound(void *args)
{
	const char *fn = (void*)args;
	play_wav_file(fn);
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
	signal(SIGINT, signal_handler);
	signal(SIGSEGV, signal_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGABRT, signal_handler);

	if (argc != 3) {
		if (argc == 2) {
			if (strcmp(argv[1], "-help") == 0) {
				fprintf(stderr, "%s: plays bad apple in a linux framebuffer\n", argv[0]);
				fprintf(stderr, "usage: %s <folder> <audio>\n", argv[0]);
				fprintf(stderr, "<folder>		folder where the frames are located\n");
				fprintf(stderr, "<audio>		audio file. can be set to `none` if you don\'t want audio\n");
				fprintf(stderr, "-help			show this menu\n");
				return 1;
			}			
		}
		fprintf(stderr, "Usage: %s <folder> <audio>\n", argv[0]);
		return 1;
	}

	fb = open("/dev/fb0", O_RDWR);
	if (fb == -1) {
		perror("open fb");
		return 1;
	}

	struct fb_var_screeninfo vinfo;
	struct fb_fix_screeninfo finfo;
	ioctl(fb, FBIOGET_FSCREENINFO, &finfo);
	ioctl(fb, FBIOGET_VSCREENINFO, &vinfo);

	screensize = finfo.line_length * vinfo.yres;
	fbp = mmap(NULL, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0);

	if (fbp == MAP_FAILED) {
		perror("mmap");
		close(fb);
		return 1;
	}

	int bytes_per_pixel = vinfo.bits_per_pixel / 8;
	int x_offset = 150, y_offset = 150;

	bool audio;

	if (strcmp(argv[2], "none") == 0) audio = false;
	else audio = true;
	pthread_t sound_thrd;

	if (audio) {
		if(pthread_create(&sound_thrd, NULL, play_sound, argv[2]) != 0) {
			perror("pthread_create");
			return 1;
		}
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

	if (audio) pthread_join(sound_thrd, NULL);

	printf("\033c"); // clear screen after

	printf("executing /sbin/init after exiting this...");
	usleep(1000000);

	return 0;
}