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

int main(int argc, char** argv)
{  

	printf("Hello world! I am the NMC app that will bug you with interrupts for eternity!\n");
	printf("Run me and use nmctl --mon to see what I'm doing \n", argc);

	while(1) { 
		easynmc_send_LPINT();
		sleep(100);		
		easynmc_send_HPINT();
		sleep(100);		
	}

	return argc; 
} 

