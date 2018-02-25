#include <stdio.h>
#include "system.h"
#include "altera_avalon_pio_regs.h"
#include "altera_avalon_performance_counter.h"
#include "images.h"

#define TRUE 1
#define DEBUG 0
#define SECTION_1 1


#define STATE_1 1
#define STATE_2 2
#define STATE_3 3


unsigned char ENABLE = 0;
unsigned char hmin = 0, hmax = 0;

extern void delay (int millisec);

void sram2sm_p3(unsigned char* base)
{
	int x, y;
	unsigned char* shared;

	shared = (unsigned char*) SHARED_ONCHIP_BASE+5; //1 cell offset from the semaphores

	int size_x = *base++;
	int size_y = *base++;
	int max_col= *base++;
	*shared++  = size_x;
	*shared++  = size_y;
	*shared++  = max_col;
	//printf("The image is: %d x %d!! \n", size_x, size_y);
	for(y = 0; y < size_y; y++)
	for(x = 0; x < size_x; x++)
	{
		*shared++ = *base++; 	// R
		*shared++ = *base++;	// G
		*shared++ = *base++;	// B
	}
}

void sram2sm_p32(unsigned char* base)
{
	int x;
	unsigned char* shared;

	shared = (unsigned char*) SHARED_ONCHIP_BASE+5; //1 cell offset from the semaphores
	unsigned int* mem = (unsigned int*) shared+3;

	int size_x = *base++;
	int size_y = *base++;
	int max_col= *base++;
	int size = (size_x*size_y*3)/4;
	*shared++  = size_x;
	*shared++  = size_y;
	*shared++  = max_col;
	//printf("The image is: %d x %d!! \n", size_x, size_y);
	//for(y = 0; y < size_y; y++)
	//for(x = 0; x < size_x; x++)
	//{
		//*shared++ = *base++; 	// R
		//*shared++ = *base++;	// G
		//*shared++ = *base++;	// B
	//}
	for(x = 0; x < size; x++){
		*mem++ = (((int)*base++)<<24)|(((int)*base++)<<16)|(((int)*base++)<<8)|(*base++);
		//*mem++ = ((int)*base++)|(((int)*base++)<<8)|(((int)*base++)<<16)|(((int)*base++)<<24);
	}
}

#if DEBUG
void splitImage(unsigned char i, unsigned char* shared_start){ //shared_start should be address 5000
	unsigned char lines_per_group = i/4;
	if(lines_per_group%2 == 0){ //if the division has an even result
		/* this doesn't consider divisions that have an even result and
		   reminder != 0 (reminder = 2 in this case), since there are
		   no images with this characteristic in the provided set */
		*shared_start = *(shared_start+1) = *(shared_start+2) = *(shared_start+3) = lines_per_group;
	}
	else{ //if the division has an odd result
		if(i%4 == 0){
			*shared_start = lines_per_group-1;
			*(shared_start+1) = lines_per_group+1;
			*(shared_start+2) = lines_per_group+1;
			*(shared_start+3) = lines_per_group-1;
		}
		else{
			*shared_start = lines_per_group+1;
			*(shared_start+1) = lines_per_group+1;
			*(shared_start+2) = lines_per_group-1;
			*(shared_start+3) = lines_per_group+1;
		}
	}
	//jump one memory location and start from shared_start+6
	unsigned char aux = (((*shared_start)+2)/2)-2;
	*(shared_start+6) = aux;
	*(shared_start+7) = aux = aux+((*(shared_start+1)+4)/2)-2; //each memory location contains the sum of the lines of the previous and the current
	*(shared_start+8) = aux = aux+((*(shared_start+2)+4)/2)-2;
	*(shared_start+9) = aux+((*(shared_start+3)+2)/2)-2;
}
#else
void splitImage(unsigned char* shared_start){
	unsigned char i = 32, lines_per_group = i/4, aux = ((lines_per_group+2)/2)-2;
	*shared_start = *(shared_start+1) = *(shared_start+2) = *(shared_start+3) = lines_per_group;
	
	*(shared_start+6) = aux;
	*(shared_start+7) = aux = aux+((lines_per_group+4)/2)-2; //each memory location contains the sum of the lines of the previous and the current
	*(shared_start+8) = aux = aux+((lines_per_group+4)/2)-2;
	*(shared_start+9) = aux+((lines_per_group+2)/2)-2;
}
#endif

int main()
{
	printf("Hello from cpu_0!\n");
  
	unsigned char* shared;

	shared = (unsigned char*) SHARED_ONCHIP_BASE;

  
	char current_image=0;
	unsigned char* C1= shared;
    unsigned char* C2= shared+1;
    unsigned char* C3= shared+2;
    unsigned char* C4= shared+3;
	
	
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
	
    int counter=0;
	while (TRUE) {
		/* Extract the x and y dimensions of the picture */
		unsigned char j = *img_array[current_image];
		unsigned char i = *(img_array[current_image]+1);
		
		
		/* Reset Performance Counter */
	    PERF_RESET(PERFORMANCE_COUNTER_0_BASE);  
	
	    /* Start Measuring */
	    PERF_START_MEASURING (PERFORMANCE_COUNTER_0_BASE);
	
	    /* Section 1 */
	    PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, SECTION_1);
		
		/* Measurement here */
		sram2sm_p3(img_array[current_image]);
		
		#if DEBUG
		splitImage(i, shared+5000);
		#else
		splitImage(shared+5000);
		#endif
		
		// Wait for all CPUs to come up
	while(!( STATE_1 == *C4)
	&&	( STATE_1 == *C3) 
	&&  ( STATE_1 == *C2) 
	&&	( STATE_1 == *C1))
	{}
		//printf("STATE 1 Passed!\n");
		
		// Send 4 Signals Work 1
		*C1=STATE_2;
		*C2=STATE_2;
		*C3=STATE_2;
		*C4=STATE_2;
		
	/* Wait for all to finish to take the processed image SRAM*/
	while(!( STATE_3 == *C4)
	&&	( STATE_3== *C3) 
	&&  ( STATE_3== *C2) 
	&&	( STATE_3== *C1))
	{}
		/* Print ASCII image */
		int ascii_x = j/2-2, ascii_size = /*(*(shared+5009))*/(i/2-2)*ascii_x;
		
		if(DEBUG){
			printf("---- ASCII Image ----\n");
			int z = 0;
			
			while(z < /*size4*/ascii_size){
				printf("%c", /*asciiImage[z]*/*(shared+6000+z));
				printf("%c", ' ');
				if((z+1)%ascii_x == 0 && z > 0)
					printf("\n");
				z++;
			}
			
			printf("\n\n");
		}
		
		PERF_END(PERFORMANCE_COUNTER_0_BASE, SECTION_1);
		
		/* End Measuring */
		PERF_STOP_MEASURING(PERFORMANCE_COUNTER_0_BASE);
		
		/* Print report */
		perf_print_formatted_report
		(PERFORMANCE_COUNTER_0_BASE,            
		ALT_CPU_FREQ,        // defined in "system.h"
		1,                   // How many sections to print
		"Section 1"        // Display-name of section(s).
		);   
		
		/* Increment the image pointer */
		current_image=(current_image+1) % number_of_images;
		counter++;
		
	}
	
	/* End Measuring */
	//PERF_STOP_MEASURING(PERFORMANCE_COUNTER_0_BASE);
	
	return 0;
}
