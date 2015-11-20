#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <easynmc/easynmc.h>
#include <easynmc/aura.h>


unsigned int *pinmux  = (unsigned int *) 0x0800CC21;
unsigned int *port    = (unsigned int *) 0x0800A403;
unsigned int *ddr     = (unsigned int *) 0x0800A407;

void aura_panic() { 
	for (;;) { 
		*port ^= (1<<6);	
		*port ^= (1<<7);	
	} ;
}

void aura_hexdump (char *desc, unsigned int *addr, int len) {
	int i; 
	for (i=0; i<len; i++) { 
		printf("0x%x: 0x%x\n", ((unsigned int)addr << 2), *addr);
		addr++;
	}
}


void echo_u32(void *in, void *out)
{
	unsigned int v = aura_get_u32();
	aura_put_u32(v);
}

void echo_u32u32(void *in, void *out)
{
	unsigned int v1 = aura_get_u32();
	unsigned int v2 = aura_get_u32();
	aura_put_u32(v1);
	aura_put_u32(v2);
}

void echo_bin(void *in, void *out)
{
	void *ptr = aura_get_bin(64);
	aura_put_bin(ptr, 64);
}

void echo_u64(void *in, void *out)
{
	unsigned long v = aura_get_u64();
	aura_put_u64(v);
}

void echo_buf(void *in, void *out)
{
	aura_buffer buf = aura_get_buf();
	int *ptr = aura_buffer_to_ptr(buf);
	if (*ptr == 0xdeadf00d)
		printf("NMC: Got expected data!\n");
	else
		printf("NMC: Got UNexpected data: 0x%x!\n", *ptr);
	aura_put_buf(buf);
}


int main(int argc, char **argv)
{
	printf("NMC: Aura RPC demo hello from nmc\n");
	aura_init();
	aura_loop_forever();

}

