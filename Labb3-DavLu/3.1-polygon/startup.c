/*
 * 	startup.c
 *
 */
__attribute__((naked)) __attribute__((section(".start_section"))) void startup(void)
{
    __asm__ volatile(" LDR R0,=0x2001C000\n"); /* set stack */
    __asm__ volatile(" MOV SP,R0\n");
    __asm__ volatile(" BL main\n");   /* call main */
    __asm__ volatile(".L1: B .L1\n"); /* never return */
}

// defines
//#define SIMULATOR
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

// de-referencing and initilaizing
void init_app(void)
{
    /* starta klockor port D och E */
    *((unsigned long*) 0x40023830) = 0x18;
	*(GPIO_E_MODER) = 0x00005555;
	*(GPIO_E_OTYPER) = 0;

	
}

typedef struct {
    char x, y;
} POINT, *PPOINT;

typedef struct {
    POINT p0;
    POINT p1;
} LINE, *PLINE;

typedef struct polygonpoint {
    char x, y;
    struct polygonpoint* next;
} POLYPOINT, *PPOLYPOINT;

__attribute__((naked)) void graphic_initalize(void)
{
    __asm volatile(" .HWORD 0xDFF0\n");
    __asm volatile(" BX LR\n");
}
__attribute__((naked)) void graphic_clear_screen(void)
{
    __asm volatile(" .HWORD 0xDFF1\n");
    __asm volatile(" BX LR\n");
}
__attribute__((naked)) void graphic_pixel_set(int x, int y)
{
    __asm volatile(" .HWORD 0xDFF2\n");
    __asm volatile(" BX LR\n");
}
__attribute__((naked)) void graphic_pixel_clear(int x, int y)
{
    __asm volatile(" .HWORD 0xDFF3\n");
    __asm volatile(" BX LR\n");
}

void delay_250ns(void)
{
    *STK_CTRL = 0;
    *STK_LOAD = ((168 / 4) - 1);
    *STK_VAL = 0;
    *STK_CTRL = 5;
    while((*STK_CTRL & 0x10000) == 0)
	;
    *STK_CTRL = 0;
}

void delay_mikro(unsigned int us)
{
#ifdef SIMULATOR
    us = us / 1000;
    us++;
#endif
    while(us > 0) {
	delay_250ns();
	delay_250ns();
	delay_250ns();
	delay_250ns();
	us--;
    }
}

void delay_milli(unsigned int ms)
{
#ifdef SIMULATOR
    ms = ms / 1000;
    ms++;
#endif
    while(ms > 0) {
	for(int i = 0; i < 1000; i++) {
	    delay_mikro(1);
	}
	ms--;
    }
}

int abs(int x)
{
    if(x == 0) {
	return 0;
    }
    if(x > 0) {
	return x;
    } else
	return -x;
}
void swap(int* x1, int* x2)
{
    int tmp = *x1;
    *x1 = *x2;
    *x2 = tmp;
}

int draw_line(PLINE l)
{
    int x0 = l->p0.x;
    int y0 = l->p0.y;
    int x1 = l->p1.x;
    int y1 = l->p1.y;
    int y, steep, deltax, deltay, error, ystep;
    if(1 > x0 || x0 > 127 || 1 > x1 || x1 > 127 || 1 > y0 || y0 > 64 || 1 > y1 || y1 > 64) {
	return 0;
    }
    if(abs(y1 - y0) > abs(x1 - x0)) {
	steep = 1;
    } else
	steep = 0;
    if(steep) {
	swap(&x0, &y0);
	swap(&x1, &y1);
    }
    if(x0 > x1) {
	swap(&x0, &x1);
	swap(&y0, &y1);
    }
    deltax = x1 - x0;
    deltay = abs(y1 - y0);
    error = 0;
    y = y0;
    if(y0 < y1) {
	ystep = 1;
    } else
	ystep = (-1);
    for(int x = x0; x <= x1; x++) {
	if(steep) {
	    graphic_pixel_set(y, x);
	} else
	    graphic_pixel_set(x, y);
	error = error + deltay;
	if((2 * error) >= deltax) {
	    y = y + ystep;
	    error = error - deltax;
	}
    }
    return 1;
}
int draw_polygon(PPOLYPOINT p)
{
    PLINE line;
    POINT p0, p1;
    p0.x = p->x;
    p0.y = p->y;
    PPOLYPOINT ptr;
    ptr = p->next;
    while(ptr != 0) {
	p1.x = ptr->x;
	p1.y = ptr->y;
	LINE line[] = {p0,p1};
	draw_line(line);
	p0.x = p1.x;
	p0.y = p1.y;
	ptr = ptr->next;
    }
    return 1;
}
POLYPOINT pg8 = { 20, 20, 0 };
POLYPOINT pg7 = { 20, 55, &pg8 };
POLYPOINT pg6 = { 70, 60, &pg7 };
POLYPOINT pg5 = { 80, 35, &pg6 };
POLYPOINT pg4 = { 100, 25, &pg5 };
POLYPOINT pg3 = { 90, 10, &pg4 };
POLYPOINT pg2 = { 40, 10, &pg3 };
POLYPOINT pg1 = { 20, 20, &pg2 };

void main(void)
{
	init_app();
    graphic_initalize();
    graphic_clear_screen();

    while(1) {
	draw_polygon(&pg1);
	delay_milli(50);
	graphic_clear_screen();
	delay_milli(50);
    }
}
