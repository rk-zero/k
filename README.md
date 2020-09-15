# k

K is my first try to make as-fast-as-possible memory manipulation and on-screen-drawing.

Unfortunately it relies on /dev/random

This uses C, the linux framebuffer /dev/fb0 and a fullhd-resolution with 32bpp.

You can utilize from 1 up to 1080 threads, so safe for future CPUs.

This program will run until 0 entropy is achieved, then quit and print out the number of needed cycles.

<img src="memuni.png"/>
