/* 
 * "Small Hello World" example. 
 * 
 * This example prints 'Hello from Nios II' to the STDOUT stream. It runs on
 * the Nios II 'standard', 'full_featured', 'fast', and 'low_cost' example 
 * designs. It requires a STDOUT  device in your system's hardware. 
 *
 * The purpose of this example is to demonstrate the smallest possible Hello 
 * World application, using the Nios II HAL library.  The memory footprint
 * of this hosted application is ~332 bytes by default using the standard 
 * reference design.  For a more fully featured Hello World application
 * example, see the example titled "Hello World".
 *
 * The memory footprint of this example has been reduced by making the
 * following changes to the normal "Hello World" example.
 * Check in the Nios II Software Developers Manual for a more complete 
 * description.
 * 
 * In the SW Application project (small_hello_world):
 *
 *  - In the C/C++ Build page
 * 
 *    - Set the Optimization Level to -Os
 * 
 * In System Library project (small_hello_world_syslib):
 *  - In the C/C++ Build page
 * 
 *    - Set the Optimization Level to -Os
 * 
 *    - Define the preprocessor option ALT_NO_INSTRUCTION_EMULATION 
 *      This removes software exception handling, which means that you cannot 
 *      run code compiled for Nios II cpu with a hardware multiplier on a core 
 *      without a the multiply unit. Check the Nios II Software Developers 
 *      Manual for more details.
 *
 *  - In the System Library page:
 *    - Set Periodic system timer and Timestamp timer to none
 *      This prevents the automatic inclusion of the timer driver.
 *
 *    - Set Max file descriptors to 4
 *      This reduces the size of the file handle pool.
 *
 *    - Check Main function does not exit
 *    - Uncheck Clean exit (flush buffers)
 *      This removes the unneeded call to exit when main returns, since it
 *      won't.
 *
 *    - Check Don't use C++
 *      This builds without the C++ support code.
 *
 *    - Check Small C library
 *      This uses a reduced functionality C library, which lacks  
 *      support for buffering, file IO, floating point and getch(), etc. 
 *      Check the Nios II Software Developers Manual for a complete list.
 *
 *    - Check Reduced device drivers
 *      This uses reduced functionality drivers if they're available. For the
 *      standard design this means you get polled UART and JTAG UART drivers,
 *      no support for the LCD driver and you lose the ability to program 
 *      CFI compliant flash devices.
 *
 *    - Check Access device drivers directly
 *      This bypasses the device file system to access device drivers directly.
 *      This eliminates the space required for the device file system services.
 *      It also provides a HAL version of libc services that access the drivers
 *      directly, further reducing space. Only a limited number of libc
 *      functions are available in this configuration.
 *
 *    - Use ALT versions of stdio routines:
 *
 *           Function                  Description
 *        ===============  =====================================
 *        alt_printf       Only supports %s, %x, and %c ( < 1 Kbyte)
 *        alt_putstr       Smaller overhead than puts with direct drivers
 *                         Note this function doesn't add a newline.
 *        alt_putchar      Smaller overhead than putchar with direct drivers
 *        alt_getchar      Smaller overhead than getchar with direct drivers
 *
 */

#include "sys/alt_stdio.h"
#include "system.h"
#define TRUE 1

/* Variables */
unsigned char ENABLE = 0;
unsigned char hmin = 0, hmax = 0;

/* Image Processing Functions */

void control(int a){
	if(a < 128)
		ENABLE = 1;
	else
		ENABLE = 0;
}

void grayscale(int row, int col, unsigned char *image, unsigned char *grayImage){
	//float aux[row][col];
	int i = 0, j = 0, x = row*col*3;	
	
	while(i < x){
		//grayImage[j] = image[i]*0.3125 + image[i+1]*0.5625 + image[i+2]*0.125; //change these floating point operations into bitwise
		grayImage[j] = (image[i]*5)/16 + (image[i+1]*9)/16 + image[i+2]/8;
		i = i+3;
		j++;
	}
}

void resize(int x1, int y1, unsigned char* grayImage, unsigned char* resizedImage){
	if((x1%2) == 0 && (y1%2) == 0){ //col = y1
		int i = 0, j = 0, f = 1, colSum = 2, size1 = x1*y1, col = y1; //i is index for grayImage, j is index for resizedImage
		unsigned char resizedPixel = 0;
		int newLine = 0, size2 = (x1/2)*(y1/2);
		while(j < size2){
			resizedPixel = (grayImage[i] + grayImage[i+1] + grayImage[i+col] + grayImage[i+1+col])/4;
			resizedImage[j] = resizedPixel;
			if(f%(col/2) == 0){
				i = col * colSum;
				colSum = colSum + 2;
				newLine = 1;
			}
			else{
				i = i + 2;
				newLine = 0;
			}
			f++;
			j++;
		}
	}
}

