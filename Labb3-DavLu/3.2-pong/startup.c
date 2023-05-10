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
//#define SIMLUATOR

#define GPIO_D 0x40020C00
#define GPIO_MODER ((volatile unsigned int*)(GPIO_D))
#define GPIO_OTYPER ((volatile unsigned short*)(GPIO_D + 0x4))
#define GPIO_SPEEDR ((volatile unsigned short*)(GPIO_D + 0x8))
#define GPIO_PUPDR ((volatile unsigned int*)(GPIO_D + 0xC))
#define GPIO_IDR_LOW ((volatile unsigned char*)(GPIO_D + 0x10))
#define GPIO_IDR_HIGH ((volatile unsigned char*)(GPIO_D + 0x11))
#define GPIO_ODR_LOW ((volatile unsigned char*)(GPIO_D + 0x14))
#define GPIO_ODR_HIGH ((volatile unsigned char*)(GPIO_D + 0x15))

#define MAX_POINTS 30

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

// de-referencing and initilaizing
void init_app(void)
{
    /* starta klockor port D och E */
    *((unsigned long*)0x40023830) = 0x18;
	*((volatile unsigned int *)0x40020C08) = 0x55555555;

    *(GPIO_E_OTYPER) = 0x00000000;
    *(GPIO_E_SPEEDR) = 0xFFFF00FF;
    *(GPIO_E_PUPDR) = 0x0000FFFF;

    *(GPIO_MODER) = 0x55005555; // 32 bitar längst till vänster
    *(GPIO_OTYPER) = 0x00000000;
    *(GPIO_SPEEDR) = 0xFFFF00FF;
    *(GPIO_PUPDR) = 0x0000FFFF;
}

typedef struct {
    char x, y;
} POINT, *PPOINT;

typedef struct {
    POINT p0;
    POINT p1;
} LINE, *PLINE;

typedef struct {
    int numpoints;
    int sizex;
    int sizey;
    POINT px[MAX_POINTS];
} GEOMETRY, *PGEOMETRY;

typedef struct tObj {
    PGEOMETRY geo;
    int dirx, diry;
    int posx, posy;
    void (*draw)(struct tObj*);
    void (*clear)(struct tObj*);
    void (*move)(struct tObj*, struct tObj*);
    void (*set_speed)(struct tObj*, int, int);
} OBJECT, *POBJECT;

GEOMETRY ball_geometry = { 12, 4, 4,
    { { 0, 1 }, { 0, 2 }, { 1, 0 }, { 1, 1 }, { 1, 2 }, { 1, 3 }, { 2, 0 }, { 2, 1 }, { 2, 2 }, { 2, 3 }, { 3, 1 },
        { 3, 2 } } };

GEOMETRY paddle_geometry = { 27, 5, 9,
    { { 0, 0 }, { 1, 0 }, { 2, 0 }, { 3, 0 }, { 4, 0 }, { 0, 1 }, { 4, 1 }, { 0, 2 }, { 4, 2 }, { 0, 3 }, { 2, 3 },
        { 4, 3 }, { 0, 4 }, { 2, 4 }, { 4, 4 }, { 0, 5 }, { 2, 5 }, { 4, 5 }, { 0, 6 }, { 4, 6 }, { 0, 7 }, { 4, 7 },
        { 0, 8 }, { 1, 8 }, { 2, 8 }, { 3, 8 }, { 4, 8 } } };

