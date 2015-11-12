#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <easynmc/easynmc.h>
#include <easynmc/aura.h>

#define MAGIC 0xbeefbabe

static int output_started;
static int num_objects; 

struct aura_message_header {
	int magic; 
	int id;
};

int aura_init()
{
	struct aura_export_table *etbl;
	etbl = (struct aura_export_table *) &g_aura_header;
	struct aura_object *o = etbl->objs;
	for(;;) { 
		if (!((o->type == AURA_OBJECT_EVENT) ||
		      (o->type == AURA_OBJECT_METHOD)))
			break;
		o->id = num_objects++;
		o++;
	}
}

static unsigned int *port    = (unsigned int *) 0x0800A403;

void aura_loop_once()
{
	struct aura_export_table *etbl = (struct aura_export_table *) &g_aura_header;

	if (g_aura_syncbuf.state != SYNCBUF_ARGOUT)
		return;
	
	int id = g_aura_syncbuf.id;
	struct aura_object *o = &etbl->objs[id];
	printf("NMC: Hadling id %d\n", id);
	o->handler((void *) g_aura_syncbuf.outbound_buffer_ptr, 
		   (void *) g_aura_syncbuf.inbound_buffer_ptr);
	g_aura_syncbuf.state = SYNCBUF_RETIN;
	/* Notify ARM */
	easynmc_send_HPINT();
}

void aura_loop_forever()
{
	for(;;) {
		int i;
		aura_loop_once();
		for (i=0; i<10; i++) { }
	}
}
