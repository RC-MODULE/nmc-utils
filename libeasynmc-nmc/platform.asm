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
	goto 0h;

end ".text";
