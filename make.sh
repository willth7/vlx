#!/bin/sh

gcc -o libvlx.so src/vlx.c -lvulkan -fPIC -shared
