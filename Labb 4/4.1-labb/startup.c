/*
 * 	startup.c
 *
 */
void startup(void) __attribute__((naked)) __attribute__((section (".start_section")) );

void startup ( void )
{
__asm volatile(
	" LDR R0,=0x2001C000\n"		/* set stack */
	" MOV SP,R0\n"
	" BL main\n"				/* call main */
	"_exit: B .\n"				/* never return */
	) ;
}

#define DELAY_COUNT 100000
#define PORT_BASE 	 0x40020C00
#define portModer 	((volatile unsigned long *) 	(PORT_BASE))
#define portOtyper	((volatile unsigned short *) 	(PORT_BASE+0x4))
#define portOspeedr	((volatile unsigned long *) 	(PORT_BASE+0x8))
#define portPupdr 	((volatile unsigned long *)		(PORT_BASE+0xC))
#define portIdrLow 	((volatile unsigned char *) 	(PORT_BASE+0x10))
#define portIdrHigh ((volatile unsigned char *) 	(PORT_BASE+0x11))
#define portOdrLow 	((volatile unsigned char *) 	(PORT_BASE+0x14))
#define portOdrHigh ((volatile unsigned char *) 	(PORT_BASE+0x15))
#define STK_ADR		0xE000E010
#define STK_CTRL	((volatile unsigned char *)		0xE000E010)
#define STK_LOAD	((volatile unsigned char *)		STK_ADR+0x4)
#define STK_VAL		((volatile unsigned char *)		STK_ADR+0x8)
#define STK_CALIB	((volatile unsigned char *)		STK_ADR+0xC)

static volatile int systick_flag;
static volatile int delay_count;

void systick_irq_handler( void ){
	*STK_CTRL = 0x00;
	delay_count = delay_count - 1;
	if(delay_count > 0){
		delay_1mikro();
	}
	else{
		systick_flag = 1;
	}
}

void init_app( void ){
	/* starta klockor port D och E */
* ( (unsigned long *) 0x40023830) = 0x18;
/* starta klockor för SYSCFG */
* ((unsigned long *)0x40023844) |= 0x4000; 	
/* Relokera vektortabellen */
* ((unsigned long *)0xE000ED08) = 0x2001C000;
	*portModer = 0x55555555;
	/* Initiera undantagsvektor */
	*(( void (**) (void) ) 0x2001C03C ) = systick_irq_handler;
}

void delay_1mikro( void ){
	*STK_CTRL = 0x00;
	*STK_LOAD = ( 168 - 1);
	*STK_VAL = 0;
	*STK_CTRL = 7;
	
}

void delay( unsigned int count ){
	if(count == 0){
		return;
	}
	delay_count = count;
	systick_flag = 0;
	delay_1mikro();
}

void main(void)
{
	init_app();
	*portOdrLow = 0xFF;
	delay( DELAY_COUNT );
	while(1){
		if( systick_flag )
			break;
	}
	*portOdrLow = 0;
}
