// Main loop


begin ".text"


global _easynmc_send_LPINT: label;
global _easynmc_send_HPINT: label;
global stop: label;
	
<_easynmc_send_LPINT>
	push gr7;
	gr7 = 24h;
	nmscu = gr7;
	pop gr7;
	return;
	
<_easynmc_send_HPINT>
	push gr7;
	gr7 = 48h;
	nmscu = gr7;
	pop gr7;
	return;

<stop>
	skip 0;
	
end ".text";


begin ".init"
	gr0 = 2;
	push gr0;
	gr0 = 3	;
	push gr0;
end ".init";
	
const EASYNMC_IOBUFLEN = 128;

//global _easynmc_stdout_hdr: label;
//global _easynmc_stdin_hdr: label;
	

begin ".easynmc_stdin"
global _easynmc_stdin_hdr: word[4] = (
	0h,                 /* ISR on IO */
	EASYNMC_IOBUFLEN,   /* size */
	0h,                 /* head */
	0h                  /* tail */
);
_easynmc_stdin_data:	word[EASYNMC_IOBUFLEN]; /* data */
end ".easynmc_stdin";



begin ".easynmc_stdout"
global _easynmc_stdout_hdr: word[4] = (
	0h,                 /* ISR on IO */
	EASYNMC_IOBUFLEN,   /* size */
	0h,                 /* head */
	0h                  /* tail */
);
_easynmc_stdout_data:	word[EASYNMC_IOBUFLEN]; /* data */
end ".easynmc_stdout";

	
//begin ".easynmc_stdout"
//_easynmc_stdout: IOChannel	;
//end ".easynmc_stdout";


/* Legacy barrier synchronisation stuff */
struct SyncroBlock
  syncFlag:   word; // Syncro flag
  value:      word; // Sync value
  array_addr: word; // Sync array address
  array_len:  word; // Sync array length in 32-bit words
end SyncroBlock;

//begin ".easynmc_sync_tonmc";
//_easynmc_sync_tonmc: SyncroBlock;
//end   ".easynmc_sync_tonmc";

//begin ".easynmc_sync_fromnmc";
//_easynmc_sync_fromnmc: SyncroBlock;
//end   ".easynmc_sync_fromnmc";