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
/*
 *  startup.c
 */
 
// GPIO
#define GPIO_BASE       0x40020C00 // MD407 port D
#define GPIO_MODER  	((volatile unsigned int *)  (GPIO_BASE))
#define GPIO_OTYPER 	((volatile unsigned short *)    (GPIO_BASE + 0x04))
#define GPIO_OSPEEDR    ((volatile unsigned int *)      (GPIO_BASE + 0x08))
#define GPIO_PUPDR      ((volatile unsigned int *)      (GPIO_BASE + 0x0C))
#define GPIO_IDR_LOW    ((volatile unsigned char *) (GPIO_BASE + 0x10))
#define GPIO_IDR_HIGH   ((volatile unsigned char *) (GPIO_BASE + 0x11))
#define GPIO_ODR_LOW    ((volatile unsigned char *) (GPIO_BASE + 0x14))
#define GPIO_ODR_HIGH   ((volatile unsigned char *) (GPIO_BASE + 0x15))
 
#define E_BASE          0x40021000 // MD407 port E
#define E_MODER         ((volatile unsigned int *)  (E_BASE))
#define E_IDR_LOW       ((volatile unsigned char *) (E_BASE + 0x10))
#define E_ODR_LOW       ((volatile unsigned char *) (E_BASE + 0x14))
 
// System Clock / Delay
#define SysTick     	0xE000E010
#define STK_CTRL        ((volatile unsigned int *)      (SysTick))
#define STK_LOAD        ((volatile unsigned int *)      (SysTick+0x4))
#define STK_VAL     	((volatile unsigned int *)      (SysTick+0x8))
 
// IRQ
#define SCB_VTOR     	    ((volatile unsigned int *) 0xE000ED08)
#define SYSCFG_BASE 		((volatile unsigned int *) 0x40013800)
#define SYSCFG_EXTICR1  	((volatile unsigned int *) 0x40013808)
#define EXTI_IMR        	((volatile unsigned int *) 0x40013C00)
#define EXTI_FTSR       	((volatile unsigned int *) 0x40013C0C)
#define EXTI_RTSR       	((volatile unsigned int *) 0x40013C08)
#define EXTI_PR     		((volatile unsigned int *) 0x40013C14)
#define EXTI3_IRQVEC    	((void (**) (void)) 0x2001C064)
#define EXTI2_IRQVEC    	((void (**) (void)) 0x2001C060)
#define EXTI1_IRQVEC   	 	((void (**) (void)) 0x2001C05C)
#define EXTI0_IRQVEC    	((void (**) (void)) 0x2001C058)
#define NVIC_ISER0      	((volatile unsigned int *) 0xE000E100)
#define NVIC_EXTI3_IRQ_BPOS (1<<9)
#define NVIC_EXTI2_IRQ_BPOS (1<<8)
#define NVIC_EXTI1_IRQ_BPOS (1<<7)
#define NVIC_EXTI0_IRQ_BPOS (1<<6)
#define EXTI3_IRQ_BPOS      (1<<3)
#define EXTI2_IRQ_BPOS      (1<<2)
#define EXTI1_IRQ_BPOS      (1<<1)
#define EXTI0_IRQ_BPOS      (1<<0)
 
static volatile int count = 0;
static volatile int diod = 0;

void irq_handler0(void) {
    // kvittera avbrott frÃ¥n EXTI0
    *EXTI_PR |= EXTI0_IRQ_BPOS;
 
    // kvittera avbrott IRQ0
    *E_ODR_LOW = 0x0010;
    *E_ODR_LOW &= ~0x0010;
    count++;
}
 
void irq_handler1(void) {
    // kvittera avbrott frÃ¥n EXTI1
    *EXTI_PR |= EXTI1_IRQ_BPOS;
 
    // kvittera avbrott IRQ1
    *E_ODR_LOW = 0x0020;
    *E_ODR_LOW &= ~0x0020;
    count = 0;
}
 
void irq_handler2(void) {
    // kvittera avbrott frÃ¥n EXTI2
    *EXTI_PR |= EXTI2_IRQ_BPOS;
 
    // kvittera avbrott IRQ2
    *E_ODR_LOW = 0x0040;
    *E_ODR_LOW &= ~0x0040;
 
    if(diod == 0) {
        // tÃ¤nd diodramp
        count = 0x00;
        diod = 1;
    } else {
        // slÃ¤ck diodramp
        count = 0xFF;
        diod = 0;
    }
}
 
void app_init ( void ) {
 
	*((unsigned long *) 0xE000ED08) = 0x2001C000;
	#ifdef	USBDM
		*((unsigned long *) 0x40023830) = 0x18;
		*((unsigned long *) 0x40023844) |= 0x4000;
		// *((unsigned long *) 0xE000ED08) = 0x2001C000;
	#endif
 
    // port D double hexa display
    *GPIO_MODER = 0x00005555;
    // irq
    *E_MODER = 0x00005500;
 
    //
    *SYSCFG_EXTICR1 &= 0x000;
    *SYSCFG_EXTICR1 |= 0x444;
 
    // Konfigurera EXTI0/1/2 fÃ¶r att generera avbrott
    *EXTI_IMR |= EXTI0_IRQ_BPOS | EXTI1_IRQ_BPOS | EXTI2_IRQ_BPOS;
 
    // Konfigurera fÃ¶r avbrott pÃ¥ positiv flank
    *EXTI_RTSR |= EXTI0_IRQ_BPOS | EXTI1_IRQ_BPOS | EXTI2_IRQ_BPOS;
    *EXTI_FTSR &= ~(EXTI0_IRQ_BPOS | EXTI1_IRQ_BPOS | EXTI2_IRQ_BPOS); 
 
    // SÃ¤tt upp avbrottsvektor
    *EXTI0_IRQVEC = irq_handler0;
    *EXTI1_IRQVEC = irq_handler1;
    *EXTI2_IRQVEC = irq_handler2;

 
    // Konfigurera den bit i NVIC som kontrollerar den avbrottslina som EXTI0/1/2 kopplats till
    *NVIC_ISER0 |= NVIC_EXTI0_IRQ_BPOS | NVIC_EXTI1_IRQ_BPOS | NVIC_EXTI2_IRQ_BPOS;
}
 
void main(void) {
    app_init();
    while(1) {
        *GPIO_ODR_LOW = count;
    }
}