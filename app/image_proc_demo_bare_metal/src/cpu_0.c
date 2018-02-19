#include "altera_avalon_performance_counter.h"
#include "system.h"
#include <stdio.h>
#include <math.h>
#include "images.h"

#define DEBUG 0

#define SECTION_1 1
//#define SECTION_2 2
//#define SECTION_3 3

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
		grayImage[j] = image[i]*0.3125 + image[i+1]*0.5625 + image[i+2]*0.125; //change these floating point operations into bitwise
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
		gx = image[i]*kx[0] + image[i+1]*kx[1] + image[i+2]*kx[2]
		   + image[i+col]*kx[3] + image[i+col+1]*kx[4] + image[i+col+2]*kx[5]
		   + image[i+(2*col)]*kx[6] + image[i+(2*col)+1]*kx[7] + image[i+(2*col)+2]*kx[8];
		
		gy = image[i]*ky[0] + image[i+1]*ky[1] + image[i+2]*ky[2]
		   + image[i+col]*ky[3] + image[i+col+1]*ky[4] + image[i+col+2]*ky[5]
		   + image[i+(2*col)]*ky[6] + image[i+(2*col)+1]*ky[7] + image[i+(2*col)+2]*ky[8];
		
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
		asciiImage[i] = asciiLevels[((int)image[i])%16];
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

//int fac5, fac10, fac15;

//long factorial(int n)
//{
  //int c;
  //long result = 1;
 
  //for (c = 1; c <= n; c++)
    //result = result * c;
 
  //return result;
//}

int main()
{ 
  printf("Hello from Nios II!\n");
  
  char current_image=0;
  
  #if DEBUG == 1
	/* Sequence of images for testing if the system functions properly */
	char number_of_images=10;
	unsigned char* img_array[10] = {img1_24_24, img1_32_22, img1_32_32, img1_40_28, img1_40_40, 
			img2_24_24, img2_32_22, img2_32_32, img2_40_28, img2_40_40 };
	#else
	/* Sequence of images for measuring performance */
	char number_of_images=3;
	unsigned char* img_array[3] = {img1_32_32, img2_32_32, img3_32_32};
	#endif
	
	unsigned char a = 0;
	unsigned char i = 0;
	unsigned char j = 0;
	
	while(a < number_of_images){
		
		/* Reset Performance Counter */
	    PERF_RESET(PERFORMANCE_COUNTER_0_BASE);  
	
	    /* Start Measuring */
	    PERF_START_MEASURING (PERFORMANCE_COUNTER_0_BASE);
	
	    /* Section 1 */
	    PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, SECTION_1);
		
		j = *img_array[a];
		i = *(img_array[a]+1);
		
		/* Image Processing Execution */
		int /*i = 0,*/ /*size1 = (x*3)*y,*/ size2 = i*j, size3 = i*j/4, size4 = (i/2-2)*(j/2-2);
		
		unsigned char grayImage[size2];
		unsigned char resizedImage[size3];
		unsigned char edgeImage[size4];
		unsigned char asciiImage[size4];
		
		grayscale(i, j, img_array[current_image]+3, grayImage);
		resize(i, j, grayImage, resizedImage);
		correction(size3, resizedImage);
		sobel(i/2,j/2, resizedImage, edgeImage);
		toAsciiArt((i/2)-2, (j/2)-2, edgeImage, asciiImage);
		
		//if(DEBUG){
			//printf("---- Gray Image ----");
			//int z = 0;
			
			//while(z < size2){
			////grayscale (put "i < size2" as condition for the while loop)
			//printf("%d",grayImage[z]);
			//printf("%c"," ");
			//if((z+1)%j == 0 && z > 0)
				//printf("\n");
			//z++;
	//}
		//}
		
		PERF_END(PERFORMANCE_COUNTER_0_BASE, SECTION_1);
		
		/* Print report */
		perf_print_formatted_report
		(PERFORMANCE_COUNTER_0_BASE,            
		ALT_CPU_FREQ,        // defined in "system.h"
		1,                   // How many sections to print
		"Section 1"        // Display-name of section(s).
		);   
		
		a++;
	}

  /* Reset Performance Counter */
  //PERF_RESET(PERFORMANCE_COUNTER_0_BASE);  

  /* Start Measuring */
  //PERF_START_MEASURING (PERFORMANCE_COUNTER_0_BASE);

  /* Section 1 */
  //PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, SECTION_1);
  //fac5 = factorial(5);
  //PERF_END(PERFORMANCE_COUNTER_0_BASE, SECTION_1);
 
  /* Section 2 */
  //PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, SECTION_2);
  //fac10 = factorial(10);
  //PERF_END(PERFORMANCE_COUNTER_0_BASE, SECTION_2);

  /* Section 3 */
  //PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, SECTION_3);
  //fac15 = factorial(15);
  //PERF_END(PERFORMANCE_COUNTER_0_BASE, SECTION_3);

  /* End Measuring */
  PERF_STOP_MEASURING(PERFORMANCE_COUNTER_0_BASE);

  /* Print measurement report */
  //perf_print_formatted_report
	 //(PERFORMANCE_COUNTER_0_BASE,            
	  //ALT_CPU_FREQ,                 // defined in "system.h"
	  //3,                            // How many sections to print
	  //"fac5", "fac10", "fac15");    // Display-name of section(s).

  //printf("\n");

  //printf("factorial(5) = %x (hexadecimal)\n",fac5);
  //printf("factorial(10) = %x (hexadecimal)\n",fac10);
  //printf("factorial(15) = %x (hexadecimal)\n",fac15);

  /* Event loop never exits. */
  while (1);

  return 0;
}
