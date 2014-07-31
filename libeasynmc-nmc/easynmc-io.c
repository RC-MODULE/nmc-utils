#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <easynmc.h>


void eputc_noint(struct nmc_stdio_channel *ch, char c)
{
	unsigned char *data;
	data = &ch->data;
	while (!CIRC_SPACE_TO_END(ch->head, ch->tail, ch->size));;
	data[ch->head++] = c;
	ch->head &= (ch->size -1);
}


void eputc(struct nmc_stdio_channel *ch, char c) 
{
	unsigned char *data;
	data = &ch->data;
	while (!CIRC_SPACE_TO_END(ch->head, ch->tail, ch->size));;
	data[ch->head++] = c;
	ch->head &= (ch->size -1);
	if (ch->isr_on_io) 
		easynmc_send_LPINT();
}


void eputs(struct nmc_stdio_channel *ch, char *str) 
{
	unsigned char *data;
	data = &ch->data;
	do { 
		while (!CIRC_SPACE_TO_END(ch->head, ch->tail, ch->size));;
		data[ch->head++] = *str;
		ch->head &= (ch->size -1);
	}
	while (*str++);
	
	if (ch->isr_on_io) 
		easynmc_send_LPINT();	
}

char egetc(struct nmc_stdio_channel *ch) 
{
	char ret;
	unsigned char *data;
	data = &ch->data;
	while (!CIRC_CNT_TO_END(ch->head, ch->tail, ch->size));;
	ret = data[ch->tail++];
	ch->tail &= (ch->size -1);
	if (ch->isr_on_io) 
		easynmc_send_LPINT();
	return ret;
}

int eread(struct nmc_stdio_channel *ch, char* dst, int size) 
{
	unsigned char *data;
	int ret = size;
	data = &ch->data;
	while (size) { 
		int tocopy, n;
		while (! ( n = CIRC_CNT_TO_END(ch->head, ch->tail, ch->size)));;
		tocopy = min_t(int, n, size);
		memcpy(dst, &data[ch->tail], tocopy);
		size -= tocopy;
		dst += tocopy;
		ch->tail += tocopy;
		ch->tail &= (ch->size -1);
	}

	if (ch->isr_on_io) 
		easynmc_send_LPINT();

	return ret;
};


int ewrite(struct nmc_stdio_channel *ch, char* src, int size) 
{
	int ret = size;
	unsigned char *data;
	data = &ch->data;
	while (size) { 
		int tocopy, n;
		while (! ( n = CIRC_SPACE_TO_END(ch->head, ch->tail, ch->size)));;
		tocopy = min_t(int, n, size);
		memcpy(&data[ch->tail], src, tocopy);
		size -= tocopy;
		src += tocopy;
		ch->tail += tocopy;
		ch->tail &= (ch->size -1);
	}
	
	if (ch->isr_on_io) 
		easynmc_send_LPINT();
	
	return ret;	
};


int printf(const char* format,...)
{
    va_list argptr;
    int r;
    va_start(argptr, format);
    r = evsprintf( stdout, format, argptr);
    va_end( argptr );
    if (stdout->isr_on_io) 
	    easynmc_send_LPINT();
    return r;
}

char getc()
{
	return egetc(&easynmc_stdin_hdr);
}

void putc(char ch) 
{
	eputc(&easynmc_stdout_hdr, ch);
}

void puts(char *str)
{
	eputs(&easynmc_stdout_hdr, str);	
}


