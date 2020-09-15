#include "dbg.h"

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h> 

#include <sys/ioctl.h> //ioctl() and TIOCGWINSZ
#include <unistd.h> // for STDOUT_FILENO

#include <fcntl.h>
#include <linux/fb.h>
#include <linux/kd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <omp.h>

#define GENESIS_BLOCK_SIZE (1920*1080) // get that from stdin
#define GENESIS_AXIS_X 1920
#define GENESIS_AXIS_Y 1080
#define GENESIS_MAX_AGE 200

struct fb_fix_screeninfo finfo;
struct fb_var_screeninfo vinfo;

struct Rainbow {
	int value;
	int r;
	int b;
	int g;
	int alpha;
};

struct Block {
	char *name;
	char data[GENESIS_AXIS_X][GENESIS_AXIS_Y];
	int size;
	int x;
	int y;
};

struct Snake {
	int x;
	int y;
	int length;
	int history[];
};

struct Block *Block_create(char *name, int size, int x, int y) { // returns an empty block

	struct Block *sheet = malloc(sizeof(struct Block));
	assert (sheet != NULL); // makes sure we got the memory
	sheet->name = strdup(name);	
	sheet->size = size;
	sheet->x = x;
	sheet->y = y;

	return sheet;

}

void Block_destroy(struct Block *sheet) {
	
	assert (sheet != NULL);
	free (sheet->name); // because strdup(licate) is like malloc()
	//free (sheet->data);
	free (sheet);

}

struct Block *Block_genesis() {

		//struct winsize size;
		//ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
		//int datasize = size.ws_col * size.ws_row;

		struct Block *genesis = Block_create("genesis", GENESIS_BLOCK_SIZE, GENESIS_AXIS_X, GENESIS_AXIS_Y);
		// this works because we generate a pointer and return it.

		int x = 0;
		int y = 0;
		
		for (y = 0; y < genesis->y; y++) { // Y

			for (x = 0; x < genesis->x; x++) { // X
				
					genesis->data[x][y] = 1;

			}

		}	

		return genesis;

}

struct Block *Block_random(struct Block *sheet) {

		time_t t;

		srand((unsigned) time(&t));

		int rando;
		int x = 0;
		int y = 0;
		
		for (y = 0; y < sheet->y; y++) { // Y

			for (x = 0; x < sheet->x; x++) { // X
				
				rando = rand() % 5;

				if (rando == 0) sheet->data[x][y] = 0;
				if (rando == 1) sheet->data[x][y] = 1;

			}

		}	

		return sheet;

}

struct Rainbow *getRainbowFromAge(double age) {

	struct Rainbow *rainbow;
	int section;
	int fraction;
	// divide in 6 groups 0 - 5. Max age will be 1 * 5, start age 0
	section = floor((age / GENESIS_MAX_AGE) * 6);
	// fraction from 0 - 255	
	fraction = floor((age / GENESIS_MAX_AGE) * 255);

	switch (section) {

		case 0: rainbow->r = 255 - fraction; rainbow->g = 255 - fraction; rainbow->b = 255 - fraction; break;
		case 1: rainbow->r = 255; rainbow->g = fraction; rainbow->b = 0; break;
		case 2: rainbow->r = 255 - fraction; rainbow->g = 255; rainbow->b = 0; break;
		case 3: rainbow->r = 0; rainbow->g = 255; rainbow->b = fraction; break;
		case 4: rainbow->r = 0; rainbow->g = 255 - fraction; rainbow->b = 255; break;
		case 5: rainbow->r = fraction; rainbow->g = 0; rainbow->b = 255; break;
		case 6: rainbow->r = 255; rainbow->g = 0; rainbow->b = 255; break;

	}

	rainbow->alpha = age;

	return rainbow;

}


int ageCell(int age) {

	if (age + 1 > GENESIS_MAX_AGE)
		return age;
	else
		return age+1;

}

struct Block *Block_scrambler(struct Block *sheet) {

		time_t t;

		srand((unsigned) time(&t));

