#!/bin/bash

# run ./deps.sh first (only once is necessary), then ./build.sh for subsequent builds

mkdir -p bin/o

g++ -c dep/RectangleBinPack/*.cpp

gcc -c -DWEBP_HAVE_GIF -DWebPFree=free -DWebPMalloc=malloc dep/libwebp/examples/*.c dep/libwebp/src/demux/*.c -Idep/libwebp/src -Idep/libwebp dep/libwebp/imageio/imageio_util.c -Ofast -lwebp -lm -lgif

mv *.o bin/o

