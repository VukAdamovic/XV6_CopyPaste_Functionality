// Console input and output.
// Input is from the keyboard or serial port.
// Output is written to the screen and serial port.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "traps.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"

static void consputc(int);

static int panicked = 0;

static struct {
	struct spinlock lock;
	int locking;
} cons;

static void
printint(int xx, int base, int sign)
{
	static char digits[] = "0123456789abcdef";
	char buf[16];
	int i;
	uint x;

	if(sign && (sign = xx < 0))
		x = -xx;
	else
		x = xx;

	i = 0;
	do{
		buf[i++] = digits[x % base];
	}while((x /= base) != 0);

	if(sign)
		buf[i++] = '-';

	while(--i >= 0)
		consputc(buf[i]);
}

// Print to the console. only understands %d, %x, %p, %s.
void
cprintf(char *fmt, ...)
{
	int i, c, locking;
	uint *argp;
	char *s;

	locking = cons.locking;
	if(locking)
		acquire(&cons.lock);

	if (fmt == 0)
		panic("null fmt");

	argp = (uint*)(void*)(&fmt + 1);
	for(i = 0; (c = fmt[i] & 0xff) != 0; i++){
		if(c != '%'){
			consputc(c);
			continue;
		}
		c = fmt[++i] & 0xff;
		if(c == 0)
			break;
		switch(c){
		case 'd':
			printint(*argp++, 10, 1);
			break;
		case 'x':
		case 'p':
			printint(*argp++, 16, 0);
			break;
		case 's':
			if((s = (char*)*argp++) == 0)
				s = "(null)";
			for(; *s; s++)
				consputc(*s);
			break;
		case '%':
			consputc('%');
			break;
		default:
			// Print unknown % sequence to draw attention.
			consputc('%');
			consputc(c);
			break;
		}
	}

	if(locking)
		release(&cons.lock);
}

void
panic(char *s)
{
	int i;
	uint pcs[10];

	cli();
	cons.locking = 0;
	// use lapiccpunum so that we can call panic from mycpu()
	cprintf("lapicid %d: panic: ", lapicid());
	cprintf(s);
	cprintf("\n");
	getcallerpcs(&s, pcs);
	for(i=0; i<10; i++)
		cprintf(" %p", pcs[i]);
	panicked = 1; // freeze other CPU
	for(;;)
		;
}

#define BACKSPACE 0x100
#define CRTPORT 0x3d4
static ushort *crt = (ushort*)P2V(0xb8000);  // CGA memory
extern int copy_mode;
extern int cursor;
extern int flag_C;
extern int flag_In;
extern int paste_mode;
extern int flag_P;
char niz_Copy [128];
static int flag_q_e = 0; // handle za q i e (ono sto je u tekstu specifirano)
int counter_CopyNiz = -1;