		int rando;
		int x = 0;
		int y = 0;

		for (y = 0; y < sheet->y; y++) { // Y

			for (x = 0; x < sheet->x; x++) { // X
		
				// make older cells more defective
				if (sheet->data[x][y] > (GENESIS_MAX_AGE / 4))
					rando = rand() % (GENESIS_MAX_AGE - sheet->data[x][y] + 1);
				else
					rando = rand() % GENESIS_MAX_AGE;


				if (rando == 0) sheet->data[x][y] = 0;
				if (rando == 1) sheet->data[x][y] = ageCell(sheet->data[x][y]);

				
			}

		}	

		return sheet;

}

int Block_countneighbors(struct Block *sheet, int x, int y) {

	int neighbors = 0;

	int leftindex = x - 1;
	int rightindex = x + 1;
	int upperindex = y - 1;
	int lowerindex = y + 1;
	if (leftindex < 0) leftindex = (GENESIS_AXIS_X - 1);
	if (rightindex == GENESIS_AXIS_X) rightindex = 0;
	if (upperindex < 0) upperindex = (GENESIS_AXIS_Y - 1);
	if (lowerindex == GENESIS_AXIS_Y) lowerindex = 0;

	int upperleft = 0;
	int upper = 0;
	int upperright = 0;
	int left = 0;
	int right = 0;
	int lowerleft = 0;
	int lower = 0;
	int lowerright = 0;

	if (sheet->data[leftindex][upperindex] >= 1) upperleft = 1;
	if (sheet->data[x][upperindex] >= 1) upper = 1;
	if (sheet->data[rightindex][upperindex] >= 1) upperright = 1;
	if (sheet->data[leftindex][y] >= 1) left = 1;
	if (sheet->data[rightindex][y] >= 1) right = 1;
	if (sheet->data[leftindex][lowerindex] >= 1) lowerleft = 1;
	if (sheet->data[x][lowerindex] >= 1) lower = 1;
	if (sheet->data[rightindex][lowerindex] >= 1) lowerright = 1;

	neighbors = upperleft + upper + upperright + left + right + lowerleft + lower + lowerright;

	return neighbors;				

}


struct Block *Block_GameOfLife(struct Block *sheet) {

		struct Block *nextblock = Block_create("genesis", GENESIS_BLOCK_SIZE, GENESIS_AXIS_X, GENESIS_AXIS_Y);

		// fill the block with 0:
		

		memset(nextblock->data, 0, nextblock->size * sizeof(char));

		int x;
		int y;

		#pragma omp parallel shared (nextblock, sheet) private (x, y)
		{

			int thread = omp_get_thread_num();
			int nothreads = omp_get_num_threads();

			for (y = thread; y <= sheet->y; y+=nothreads) { // Y

				for (x = 0; x < sheet->x; x++) { // X
					
					if (sheet->data[x][y] >= 1) { //live cell with age

							nextblock->data[x][y] = 0;

							if (Block_countneighbors(sheet, x, y) == 2 || Block_countneighbors(sheet, x, y) == 3)
									nextblock->data[x][y] = ageCell(sheet->data[x][y]);
							
					}

					else {

							if (Block_countneighbors(sheet, x, y) == 3) nextblock->data[x][y] = 1;
													
					}

				}

			}

		}

		Block_destroy(sheet);

		return nextblock; // this returns just a pointer.

}


