// Main loop

import from conf.mlb;
DECLARE_DEBUG()
import from k1879_def.mlb;

global int_start_prog: label;

// Define constants for loader
K1879_DEF();

nobits ".bss"

LoaderStack: long[20];

end ".bss";

begin ".text"


/* TODO: Portability? */
<_VEC_Reset>
.wait;
	push ar0,gr0;
	push ar6,gr6;

	// afifo flush
	nul;
	nul;
	gr6 = 0000_0100_0000_0000b; // bit-10 afifo not empty flag
	gr0 = intr;
	gr7 = gr0 and gr6;
	with gr7;

	if <>0 goto IsWfifoEmpty;	// if afifo is empty
	rep 32 [ar7]=afifo;			// clear afifo
	
	// wfifo flush
	<IsWfifoEmpty>
		gr6 = 0100_0000_0000_0000_0000_0000b; // bit-22 wfifo not empty flag
		gr0 = intr;
		gr7 = gr0 and gr6;
		with gr7;
		if <>0 goto End; 		// if wfifo is empty
		sb=0;
		ftw;					// read 1 long word from wfifo
		wtw;
		nul;
		nul;
		nul;
		nul;
		nul;
		nul;
	goto IsWfifoEmpty;
	<End>
	gr0 = 0100_0000_0000_0100_0000_0000b; // bit-22|10
	gr7 = intr;
	gr7 = gr7 and gr0;
	gr7 = gr7 xor gr0;
	pop ar6,gr6;
	pop ar0,gr0;
	return;
	
<sendLPINT>
	gr7 = 24h;
	nmscu = gr7;
	return;
	
<sendHPINT>
	gr7 = 48h;
	nmscu = gr7;
	return;
	
<int_start_prog>	
	ar7 = LoaderStack;

	/* Fetch the return code, if any */
	[NMC_PROG_RETURN] = gr7;
	
	gr0 = LOADER_CODE_VERSION;	
	[NMC_CODEVERSION] = gr0;
	
	gr0 = STATE_READY;	
	[NMC_CORE_STATUS] = gr0;

	/* Reset Vector core, otherwise we can hang up */
	call _VEC_Reset;

.if DEBUG;
	// TS2 as GPIO
	gr0 = [0_0800_CC21h];
	gr1 = 20h;
	gr0 = gr0 and not gr1;
	[0_0800_CC21h] = gr0;

	// Bits <6, 7> OUTPUT
	gr0 = [0_0800_A407h];
	gr1 = 0C0h;
	gr0 = gr0 or gr1;
	[0_0800_A407h] = gr0;
.endif; 
	
	/* Call HPINT if host instructs us
	 * to do so
	 */
	
	gr7 = 1h;
	gr4 = [NMC_ISR_ON_START];
	with gr4 = gr4 and gr7;
	if =0 skip Main_loop;

	call sendHPINT;

<Main_loop>

	/* Tell the host we're ready */
	gr0 = STATE_READY;	
	[NMC_CORE_STATUS] = gr0;

	/* Loop on some dumb arithmetics to allow
	 * edcl get access to the IM bank
	*/
	
	gr4 = false;	
	gr5 = false;				
	gr6 = false;
	gr4 = gr4 + gr5;
	gr5 = gr6 + gr5;
	gr4 = gr4 + gr5;
	gr5 = gr6 + gr5;
	gr4 = gr4 + gr5;
	gr5 = gr6 + gr5;
	gr4 = gr4 + gr5;
	gr5 = gr6 + gr5;

	
.if DEBUG;
	// First LED on
	gr1 = 40h;
	gr0 = [0_0800_A403h];
	gr0 = gr0 or gr1;
	[0_0800_A403h] = gr0;

	gr2 = 200000h;
<First_LED_on_loop>
	gr2--;
	if > skip First_LED_on_loop;

	// First LED off
	gr1 = 40h;
	gr0 = [0_0800_A403h];
	gr0 = gr0 and not gr1;
	[0_0800_A403h] = gr0;

	gr2 = 2000000h;
<First_LED_off_loop>
	gr2--;
	if > skip First_LED_off_loop;
.endif;

	gr7 = 1h;
	gr4 = [NMC_CORE_START];
	with gr4 = gr4 and gr7;
	if =0 skip Main_loop;

	/* Update our state */
	gr0 = STATE_RUNNING;
	gr1 = 0h	   ;
	
	[NMC_CORE_STATUS] = gr0;
	[NMC_CORE_START]  = gr1;
	
        /* load & call the rogram entry point */
	ar4 = [NMC_PROG_ENTRY];
	call ar4;
	[NMC_PROG_ENTRY] = gr7	;

	goto int_start_prog;

end ".text";