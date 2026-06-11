#!/bin/bash

if [ ! -f bin/o/gifdec.o ]; then
	echo "ERROR: run ./deps.sh first to compile dependencies, then retry ./build.sh"
	exit -1
fi

mkdir -p bin/o

gcc -c src/*.c src/exporter/*.c -D_DEFAULT_SOURCE -D_XOPEN_SOURCE=500 -Idep -Wall -Wextra -s -Os
mv *.o bin/o

g++ -o bin/ezspritesheet bin/o/*.o -lgif -lm -lwebp -Wall -Wextra

