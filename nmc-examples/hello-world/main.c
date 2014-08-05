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
	printf("Hello world from NeuroMatrix! I am the NMC printf'ing to you!\n");
	return 0; 
} 

