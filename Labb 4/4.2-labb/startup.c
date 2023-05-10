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

#define PORT_BASE 	 0x40020C00
#define GPIO_MODER  	((volatile unsigned int *)  (PORT_BASE))
#define portModer 	((volatile unsigned long *) 	(PORT_BASE))
#define portOdrLow 	((volatile unsigned char *) 	(PORT_BASE+0x14))
#define portOdrHigh ((volatile unsigned char *) 	(PORT_BASE+0x15))

#define PORTE_BASE	0x40021000
#define E_MODER     ((volatile unsigned int *)  (PORTE_BASE))
#define portEModer 	((volatile unsigned long *) 	(PORTE_BASE))
#define portEIdrLow ((volatile unsigned char *) 	(PORTE_BASE+0x10))
#define E_ODR_LOW       ((volatile unsigned char *) (PORTE_BASE + 0x14))
#define portEOdrHigh ((volatile unsigned char *) 	(PORTE_BASE+0x15))
#define SYSCFG_EXTICR1 0x40013808
#define NVIC_ISER0      	((volatile unsigned int *) 0xE000E100)

static volatile int count = 0;
static volatile int diod = 0;

// Define symbolic constants for register addresses and bits
#define EXTI_PR 0x40013C14 // Pending register
#define EXTI3 0x8 // Bit 3
#define GPIOE_IDR 0x40021010 // Input data register
#define GPIOE_ODR 0x40021014 // Output data register
#define IRQ0 0x1 // Bit 0
#define IRQ1 0x2 // Bit 1
#define IRQ2 0x4 // Bit 2


#define		  VECTOR_DEST			0x2001C000
#define		  SCB_VTOR			((volatile unsigned int *) 0xE000ED08)

#define    	SYSCFG_EXTICR1      ((volatile unsigned int*) 0x40013808)

#define   	EXTI_IMR            ((volatile unsigned int*) 0x40013C00)
#define    	EXTI_RTSR           ((volatile unsigned int*) 0x40013C08)
#define    	EXTI_FTSR           ((volatile unsigned int*) 0x40013C0C)
#define    	EXTI_PR             ((volatile unsigned int*) 0x40013C14)

#define    	EXTI3_IRQVEC		(( void (**) (void)) 0x2001C064)
#define    	EXTI2_IRQVEC		(( void (**) (void)) 0x2001C060)
#define    	EXTI1_IRQVEC		(( void (**) (void)) 0x2001C05C)
#define    	EXTI0_IRQVEC		(( void (**) (void)) 0x2001C058)

#define    	NVIC_ISPR0			((volatile unsigned int *) 0xE000E100)

#define    	NVIC_EXTI3_IRQ_BPOS	(1<<9)
#define    	NVIC_EXTI2_IRQ_BPOS	(1<<8)
#define    	NVIC_EXTI1_IRQ_BPOS (1<<7)
#define    	NVIC_EXTI0_IRQ_BPOS	(1<<6)

#define    	EXTI3_IRQ_BPOS		8
#define    	EXTI2_IRQ_BPOS		4
#define    	EXTI1_IRQ_BPOS		2
#define    	EXTI0_IRQ_BPOS		1

void irq_handler( void ){
    if(*((unsigned int *) EXTI_PR) & EXTI3){ // If interrupt from EXTI3
        switch(*((unsigned int *) GPIOE_IDR) & (IRQ0 | IRQ1 | IRQ2)){ // Check which IRQ bit is set
            case IRQ0: // If interrupt IRQ0
				*E_ODR_LOW = 0x0010;  // Reset IRQ Flip Flop fÃ¶r IRQ0
				*E_ODR_LOW &= ~0x0010;  // Reset IRQ Flip Flop fÃ¶r IRQ0
				count++;
                break;
            case IRQ1: // If interrupt IRQ1
				*E_ODR_LOW = 0x0020; // Reset IRQ Flip Flop fÃ¶r IRQ1s
				*E_ODR_LOW &= ~0x0020; // Reset IRQ Flip Flop fÃ¶r IRQ1
				count=0;
                break;
            case IRQ2: // If interrupt IRQ2
				*E_ODR_LOW = 0x0040; // Reset IRQ Flip Flop fÃ¶r IRQ2
				    if(diod == 0) {
						// tÃ¤nd diodramp
						count = 0x00;
						diod = 1;
					} else {
						// slÃ¤ck diodramp
						count = 0xFF;
						diod = 0;
					}
				*E_ODR_LOW &= ~0x0040; // Reset IRQ Flip Flop fÃ¶r IRQ2
                break;
			}
        }
        *((unsigned int *) EXTI_PR) |= EXTI3; // Acknowledge interrupt from EXTI3
    }

void app_init ( void ) {
	
	/* starta klockor port D och E */
		* ( (unsigned long *) 0x40023830) = 0x18;
	/* starta klockor för SYSCFG */
	* ((unsigned long *)0x40023844) |= 0x4000; 	
	/* Relokera vektortabellen */
	* ((unsigned long *)0xE000ED08) = 0x2001C000;
 
	*((unsigned long *) 0xE000ED08) = 0x2001C000;
 
    // port D double hexa display
    *GPIO_MODER = 0x00005555;
    // irq
    *E_MODER = 0x00005500;
 
    //
    *SYSCFG_EXTICR1 &= 0x0000;
    *SYSCFG_EXTICR1 |= 0x4444;
 
    // Konfigurera EXTI0/1/2/3 fÃ¶r att generera avbrott
    *EXTI_IMR |= EXTI0_IRQ_BPOS | EXTI1_IRQ_BPOS | EXTI2_IRQ_BPOS | EXTI3_IRQ_BPOS;
 
    // Konfigurera fÃ¶r avbrott pÃ¥ positiv flank
    *EXTI_RTSR |= EXTI0_IRQ_BPOS | EXTI1_IRQ_BPOS | EXTI2_IRQ_BPOS | EXTI3_IRQ_BPOS;
    *EXTI_FTSR &= ~(EXTI0_IRQ_BPOS | EXTI1_IRQ_BPOS | EXTI2_IRQ_BPOS | EXTI3_IRQ_BPOS); 
 
    // SÃ¤tt upp avbrottsvektor
    *EXTI0_IRQVEC = irq_handler;
    *EXTI1_IRQVEC = irq_handler;
    *EXTI2_IRQVEC = irq_handler;
	*EXTI3_IRQVEC = irq_handler;
 
    // Konfigurera den bit i NVIC som kontrollerar den avbrottslina som EXTI3 kopplats till.
    *NVIC_ISER0 |= NVIC_EXTI3_IRQ_BPOS;
}

void main(void)
{
	app_init();
	while(1){
		*portOdrLow = count;
	}
}