void sobel(int x2, int y2, unsigned char* image, unsigned char* edgeImage){
	int kx[9] = {-1,0,1,-2,0,2,-1,0,1};
	int ky[9] = {1,2,1,0,0,0,-1,-2,-1};
	int i = 0, j = 0, z = 1, f = 1, /*size = x2*y2,*/ limit = (x2-2)*(y2-2)/*size - 2*y2*/, col = y2;
	unsigned char gx = 0, gy = 0, g = 0;
	int newLine = 1;
	while(j < limit){
		gx = image[i]*kx[0] + /*image[i+1]*kx[1] +*/ image[i+2]*kx[2]
		   + image[i+col]*kx[3] + /*image[i+col+1]*kx[4] +*/ image[i+col+2]*kx[5]
		   + image[i+(2*col)]*kx[6] + /*image[i+(2*col)+1]*kx[7] +*/ image[i+(2*col)+2]*kx[8];
		
		gy = image[i]*ky[0] + image[i+1]*ky[1] + image[i+2]*ky[2]
		   + /*image[i+col]*ky[3] + image[i+col+1]*ky[4] + image[i+col+2]*ky[5]
		   +*/ image[i+(2*col)]*ky[6] + image[i+(2*col)+1]*ky[7] + image[i+(2*col)+2]*ky[8];
		
		//g = sqrt(gx*gx + gy*gy); //change with square root algorithm
		g = gx + gy;
		edgeImage[j] = g;
		
		if(f/(col-2) == z && i > 0 && !newLine){
			i = /*i + 3*/col*z;
			newLine = 1;
			z++; //current line in a 2D type of view
		}
		else{
			newLine = 0;
			i++;
		}
		
		f++;
		j++;
	}
}

void toAsciiArt(int row, int col, unsigned char *image, unsigned char *asciiImage){
	unsigned char asciiLevels[16] = {' ','.',':','-','=','+','/','t','z','U','w','*','0','#','%','@'};
	
	int i = 0, size = row*col;
	
	while(i < size){
		asciiImage[i] = asciiLevels[((int)image[i])/16];
		i++;
	}
}

unsigned char brightness(int size, unsigned char *min, unsigned char *max, unsigned char* array){
	*min = 10000;
	*max = -10000;
	int i = 0;
	unsigned char avg = 0; //holds the sums for the average brightness
	while(i < size){
		if(array[i] < *min)
			*min = array[i];
		if(array[i] > *max)
			*max = array[i];
		avg = avg + array[i];
		i++;
	}
	avg = avg/(i+1); //actual brightness
	return avg;
}

void correction(int size, unsigned char *array){
	control(brightness(size, &hmin, &hmax, array));
	if(ENABLE){
		unsigned char sub = hmax - hmin;
		int i = 0, mul = 1;
		if(sub <= 127){
			if(sub > 63)
				mul = 2;
			else{
				if(sub > 31)
					mul = 4;
				else{
					if(sub > 15)
						mul = 8;
					else
						mul = 16;
				}
			}
			while(i < size){
				array[i] = (array[i] - hmin) * mul;
				i++;
			}
		}
	}
}

extern void delay (int millisec);

int main()
{ 
	alt_putstr("Hello cpu_4!\n");
	
	delay(50);
	
	unsigned char* address = (unsigned char*) SHARED_ONCHIP_BASE, *sem_4 = address+3, *local_start = address, *prev_start = address;
	//unsigned char j = *(address+5), i = *(address+6);
	unsigned char grayImage[480], resizedImage[120], edgeImage[72]/*, asciiImage[((38*38)/4)/4]*/;
	//unsigned char grayImage[40*40/4], resizedImage[((40*40)/4)/4], edgeImage[((38*38)/4)/4], asciiImage[((38*38)/4)/4];
	
	while (TRUE) {
		while(!(*sem_4)){} //wait for the semaphore to be released
		*sem_4 = 0; //take the semaphore as soon as it gets released
		prev_start = address+5000;
		unsigned char j = *(address+5), i = *(address+5003)+2;
		unsigned char* asciiStart = address+6000+(*(address+5008)*(j-2));
		//alt_printf("4!\n"); //semaphore test print statement
		//j = *(address+5);
		//i = *(address+6);
		//the RGB pixels start at address+7
		local_start = address+7+(*(prev_start)*j*3)+(*(prev_start+1)*j*3)+(*(prev_start+2)*j*3)-(2*j*3);
		grayscale(i, j, local_start, grayImage);
		resize(i, j, grayImage, resizedImage);
		correction(i*i/4, resizedImage);
		sobel(i/2,j/2, resizedImage, edgeImage);
		toAsciiArt((i/2)-2, (j/2)-2, edgeImage, /*asciiImage*/asciiStart);
		
		*sem_4 = 1;
		delay(20);
	}
	
	return 0;
}
