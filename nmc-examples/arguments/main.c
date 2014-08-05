#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <easynmc/easynmc.h>

void sleep(int k){
	int f=3;
	for(int i=0; i<k; i++)
		for(int j=0; j<1000; j++)
			for(int k=0; k<10; k++){
				f*=f;
			}

}

/* bits 6 & 7 are leds on MB77.07 board (FJ GPIO) 
 * Please note, that NeuroMatrix doesn't have byte addressing
 * So the adresses below will NOT work if you want to run that on 
 * ARM core.
 */

unsigned int *pinmux  = (unsigned int *) 0x0800CC21;
unsigned int *port    = (unsigned int *) 0x0800A403;
unsigned int *ddr     = (unsigned int *) 0x0800A407;


/* 
   This application will print out all the arguments one by one 
   with a small delay, blinking leds along the way. 
   The led code assumes this code is run on MB77.07
*/

int main(int argc, char** argv)
{  

	*pinmux &= ~(1<<5);          /* Set TS2 to GPIO mode */
	*ddr |= ((1<<6) | (1<<7));   /* Set mode to output   */ 
	*port &= ~((1<<6) | (1<<7)); /* Turn leds off        */

	printf("Hello world! I am the NMC blinking ledz!\n");
	printf("I have been given %d arguments\n", argc);
	int i;
	for (i=0; i<argc; i++) {
		printf("Argument %d is %s\n", i, argv[i]);
		sleep(100);		
		*port ^= 1<<6; 
		sleep(100);		
		*port ^= 1<<7; 
	}

	return argc; 
} 