int main (int argc, char *argv[]) {

	//#pragma omp single
	system("clear");
	system("dd if=/dev/zero of=/dev/fb0");

	int timecounter = 0;
	struct Block *genesis = Block_create("genesis", GENESIS_BLOCK_SIZE, GENESIS_AXIS_X, GENESIS_AXIS_Y);
	struct Block *history = Block_create("genesis", GENESIS_BLOCK_SIZE, GENESIS_AXIS_X, GENESIS_AXIS_Y);

	Block_random(genesis);

  int fbfd = 0;
  struct fb_var_screeninfo vinfo;
  struct fb_fix_screeninfo finfo;
  long int screensize = 0;
  char *fbp = 0;
  int x = 0, y = 0;
	int scalex = 0, scaley = 0;
  long int location = 0;

  // Open the file for reading and writing
  fbfd = open("/dev/fb0", O_RDWR);
  if (fbfd == -1) {
      perror("Error: cannot open framebuffer device");
      exit(1);
  }
  printf("The framebuffer device was opened successfully.\n");

  // Get fixed screen information
  if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo) == -1) {
      perror("Error reading fixed information");
      exit(2);
  }

  // Get variable screen information
  if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
      perror("Error reading variable information");
      exit(3);
  }

  printf("%dx%d, %dbpp\n", vinfo.xres_virtual, vinfo.yres_virtual, vinfo.bits_per_pixel);

  // Figure out the size of the screen in bytes
  screensize = vinfo.xres_virtual * vinfo.yres_virtual * vinfo.bits_per_pixel / 8;

	printf("Screensize %d\n", screensize);

  // Map the device to memory
  fbp = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
  if ((int)fbp == -1) {
      perror("Error: failed to map framebuffer device to memory");
      exit(4);
  }
  printf("The framebuffer device was mapped to memory successfully.\n");

	printf("Screensize %d\n", screensize);

	//sleep(2);

	system("clear");


  //int ttyfd = open ("/dev/tty", O_RDWR);
	//ioctl (ttyfd, KDSETMODE, KD_GRAPHICS);

	while (1) {

		//printf("Timecounter: %d \n", timecounter);
		//printf("Genesis-Block Address: %p \n", &genesis);
		//printf("Genesis-Block Size: %zu \n", genesis->size);

		//		genesis = 
		genesis = Block_GameOfLife(genesis);
		//printf("Genesis-Block: %s \n", genesis);

		x = 0; y = 0;       // Where we are going to put the pixel


		// Figure out where in memory to put the pixel
		for (y = 0; y < genesis->y; y++) {
		    for (x = 0; x < genesis->x; x++) {

						//int scale;
						//for (scalex = 0; scalex<2; scalex++) {

							//for (scaley = 0; scaley<2; scaley++) {

						    location = (((x)+scalex)+vinfo.xoffset) * (vinfo.bits_per_pixel/8) +
						               (((y)+scaley)+vinfo.yoffset) * finfo.line_length;

								if (genesis->data[x][y] >= 1) {

									struct Rainbow *rainbow = getRainbowFromAge(genesis->data[x][y]);

							    *(fbp + location) = rainbow->b;        	// blue
							    *(fbp + location + 1) = rainbow->g;     // green
							    *(fbp + location + 2) = rainbow->r;    	// red
							    *(fbp + location + 3) = rainbow->alpha;      // transparency


								}

								else {


							    *(fbp + location) = 0;        // Some blue
							    *(fbp + location + 1) = 0;     // A little green
							    *(fbp + location + 2) = 0;    // A lot of red
							    *(fbp + location + 3) = 0;      // No transparency

									location += 4;

								}

							//}

						//}

		    }

		}

		/*
		int x = 0;
		int y = 0;

		for (y = 0; y < genesis->y; y++) { // Y

			for (x = 1; x < genesis->x; x++) { // X

				if (genesis->data[x][y] == 0)
					printf(" ");
				else
					printf("O");
					
			}

			printf("\n");

		}
		*/

		timecounter++;

		if (timecounter % 100 == 0)
			//Block_scrambler(genesis);

		if (timecounter % 10 == 0) {

			if (memcmp(history->data, genesis->data, genesis->size) == 0) {

				printf ("Zero entropy after %d cycles\n", timecounter);
				exit(0);

			}
			*history = *genesis;

		}

		if (timecounter >= 2000) {

			//Block_random(genesis);
			//timecounter = 0;

		}

		//usleep (80000);
		//system("clear");

	}

	munmap(fbp, screensize);
	close(fbfd);

	//Block_destroy(*genesis);

	return 0;

}