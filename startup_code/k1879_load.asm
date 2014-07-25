// Main loop

const DEBUG = 1;

import from k1879_def.mlb;

global int_start_prog: label;

// Define constants for loader
K1879_DEF();

nobits ".bss"

LoaderStack: long[20];

end ".bss";

begin ".text"


/* TODO: Portability */	
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

	gr0 = LOADER_CODE_VERSION;	
	[NMC_CODEVERSION] = gr0;
	
	gr0 = STATE_READY;	
	[NMC_CORE_STATUS] = gr0;

	/* Call HPINT if host instructs us
	 * to do so
	 */
	
	gr7 = 1h;
	gr4 = [NMC_ISR_ON_START];
	with gr4 = gr4 and gr7;
	if =0 skip Main_loop;

	call sendHPINT;
	
.if DEBUG;
	// Режим GPIO TS2
	gr0 = [0_0800_CC21h];
	gr1 = 20h;
	gr0 = gr0 and not gr1;
	[0_0800_CC21h] = gr0;

	// Выводы TS3_D <6, 7> на выход?
	gr0 = [0_0800_A407h];
	gr1 = 0C0h;
	gr0 = gr0 or gr1;
	[0_0800_A407h] = gr0;
.endif;


<Main_loop>

	/* Tell the host we're ready */
	gr0 = STATE_READY;	
	[NMC_CORE_STATUS] = gr0;

	/* Loop on some dump arithmetics to allow
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
	/* fetch return value ? */	
	goto int_start_prog;

end ".text";