void draw_ballobject(POBJECT o)
{
    for(int i = 0; i < (o->geo->numpoints); i++) {
	graphic_pixel_set(o->geo->px[i].x + o->posx, o->geo->px[i].y + o->posy);
    }
}
void clear_ballobject(POBJECT o)
{
    for(int i = 0; i < (o->geo->numpoints); i++) {
	graphic_pixel_clear(o->geo->px[i].x + o->posx, o->geo->px[i].y + o->posy);
    }
}
void move_ballobject(POBJECT o, POBJECT p)
{
    clear_ballobject(o);
    o->posx = o->posx + o->dirx;
    o->posy = o->posy + o->diry;
    if(o->posx < 1) {
	o->dirx = o->dirx * (-1);
    }
    if(pixel_overlap(o, p)) {
	o->dirx = o->dirx * (-1);
    }
    if(o->posy < 1) {
	o->diry = o->diry * (-1);
    }
    if(o->posy + o->geo->sizey > 64) {
	o->diry = o->diry * (-1);
    }
    draw_ballobject(o);
}
void set_ballobject_speed(POBJECT o, int speedx, int speedy)
{
    o->dirx = speedx;
    o->diry = speedy;
}
void ball_reset(POBJECT b)
{
    clear_ballobject(b);
    b->posx = 1;
    b->posy = 1;
    b->set_speed(b, 4, 1);
}

void paddle_reset(POBJECT p)
{
    clear_ballobject(p);
    p->posx = 123;
    p->posy = 28;
}

int pixel_overlap(POBJECT o1, POBJECT o2)
{
    int offset1x = o1->posx;
    int offset1y = o1->posy;
    int offset2x = o2->posx;
    int offset2y = o2->posy;
    for(int i = 0; i < o1->geo->numpoints; i++) {
	for(int j = 0; j < o2->geo->numpoints; j++)
	    if((offset1x + o1->geo->px[i].x == offset2x + o2->geo->px[j].x) &&
	        (offset1y + o1->geo->px[i].y == offset2y + o2->geo->px[j].y))
		return 1;
    }
    return 0;
}
void kbdActivate(unsigned int row)
{

    switch(row) {
    case 1:
	*GPIO_ODR_HIGH = 0x10;
	break;
    case 2:
	*GPIO_ODR_HIGH = 0x20;
	break;
    case 3:
	*GPIO_ODR_HIGH = 0x40;
	break;
    case 4:
	*GPIO_ODR_HIGH = 0x80;
	break;
    case 0:
	*GPIO_ODR_HIGH = 0x00;
	break;
    }
}
int kbdGetCol(void)
{
    /* Om nÃ¥gon tangent (i aktiverad rad) Ã¤r nedtryckt, returnera dess kolumnummer,
     * annars returnera 0*/
    unsigned char c = *GPIO_IDR_HIGH;
    if(c & 0x8)
	return 4;
    if(c & 0x4)
	return 3;
    if(c & 0x2)
	return 2;
    if(c & 0x1)
	return 1;
    return 0;
}

unsigned char keyb(void)
{
    unsigned char key[] = { 1, 2, 3, 0xA, 4, 5, 6, 0xB, 7, 8, 9, 0xC, 0xE, 0, 0xF, 0xD };
    int row, col;
    for(row = 1; row <= 4; row++) {
	kbdActivate(row);
	if(col = kbdGetCol()) {
	    kbdActivate(0);
	    return key[4 * (row - 1) + (col - 1)];
	}
    }
    kbdActivate(0);
    return 0xFF;
}

static OBJECT ballobject = { &ball_geometry, 0, 0, 1, 1, draw_ballobject, clear_ballobject, move_ballobject,
    set_ballobject_speed };
static OBJECT paddle = { &paddle_geometry, 0, 0, 123, 28, draw_ballobject, clear_ballobject, move_ballobject,
    set_ballobject_speed };

void main(void)
{
    char c;
    POBJECT b = &ballobject;
    POBJECT p = &paddle;
    init_app();
    graphic_initalize();
    graphic_clear_screen();
    b->set_speed(b, 4, 2);
    while(1) {
	b->move(b, p);
	delay_mikro(100);
	p->move(p, b);
	c = keyb();
	switch(c) {
	case 3:
	    p->set_speed(p, 0, -3);
	    break;
	case 9:
	    p->set_speed(p, 0, 3);
	    break;
	case 6:
	    ball_reset(b);
	    paddle_reset(p);
	    break;
	default:
	    p->set_speed(p, 0, 0);
	    break;
	}
	if(b->posx + b->geo->sizex > 128) {
	    break;
	}
    }
}