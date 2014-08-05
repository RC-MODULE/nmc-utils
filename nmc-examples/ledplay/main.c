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


int main(int argc, char** argv)
{  

	*pinmux &= ~(1<<5);          /* Set TS2 to GPIO mode */
	*ddr |= ((1<<6) | (1<<7));   /* Set mode to output   */ 
	*port &= ~((1<<6) | (1<<7)); /* Turn leds off        */

	printf("Hello world! I am the NMC blinking ledz app!\n");
	printf("Press 'a' to turn off LED1\n", argc);
	printf("Press 'A' to turn on  LED1\n", argc);
	printf("Press 's' to turn off LED2\n", argc);
	printf("Press 'S' to turn on  LED2\n", argc);

	while (1) {
		int a = getc();
		switch(a) {
		case 'a':
			*port &= ~(1<<6);
			break;
		case 'A':
			*port |= (1<<6);
			break;
		case 's':
			*port &= ~(1<<7);
			break;
		case 'S':
			*port |= (1<<7);
			break;

		};
	}

	return 0; 
} 

