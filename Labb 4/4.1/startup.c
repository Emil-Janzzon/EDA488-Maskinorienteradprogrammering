/*
 * 	startup.c
 *
 */
__attribute__((naked)) __attribute__((section (".start_section")) )
void startup ( void )
{
__asm__ volatile(" LDR R0,=0x2001C000\n");		/* set stack */
__asm__ volatile(" MOV SP,R0\n");
__asm__ volatile(" BL main\n");					/* call main */
__asm__ volatile(".L1: B .L1\n");				/* never return */
}


static volatile int systick_flag;
static volatile int delay_count;

#ifdef SIMULATOR
#define DELAY_COUNT 100
#else
#define DELAY_COUNT 1000000
#endif

// definitions
#define PORT_BASE 0x40021000
#define GPIO_E_MODER ((volatile unsigned int*)PORT_BASE)
#define GPIO_E_OTYPER ((volatile unsigned int*)PORT_BASE + 0x4)
#define GPIO_E_SPEEDR ((volatile unsigned int*)PORT_BASE + 0x8)
#define GPIO_E_PUPDR ((volatile unsigned int*)PORT_BASE + 0xC)

#define STK_D 0xE000E010
#define STK_CTRL ((volatile unsigned int*)STK_D)
#define STK_LOAD ((volatile unsigned int*)STK_D + 4)
#define STK_VAL ((volatile unsigned int*)STK_D + 8)
#define STK_CALIB ((volatile unsigned int*)STK_D + 0xC)
#define SIMLUATOR

#define GPIO_D 0x40020C00
#define GPIO_MODER ((volatile unsigned int*)(GPIO_D))
#define GPIO_OTYPER ((volatile unsigned short*)(GPIO_D + 0x4))
#define GPIO_SPEEDR ((volatile unsigned short*)(GPIO_D + 0x8))
#define GPIO_PUPDR ((volatile unsigned int*)(GPIO_D + 0xC))
#define GPIO_IDR_LOW ((volatile unsigned char*)(GPIO_D + 0x10))
#define GPIO_IDR_HIGH ((volatile unsigned char*)(GPIO_D + 0x11))
#define GPIO_ODR_LOW ((volatile unsigned char*)(GPIO_D + 0x14))
#define GPIO_ODR_HIGH ((volatile unsigned char*)(GPIO_D + 0x15))
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

void init_app(void)
{
	/* PORT D0-7*/
	*GPIO_MODER = 0x55555555;
	/* Initiera undantagsvektor */
	*(( void (**) (void) ) 0x2001C03C ) = systick_irq_handler;
}

void delay_1mikro(void)
{
	*STK_CTRL = 0;
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
	unsigned char c=0;
	init_app();
	*GPIO_ODR_LOW = 0;
	delay( DELAY_COUNT );
	*GPIO_ODR_LOW = 0xFF;
	while(1){
		if( systick_flag )
			break;
		*GPIO_ODR_LOW = c;
		c++;
	}
	*GPIO_ODR_LOW = 0;
}