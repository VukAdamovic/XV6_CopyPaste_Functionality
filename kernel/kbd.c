 #include "types.h"
#include "x86.h"
#include "defs.h"
#include "kbd.h"

int copy_mode = 0;
int cursor = 0;
int flag_C = 0;  // handle da ne ispisuje C kad izadjem iz rezima
int flag_P = 0; // handle da ne ispisuje P kad izadjem iz rezima
int flag_In = 0; // kada sam u copy mode i pritisnuo q 
int paste_mode = 0; 
extern int counter_CopyNiz;
int
kbdgetc(void)
{
	static uint shift;
	static uchar *charcode[4] = {
		normalmap, shiftmap, ctlmap, ctlmap
	};
	uint st, data, c;

	st = inb(KBSTATP);
	if((st & KBS_DIB) == 0)
		return -1;
	data = inb(KBDATAP);

	if(data == 0xE0){
		shift |= E0ESC;
		return 0;
	} else if(data & 0x80){
		// Key released
		data = (shift & E0ESC ? data : data & 0x7F);
		shift &= ~(shiftcode[data] | E0ESC);
		return 0;
	} else if(shift & E0ESC){
		// Last character was an E0 escape; or with 0x80
		data |= 0x80;
		shift &= ~E0ESC;
	}

	shift |= shiftcode[data]; // ovde znamo koje smo dugme pritisnuli od onih definisanih  | sa odredjenom maskom
	shift ^= togglecode[data]; 
	c = charcode[shift & (CTL | SHIFT)][data]; //ovde izvlacis za odg scan code iz mape asci vrednost
	if(shift & CAPSLOCK){
		if('a' <= c && c <= 'z')
			c += 'A' - 'a';
		else if('A' <= c && c <= 'Z')
			c += 'a' - 'A';
	}

	if((shift & (ALT | SHIFT)) && (c =='C') && flag_In == 0){ 
		copy_mode = !copy_mode; 
		cursor = 1;
		flag_C = 1;
	}
	if ((shift & (ALT| SHIFT)) && (c =='P') && copy_mode == 0){
		if(counter_CopyNiz >= 0){
			paste_mode = 1;
			flag_P = 1;
		}else{
			flag_P = 1;		
		}
	}

	return c;
}

void
kbdintr(void)
{
	consoleintr(kbdgetc);
}
