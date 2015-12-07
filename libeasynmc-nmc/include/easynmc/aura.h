#ifndef AURA_H
#define AURA_H

#define AURA_MAGIC_HDR 0xdeadf00d
#define AURA_OBJECT_EVENT  0xdeadc0de
#define AURA_OBJECT_METHOD 0xdeadbeaf

#define AURA_STRLEN 16 

#define U32  "3"
#define U64  "4"

#define S32  "8"
#define S64  "9"

#define BUFFER  "b"

#define BIN(len) "s" #len "."

enum { 
	AURA_SYNC_OWNER_HOST = 0,
	AURA_SYNC_OWNER_NMC = 1,
};

struct aura_object {
	unsigned int type;
	unsigned int id;
	char name[AURA_STRLEN];
	char argfmt[AURA_STRLEN];
	char retfmt[AURA_STRLEN];
	void (*handler)(void *in, void *out);
};

struct aura_header {
	unsigned int magic;
	unsigned int strlen;
};

struct aura_export_table {
	struct aura_header hdr; 
	struct aura_object objs[];
};

enum { 
	SYNCBUF_IDLE = 0,
	SYNCBUF_ARGOUT = 1,
	SYNCBUF_RETIN = 2
};

struct aura_sync_buffer { 
	unsigned int state;
	unsigned int id;
	unsigned int inbound_buffer_ptr; /* From NMC to host */
	unsigned int outbound_buffer_ptr; /* From host to nmc */
};

extern struct aura_header g_aura_header;
extern struct aura_sync_buffer g_aura_syncbuf;

void aura_panic();

void aura_loop_forever();
void aura_loop_once();

int aura_init();

typedef unsigned long aura_buffer;

#define aura_buffer_to_ptr(buf) ((void *) (buf & 0xffffffffUL))
 
#define GETFUNC(type) (* (type *) in); in = ((char *) in) + sizeof(type)
#define PUTFUNC(type, value) (* (type *) out) = value; out = ((char *) out) + sizeof(type)

#define aura_get_u32() GETFUNC(unsigned int)
#define aura_get_s32() GETFUNC(int)
#define aura_get_u64() GETFUNC(unsigned long)
#define aura_get_s64() GETFUNC(long)
#define aura_get_bin(len) in; in = (char *) in + (len / 4)
#define aura_get_buf() GETFUNC(unsigned long)

#define aura_put_u32(v) PUTFUNC(unsigned int, v)
#define aura_put_s32(v) PUTFUNC(int, v)
#define aura_put_u64(v) PUTFUNC(unsigned long, v)
#define aura_put_s64(v) PUTFUNC(long, v)
#define aura_put_bin(buf, len) memcpy(out, buf, (len / 4)), out = (char *) out + (len / 4)
#define aura_put_buf(v) PUTFUNC(unsigned long, v)

#define AURA_METHOD(func, nm, arg, ret)		\
	extern void func(void *in, void *out);	\
	struct aura_object g_export_ ## nm = {	\
		.type = AURA_OBJECT_METHOD,	\
		.name = #nm,			\
		.argfmt = "" arg,		\
		.retfmt = "" ret,		\
		.handler = func			\
	};

#endif