static void
cgaputc(int c)
{
	int pos;
	static int last_pos;  // promenljiva za vracanje pozicije kursora
	static int start_pos = -1; // promenljiva za caku kopiranja
	static int end_pos = -1; // promenljiva za caku kopiranja
	

	outb(CRTPORT, 14);
	pos = inb(CRTPORT+1) << 8;
	outb(CRTPORT, 15);
	pos |= inb(CRTPORT+1);
	
		
		
	if(copy_mode == 0 && cursor == 1){
		crt[pos] = (crt[pos]&0xff) | 0x0700;
		pos = last_pos;	
		cursor = 0;

	}else if(copy_mode == 1 && cursor == 1){
		last_pos = pos;
		cursor = 0;
	}
	

	if(c == '\n' ){
		if(copy_mode == 0){
			pos += 80 - pos%80;
			crt[pos] = ' ' | 0x0700;
		}
	}

	else if(c == BACKSPACE ){
		if(pos > 0 && copy_mode == 0) {
			crt[--pos] = ' ' | 0x0700;  //* bez ove linije ne bi mi radio backspace    
		}
	} 

	else {
		
		if(copy_mode == 0){ // ako nije copy mode radi normalno
			if(flag_C == 0 && flag_P == 0){
				crt[pos++] = (c&0xff) | 0x0700; 
				crt[pos] = ' ' | 0x0700;
			}else{ // handle da ne  C || P kada uradim ALT +C || P
			 	flag_C = 0;
				flag_P = 0;
			}
			if(counter_CopyNiz >=0 && paste_mode == 1){ 
				for(int i = 0; i < counter_CopyNiz; i++){
					crt[pos++] = (niz_Copy[i]&0xff) |0x0700;	
				}
				crt[pos] = ' ' | 0x0700;
				paste_mode = 0; 
			}
		}
		else if(copy_mode == 1){ // ako je copy mode oboji kursor u belo 
			crt[pos] = (crt[pos]&0xff) | 0x7000;
		}

		
		if(copy_mode && c =='q' && flag_q_e == 0){ //korisnik je pritisuo q u copy rezimu
			start_pos = pos; 
			//counter_CopyNiz = 0;
			flag_q_e = 1;
			flag_In = !flag_In;
		}
		if(copy_mode  && c == 'e' && flag_q_e == 1){ //korisnik je pritisnuo e copy rezimu
			end_pos = pos;
			flag_In = 0;
			counter_CopyNiz = 0;
			flag_q_e = 0;
			for(int i = 0; i < 2000 ; i++){	 			
				crt[i] = (crt[i]&0xff) | 0x0700; // da se sva slova vrate u normalu
			}
		}





		if(copy_mode && c =='a'){
			if(pos - 1 >= 0){
				pos = pos - 1;
				for(int i = 0; i < 2000 ; i++){
					if(i == pos && start_pos == -1){ // ako nismo pritisnuli q oboji trenutni
						crt[pos] = (crt[pos]&0xff) | 0x7000;
					}
					else if(i != pos && start_pos == -1){//sve ostali su crne pozdine bela slova
						crt[i] = (crt[i]&0xff) | 0x0700;
					}
				}
			}
		}

		if(copy_mode && c =='d'){
			if(pos + 1  <= 2000){
				pos = pos + 1;
				for(int i = 0; i < 2000 ; i++){
					if(i == pos && start_pos == -1){
						crt[pos] = (crt[pos]&0xff) | 0x7000;
					}else if(i != pos && start_pos == -1){
						crt[i] = (crt[i]&0xff) | 0x0700;
					}
				}
			}
		}

		if(copy_mode && c =='s'){
			if(pos+80 < 2000){
				pos = pos + 80;
				for(int i = 0; i < 2000 ; i++){
					if(i == pos && start_pos == -1){
						crt[pos] = (crt[pos]&0xff) | 0x7000;
					}else if(i != pos && start_pos == -1){
						crt[i] = (crt[i]&0xff) | 0x0700;
					}
				}
			}
		}

		if(copy_mode && c =='w'){
			if(pos - 80 >= 0){
				pos = pos - 80;
				for(int i = 0; i < 2000 ; i++){
					if(i == pos && start_pos == -1){
						crt[pos] = (crt[pos]&0xff) | 0x7000;
					}else if(i != pos && start_pos == -1){
						crt[i] = (crt[i]&0xff) | 0x0700;
					}
				}
			}
		}
	}



	if(copy_mode && start_pos != -1 && end_pos == -1){ // ako smo samo pritisnuli q
		if(start_pos <= pos){ // ona caka kod kopiranja
			for(int i = 0; i < 2000 ; i++){
				if(i >= start_pos && i <= pos){  
					crt[i] = (crt[i]&0xff) | 0x7000;				
				}else{
					crt[i] = (crt[i]&0xff) | 0x0700;
				}
			}
		}
		else if(start_pos > pos){ // ona caka kod kopiranja
			for(int i = 0; i < 2000 ; i++){
				if(i >= pos && i < start_pos){ 
					crt[i] = (crt[i]&0xff) | 0x7000;				
				}else{
					crt[i] = (crt[i]&0xff) | 0x0700;
				}
			}		
		}
	}
	else if(copy_mode && start_pos != -1 && end_pos != -1){ // ako smo sel i pritisnuli e
		if((start_pos <= end_pos) && (end_pos - start_pos < 128)){
			for(int i = 0; i < 2000 ; i++){ 
				if(i >= start_pos && i <= end_pos){
					niz_Copy[counter_CopyNiz++] =crt[i];	 			
				}
				crt[i] = (crt[i]&0xff) | 0x0700; // vrati na normalu boju
			}
		}
		else if((start_pos > end_pos) && (start_pos - end_pos < 128)){
			for(int i = 0; i < 2000 ; i++){
				if(i >= end_pos && i < start_pos){  
					niz_Copy[counter_CopyNiz++] = crt[i];	 			
				}
				crt[i] = (crt[i]&0xff) | 0x0700; // vrati na normalu boju
			}
		}
		start_pos = -1;  //vrati  
		end_pos = -1;	//vrati
		
	}

	
	if(pos < 0 || pos > 25*80)
		panic("pos under/overflow");

	if((pos/80) >= 23 && copy_mode == 0){  // Scroll up. 
		memmove(crt, crt+80, sizeof(crt[0])*23*80);     //23
		pos -= 80;
		memset(crt+pos, 0, sizeof(crt[0])*(24*80 - pos)); // 24
	}
	
	
	outb(CRTPORT, 14);
	outb(CRTPORT+1, pos>>8);
	outb(CRTPORT, 15);
	outb(CRTPORT+1, pos);



}

