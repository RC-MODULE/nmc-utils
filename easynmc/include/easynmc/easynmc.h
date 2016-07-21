#ifndef EASYNMC_H_H
#define EASYNMC_H_H

#include <stdarg.h>

/* Return count in buffer.  */
#define CIRC_CNT(head,tail,size) (((head) - (tail)) & ((size)-1))

/* Return space available, 0..size-1.  We always leave one free char
   as a completely full buffer has head == tail, which is the same as
   empty.  */
#define CIRC_SPACE(head,tail,size) CIRC_CNT((tail),((head)+1),(size))

/* Return count up to the end of the buffer.  Carefully avoid
   accessing head and tail more than once, so they can change
   underneath us without returning inconsistent results.  */
inline int CIRC_CNT_TO_END(int head, int tail,int size) 
{ 
	int end = (size) - (tail);	       
	int n = ((head) + end) & ((size)-1); 
	return n < end ? n : end;
}

/* Return space available up to the end of the buffer.  */
inline int CIRC_SPACE_TO_END(int head, int tail, int size)
{
	int end = (size) - 1 - (head);	       
	int n = (end + (tail)) & ((size)-1);   
	return n <= end ? n : end+1;
}

struct nmc_stdio_channel;

struct nmc_stdio_channel {
	unsigned int isr_on_io;
	unsigned int size;
	unsigned int head;
	unsigned int tail;
	unsigned char data; //first word
};


#define min_t(type, a, b) (((type)(a)<(type)(b))?(type)(a):(type)(b))
#define max_t(type, a, b) (((type)(a)>(type)(b))?(type)(a):(type)(b))


extern struct nmc_stdio_channel easynmc_stdout_hdr;
extern struct nmc_stdio_channel easynmc_stdin_hdr;

#define stdin  (&easynmc_stdin_hdr)
#define stdout (&easynmc_stdout_hdr)

void easynmc_send_LPINT(void);
void easynmc_send_HPINT(void);



void eputc(struct nmc_stdio_channel *ch, char c);
void eputs(struct nmc_stdio_channel *ch, char *str);
char egetc(struct nmc_stdio_channel *ch);
int eread(struct nmc_stdio_channel *ch, char* dst, int size);
int ewrite(struct nmc_stdio_channel *ch, char* src, int size);
void eputc_noint(struct nmc_stdio_channel *ch, char c);
void eputc_smart(struct nmc_stdio_channel *ch, char c);
int printf(const char* format,...);
int eprintf(struct nmc_stdio_channel *ch, const char* format,...);
int evsprintf(struct nmc_stdio_channel *chan, const char* format, va_list args );

char getc();
void putc(char ch);
void puts(char *str);

#endif
