//#include <stdio.h>
#include "system.h"
//#include "io.h"
#include "sys/alt_stdio.h"

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
	alt_printf("Hello from cpu_2!\n");
	
	delay(50);
	
	unsigned char* address = (unsigned char*) SHARED_ONCHIP_BASE, *sem_2 = address+1, *local_start = address, *prev_start = address;
	//unsigned char j = *(address+5), i = *(address+6);
	unsigned char grayImage[560], resizedImage[140], edgeImage[90]/*, asciiImage[((38*38)/4)/4]*/;
	//unsigned char grayImage[40*40/4], resizedImage[((40*40)/4)/4], edgeImage[((38*38)/4)/4], asciiImage[((38*38)/4)/4];
	
	while (TRUE) {
		while(!(*sem_2)){} //wait for the semaphore to be released
		*sem_2 = 0; //take the semaphore as soon as it gets released
		prev_start = address+5000;
		unsigned char j = *(address+5), i = *(address+5001)+4;
		unsigned char* asciiStart = address+6000+(*(address+5006)*(j-2));
		//alt_printf("2!\n"); //semaphore test print statement
		//j = *(address+5);
		//i = *(address+6);
		//the RGB pixels start at address+7
		local_start = address+7+(*(prev_start)*j*3)-(2*j*3);
		grayscale(i, j, local_start, grayImage);
		resize(i, j, grayImage, resizedImage);
		correction(i*i/4, resizedImage);
		sobel(i/2,j/2, resizedImage, edgeImage);
		toAsciiArt((i/2)-2, (j/2)-2, edgeImage, /*asciiImage*/asciiStart);
		
		*sem_2 = 1;
		delay(20);
	}
	return 0;
}