void
consputc(int c)
{
	if(panicked){
		cli();
		for(;;)
			;
	}

	if(c == BACKSPACE){
		uartputc('\b'); uartputc(' '); uartputc('\b');
	} else
		uartputc(c);
	cgaputc(c);
}

#define INPUT_BUF 128
struct {
	char buf[INPUT_BUF];
	uint r;  // Read index
	uint w;  // Write index
	uint e;  // Edit index
} input;

#define C(x)  ((x)-'@')  // Control-x

void
consoleintr(int (*getc)(void))
{
	int c, doprocdump = 0;
	//c je ascii karakter
	acquire(&cons.lock);
	while((c = getc()) >= 0){
		switch(c){
		case C('P'):  // Process listing.
			// procdump() locks cons.lock indirectly; invoke later
			doprocdump = 1;
			break;
		case C('U'):  // Kill line.
			while(input.e != input.w &&
			      input.buf[(input.e-1) % INPUT_BUF] != '\n'){
				input.e--;
				consputc(BACKSPACE);
			}
			break;
		case C('H'): case '\x7f':  // Backspace
			if(input.e != input.w){
				input.e--;
				consputc(BACKSPACE);
			}
			break;
		default:
			if(c != 0 && input.e-input.r < INPUT_BUF){
				c = (c == '\r') ? '\n' : c;
				if(copy_mode == 0 && flag_C == 0 && flag_P == 0){  // ako smo u normalnom rezimu neka mi dodaje u buffer 
					input.buf[input.e++ % INPUT_BUF] = c;
				}
				if(paste_mode == 1 && counter_CopyNiz >= 0) { //ako moj niz nije prazan && ako je paste_mode = 1  dodaj u buffer	//
					for(int i = 0; i < counter_CopyNiz; i++){
						input.buf[input.e++ % INPUT_BUF] = niz_Copy[i];
					}
					if(input.e - input.w > 127){
						while(counter_CopyNiz - (input.e - input.w) > 127){
							input.e--;
							consputc(BACKSPACE);
						}
					}
						
				}
				
				consputc(c);

					
				if(c == '\n' || c == C('D') || input.e == input.r+INPUT_BUF){
					input.w = input.e;
					wakeup(&input.r);
				}
			}
			break;
		}
	}
	release(&cons.lock);
	if(doprocdump) {
		procdump();  // now call procdump() wo. cons.lock held
	}
}

int
consoleread(struct inode *ip, char *dst, int n) //dst moze da mi bude jedan karakter a moze biti i niz  | n predstavlja duzinu niza 
{
	uint target;
	int c;

	iunlock(ip);
	target = n;
	acquire(&cons.lock);
	while(n > 0){
		while(input.r == input.w){
			if(myproc()->killed){
				release(&cons.lock);
				ilock(ip);
				return -1;
			}
			sleep(&input.r, &cons.lock);
		}

		c = input.buf[input.r++ % INPUT_BUF];
		
		
		if(c == C('D')){  // EOF
			if(n < target){
				// Save ^D for next time, to make sure
				// caller gets a 0-byte result.
				input.r--;
			}
			break;
		}
		*dst++ = c; //ovo znaci gde ti je kursor tu ce upisati taj karakter
		--n;
		if(c == '\n')
			break;
	}
	release(&cons.lock);
	ilock(ip);

	return target - n;
}

int
consolewrite(struct inode *ip, char *buf, int n)
{
	int i;

	iunlock(ip);
	acquire(&cons.lock);
	for(i = 0; i < n; i++)
		consputc(buf[i] & 0xff);
	release(&cons.lock);
	ilock(ip);

	return n;
}

void
consoleinit(void)
{
	initlock(&cons.lock, "console");

	devsw[CONSOLE].write = consolewrite;
	devsw[CONSOLE].read = consoleread;
	cons.locking = 1;

	ioapicenable(IRQ_KBD, 0);
}

