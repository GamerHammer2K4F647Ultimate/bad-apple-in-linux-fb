# bad-apple-in-linux-fb

Bad apple!! in a linux framebuffer


## Building

### Dependecies
 - ffmpeg
 - good gpu
 - a static build of alsa-lib
 
```console
$ make -j1
$ sudo make install # if you want to
```

This will also generate the .ppm files for the frames (going to be a few gigabytes).

also it is a static build because this can (hopefully) run before `/sbin/init` (stupid idea)
