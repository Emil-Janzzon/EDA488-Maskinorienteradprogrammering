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
    __asm__ volatile(".L1: B .L1\n");				/* never return*/
}
void app_init(void)
{
    *((unsigned int*)0x40020C00)=0x55555555;
}
#define GPIO_D_ODRLOW ((volatile unsigned int *)(0x40020C14))
#define STK_CTRL ((volatile unsigned int *)(0xE000E010))
#define STK_LOAD ((volatile unsigned int *)(0xE000E014))
#define STK_VAL  ((volatile unsigned int *)(0xE000E018))

void delay_250ns(void){
    *STK_CTRL = 0;
    *STK_LOAD = ((168/4)-1); //countvalue
    *STK_VAL = 0;
    *STK_CTRL = 5;
    while((*STK_CTRL & 0x10000)==0);
    *STK_CTRL = 0;
}

void delay_mikro(unsigned int us){
    #ifdef SIMULATOR
    us = us / 1000;
    us++;
    #endif
    while(us>0)
    {
        delay_250ns();
        delay_250ns();
        delay_250ns();
        delay_250ns();
        us--;
    }

}
void delay_milli(unsigned int ms){
    /*#ifdef SIMULATOR
        ms = ms / 25;
        ms++;
    #endif*/
    
    while(ms>0){
        delay_mikro(1000);
        ms--;
    }
}
void main(void)
{
    app_init();
    while(1)
    {
        *GPIO_D_ODRLOW =0x00;
        *GPIO_D_ODRLOW =0x00;
        delay_milli(500);
        *GPIO_D_ODRLOW =0xFF;
        *GPIO_D_ODRLOW =0xFF;
        delay_milli(500);
    }
}

//För labb - Settings -> Compiler -> Preprocessors = EMPTY




