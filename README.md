# bad-apple-in-linux-fb

Bad apple!! in a linux framebuffer

No audio yet.

## Building

### Dependecies
 - ffmpeg
 - good gpu
 
```console
$ make
```

This will also generate the .ppm files for the frames (going to be a few gigabytes).

also it is a static build because this can (hopefully) run before `/sbin/init` (stupid idea)
