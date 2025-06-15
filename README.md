# bad-apple-in-linux-fb

Bad apple!! in a linux framebuffer

[Video](https://www.youtube.com/watch?v=CJqbjoz2cJo)

## Building

### Dependecies
 - ffmpeg
 - good gpu
 - a static build of [alsa-lib](https://www.alsa-project.org/files/pub/lib/)

### Static alsa-lib build:
```console
$ ./configure --disable-shared --enable-static --prefix=/opt/alsa-static
$ make
$ sudo make install
```
 
### And finally, the actual program
```console
$ make -j1
$ sudo make install # if you want to
```

This will also generate the .ppm files for the frames (going to be a few gigabytes).

also it is a static build because this can (hopefully) run before `/sbin/init` (stupid idea)
