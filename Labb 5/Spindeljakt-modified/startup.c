
#include <stdio.h>
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

//#define SIMLUATOR

#define GPIO_D 0x40020C00
#define GPIO_D_MODER ((volatile unsigned int*)(GPIO_D))
#define GPIO_D_OTYPER ((volatile unsigned short*)(GPIO_D + 0x4))
#define GPIO_D_SPEEDR ((volatile unsigned short*)(GPIO_D + 0x8))
#define GPIO_D_PUPDR ((volatile unsigned int*)(GPIO_D + 0xC))
#define GPIO_D_IDR_HIGH ((volatile unsigned char*)(GPIO_D + 0x11))
#define GPIO_D_ODR_HIGH ((volatile unsigned char*)(GPIO_D + 0x15))

// definitions
#define PORT_BASE 0x40021000
#define GPIO_E_MODER ((volatile unsigned int*)PORT_BASE)
#define GPIO_E_OTYPER ((volatile unsigned int*)PORT_BASE + 0x4)
#define GPIO_E_SPEEDR ((volatile unsigned int*)PORT_BASE + 0x8)
#define GPIO_E_PUPDR ((volatile unsigned int*)PORT_BASE + 0xC)
#define GPIO_E_IDR_HIGH ((volatile unsigned int*)PORT_BASE +0x11)
#define GPIO_E_ODR_LOW ((volatile unsigned int*)PORT_BASE +0x14)
#define GPIO_E_ODR_HIGH ((volatile unsigned int*)PORT_BASE +0x15)

#define GPIO_E 0x40021000
#define GPIO_MODER ((volatile unsigned int *) (GPIO_E)) 
#define GPIO_OTYPER ((volatile unsigned short *) (GPIO_E+0x4)) 
#define GPIO_OSPEEDR ((volatile unsigned long*) (GPIO_E+8))
#define GPIO_PUPDR ((volatile unsigned int *) (GPIO_E+0xC)) 
#define GPIO_IDR_LOW ((volatile unsigned char *) (GPIO_E+0x10)) 
#define GPIO_IDR_HIGH ((volatile unsigned char *) (GPIO_E+0x11)) 
#define GPIO_ODR_LOW ((volatile unsigned char *) (GPIO_E+0x14)) 
#define GPIO_ODR_HIGH ((volatile unsigned char *) (GPIO_E+0x15)) 

unsigned volatile int lives = 3;
unsigned volatile int score = 0;
#define MAX_POINTS 30
/* Masker för Bitar i styrregistret för användning av LC-Display modulen */
#define B_E 		0x40 /* Enable Signal*/
#define B_SELECT 	0x4  /* Välj ASCI-Display*/
#define B_RW 		0x2  /* 0 = Write, 1=Read*/
#define B_RS 		0x1  /* 0 = Control, 1 =Data*/

// addessera ASCII-display och ettställ de bitar som är 1 i x. 
void ascii_ctrl_bit_set( unsigned char x ){
	unsigned char c;
	c = *GPIO_ODR_LOW;
	c |= ( B_SELECT | x);
	*GPIO_ODR_LOW = c;
}
// addessera ASCII-display och nollställ de bitar som är 1 i x
void ascii_ctrl_bit_clear( unsigned char x ){
	unsigned char c;
	c = *GPIO_ODR_LOW;
	c = B_SELECT | ( c & ~x );
	*GPIO_ODR_LOW = c;
}

// Funktion för att skriva till displayen. 
//Skickar en byte till LCD-modulen genom att sätta och rensa enable sign
void ascii_write_controller( unsigned char byte ){
	//Delay 40 ns
	ascii_ctrl_bit_set(B_E);
	*GPIO_ODR_HIGH = byte;
	delay_250ns();
	ascii_ctrl_bit_clear(B_E);
	//Delay 10 ns
}

// Skriv ett kommando till displayen
//Skickar ett kommando till LCD-modulen genom att sätta och rensa enable signalbiten
void ascii_write_cmd( unsigned char command ){
	ascii_ctrl_bit_clear( B_RS );
	ascii_ctrl_bit_clear( B_RW );
	ascii_write_controller( command );
}

// Skriv data till displayen
//Genom att sätta data/kontroll-biten och rensa läs/skriv-biten
void ascii_write_data( data ){
	ascii_ctrl_bit_set( B_RS );
	ascii_ctrl_bit_clear( B_RW );
	ascii_write_controller( data );
}

// Läs från displayen
// Läser en byte från LCD-modulen genom att sätta och rensa enable-signalbiten
unsigned char ascii_read_controller( void ){
	unsigned char c;
	ascii_ctrl_bit_set( B_E );
	delay_250ns();
	delay_250ns();
	c = *GPIO_IDR_HIGH;
	ascii_ctrl_bit_clear( B_E );
	return c;
}


//Läs statusbit från displayen (ger 0x80 om man inte kan skriva och 0 om man kan skriva)
//Läser statusbiten från LCD-modulen genom att rensa data/kontrollbiten
//och sätta läs/skriv-biten
unsigned char ascii_read_status( void ){
	*GPIO_MODER = 0x00005555;
	ascii_ctrl_bit_clear( B_RS );
	ascii_ctrl_bit_set( B_RW );
	char c = ascii_read_controller();
	*GPIO_MODER = 0x55555555;
	return c;
}

// Läsa data från displayen
// Läser data från LCD-module ngenom att sätta data/kontrol-biten och läs/skriv-biten.
unsigned char ascii_read_data( void ){
	*GPIO_MODER= 0x00005555;
	ascii_ctrl_bit_set( B_RS );
	ascii_ctrl_bit_set( B_RW );
	unsigned char rv = ascii_read_controller();
	*GPIO_MODER = 0x55555555;
	return rv;
}

// Initiera ascii-display så att man kan skriva och läsa data ifrån den
// Sätter funktionen displaykontrollen, rensa displayren, entrymodekommandona för LCD-modulen
void ascii_init( void ){
	//Function set
	while( (ascii_read_status() & 0x80) == 0x80){}
	delay_mikro( 8 );
	ascii_write_cmd( 0b111000 );
	delay_mikro( 39 );
	//Display control
	while( (ascii_read_status() & 0x80) == 0x80){}
	delay_mikro( 8 );
	ascii_write_cmd( 0b1110 );
	delay_mikro( 39 );
	//Clear Display
	while( (ascii_read_status() & 0x80) == 0x80){}
	delay_mikro( 8 );
	ascii_write_cmd( 0b1 );
	delay_milli( 2 );
	//Entry mode set
	while( (ascii_read_status() & 0x80) == 0x80){}
	delay_mikro( 8 );
	ascii_write_cmd( 0b110 );
	delay_mikro( 39 );
}

// Gå till angiven rad och kolumn hos ascii-displayen
//Flytar markören till given rad och kollumn på LCD-modulen
void ascii_gotoxy( int x, int y ){
	int address = x-1;
	if( y == 2 ){
		address = address + 0x40;
	}
	ascii_write_cmd( 0x80 | address );
	delay_mikro( 43 );
}

// Rutin för att skriva data till ASCII-diplayen
//Skriver ett tecken till LCD-modulen
void ascii_write_char( unsigned char c ){
	while ( (ascii_read_status() & 0x80) == 0x80 ){}
	delay_mikro( 8 );
	ascii_write_data( c );
	delay_mikro( 43 );
}

// Initera port 15E-8E och 7E-0E som utportar
void init_appASCII(){
	*((unsigned long *) 0x40023830) = 0x18;
	*GPIO_MODER  = 0x55555555;
	*GPIO_OTYPER   = 0x0000;
	*GPIO_OSPEEDR  = 0x55555555;
}

// A function to convert an integer to a string
void int_to_str(int n, char *buffer) {
    int i = 0; // index for the buffer
    int sign = n; // store the sign of n
    // If n is zero, store '0' in buffer and return
    if (n == 0) {
        buffer[i++] = '0';
        buffer[i] = '\0';
        return;
    }
    // If n is negative, make it positive and remember the sign
    if (n < 0) {
        n = -n;
    }
    // Extract digits from right to left and store them in buffer
    while (n > 0) {
        int digit = n % 10; // get the last digit
        buffer[i++] = digit + '0'; // convert it to a character and store it
        n = n / 10; // remove the last digit
    }
    // If sign is negative, append '-'
    if (sign < 0) {
        buffer[i++] = '-';
    }
    // Append null character to mark the end of string
    buffer[i] = '\0';
    // Reverse the buffer to get the correct order of digits
    reverse(buffer, i);
}

// A function to reverse a string
void reverse(char *buffer, int length) {
    int start = 0; // index for the start of string
    int end = length - 1; // index for the end of string
    while (start < end) {
        // Swap characters at start and end
        char temp = buffer[start];
        buffer[start] = buffer[end];
        buffer[end] = temp;
        // Move start and end towards the middle
        start++;
        end--;
    }
}

/* void int_to_str(int n, char *buffer) {
	snprintf(buffer, "%d", n); // write the integer to the buffer as a string
}*/
#define STK_CTRL ((volatile unsigned int *)(0xE000E010))
#define STK_LOAD ((volatile unsigned int *)(0xE000E014))
#define STK_VAL  ((volatile unsigned int *)(0xE000E018))

//Skapar en fördröjning om 250nanosekunder
void delay_250ns(void){
    *STK_CTRL = 0;
    *STK_LOAD = ((168/4)-1); //countvalue
    *STK_VAL = 0;
    *STK_CTRL = 5;
    while((*STK_CTRL & 0x10000)==0);
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

    *(GPIO_D_MODER) = 0x55005555; // 32 bitar längst till vänster
    *(GPIO_D_OTYPER) = 0x00000000;
    *(GPIO_D_SPEEDR) = 0xFFFF00FF;
    *(GPIO_D_PUPDR) = 0x0000FFFF;
}
// structen POINT får två variabler , x & y.
typedef struct {
    char x, y;
} POINT, *PPOINT;
// structen LINE definerar en linje mellan två punkter, p0 & p1
typedef struct {
    POINT p0;
    POINT p1;
} LINE, *PLINE;
// structen GEOMETRY definerar en form genom en array av poäng och dess storlek
typedef struct {
    int numpoints;
    int sizex;
    int sizey;
    POINT px[MAX_POINTS];
} GEOMETRY, *PGEOMETRY;
// structen OBJECT definerar ett spel objekt med avseende på dess geometri, position, riktning, and fyra funktion.
typedef struct tObj {
    PGEOMETRY geo;
    int dirx, diry;
    int posx, posy;
    void (*draw)(struct tObj*);
    void (*clear)(struct tObj*);
    void (*move)(struct tObj*);
    void (*set_speed)(struct tObj*, int, int);
} OBJECT, *POBJECT;
//definerar en boll med 12punkter
GEOMETRY ball_geometry = { 12, 4, 4,
    { { 0, 1 }, { 0, 2 }, { 1, 0 }, { 1, 1 }, { 1, 2 }, { 1, 3 }, { 2, 0 }, { 2, 1 }, { 2, 2 }, { 2, 3 }, { 3, 1 },
        { 3, 2 } } };
//definerar en spindel med formen av en octagon med 22 punkter.
GEOMETRY spider_geometry = { 22, 6, 8,
    { { 2, 0 }, { 3, 0 }, { 1, 1 }, { 4, 1 }, { 0, 2 }, { 1, 2 }, { 2, 2 }, { 3, 2 }, { 4, 2 }, { 5, 2 }, { 0, 3 },
        { 2, 3 }, { 3, 3 }, { 5, 3 }, { 1, 4 }, { 4, 4 }, { 2, 5 }, { 3, 5 }, { 1, 6 }, { 4, 6 }, { 0, 7 },
        { 5, 7 } } };
//ritar ett spelobjekt på grafikskärmen genom att ställa in pixlarna enligt dess geometri och position
void draw_ballobject(POBJECT o)
{
    for(int i = 0; i < (o->geo->numpoints); i++) {
	graphic_pixel_set(o->geo->px[i].x + o->posx, o->geo->px[i].y + o->posy);
    }
}
//rensar ett spelobjekt från grafikskärmen genom att rensa pixlarna enligt dess geometri och position.
void clear_ballobject(POBJECT o)
{
    for(int i = 0; i < (o->geo->numpoints); i++) {
	graphic_pixel_clear(o->geo->px[i].x + o->posx, o->geo->px[i].y + o->posy);
    }
}

//flyttar ett spelobjekt genom att ändra dess position enligt dess riktning och hastighet. 
//Den kontrollerar också kollisioner med kanterna av skärmen och studsar tillbaka om det behövs.
void move_ballobject(POBJECT o)
{
    clear_ballobject(o);
    o->posx = o->posx + o->dirx;
    o->posy = o->posy + o->diry;
    if(o->posx < 1) {
	o->dirx = o->dirx * (-1);
    }
    if(o->posx + o->geo->sizex > 128) {
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
// ställer in hastigheten och riktningen för ett spelobjekt genom att ändra dess dirx och diry-fält.
void set_ballobject_speed(POBJECT o, int speedx, int speedy)
{
    o->dirx = speedx;
    o->diry = speedy;
}
// kontrollerar om två spelobjekt är i kontakt genom att jämföra deras positioner och storlekar. Den returnerar 1 om de är i kontakt, och 0 annars.
int objects_contact(POBJECT o1, POBJECT o2)
{
	int offset1x = o1->posx;
    int offset1y = o1->posy;
	
	int offset3x = o1->posx + o1->geo->sizex;
	int offset3y = o1->posy + o1->geo->sizey;
	
    int offset2x = o2->posx;
    int offset2y = o2->posy;
	
	int offset4x = o2->posx + o2->geo->sizex;
	int offset4y = o2->posy + o2->geo->sizey;
	
	if((offset3x<offset2x || offset1x>offset4x || offset3y<offset2y || offset1y>offset4y)) {
		return 0;
	}
	else {
		return objects_overlap(o1,o2);
	}	
}

int objects_overlap(POBJECT o1, POBJECT o2)
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

void kbdActivate( unsigned int row )
{ /* Aktivera angiven rad hos tangentbordet, eller
	* deaktivera samtliga */
	switch( row )
	{
	case 1: *GPIO_D_ODR_HIGH = 0x10; break;
	case 2: *GPIO_D_ODR_HIGH = 0x20; break;
	case 3: *GPIO_D_ODR_HIGH = 0x40; break;
	case 4: *GPIO_D_ODR_HIGH = 0x80; break;
	case 0: *GPIO_D_ODR_HIGH = 0x00; break;
	}
}

int kbdGetCol ( void )
{ /* Om någon tangent (i aktiverad rad)
* är nedtryckt, returnera dess kolumnnummer,
* annars, returnera 0 */
	unsigned char c;
	c = *GPIO_D_IDR_HIGH;
	if ( c & 0x8 ) return 4;
	if ( c & 0x4 ) return 3;
	if ( c & 0x2 ) return 2;
	if ( c & 0x1 ) return 1;
	return 0;
}

unsigned char keyb(void)
{
	unsigned char key[]={1,2,3,0xA,4,5,6,0xB,7,8,9,0xC,0xE,0,0xF,0xD};
	int row, col;
	for (row=1; row <=4 ; row++ ) {
		kbdActivate( row );
		if( (col = kbdGetCol () ) )
		{
			kbdActivate( 0 );
			return key [4*(row-1)+(col-1) ];
		}
	}
	kbdActivate( 0 );
	return 0xFF;
}
    
static OBJECT   ballobject =
{
    &ball_geometry,   /* geometry för en boll */
    5,2,
    10,10,
    draw_ballobject,
    clear_ballobject,
    move_ballobject,
    set_ballobject_speed,
};

static OBJECT   spider =
{
    &spider_geometry,   /* geometry för en spindel: initial position, hastighet och funktioner.*/
    0,0,
    61,28,
    draw_ballobject,
    clear_ballobject,
    move_ballobject,
    set_ballobject_speed,
};

void victim_reset(POBJECT victim)
{
	
	clear_ballobject(victim);
    victim->posx = 61;
    victim->posy = 28;
}

void creature_reset(POBJECT creature)
{
	
	clear_ballobject(creature);
    creature->posx = 1;
    creature->posy = 1;
    creature->set_speed(creature, 5, 2);
}

void draw_lives(int lives) {
    char symbol = '*'; // choose a symbol for life
    int x = 1; // starting x position for drawing
    int y = 1; // starting y position for drawing
    ascii_gotoxy(x, y); // go to the position
    for (int i = 0; i < lives; i++) {
        ascii_write_char(symbol); // write each symbol
        x++; // move x position by 1 for next symbol
    }
}

void draw_score(int score) {
    char buffer[10];
    int_to_str(score, buffer); // convert score to string
    int x = 1; // starting x position for drawing
    int y = 2; // starting y position for drawing
    ascii_gotoxy(x, y); // go to the position
    for (int i = 0; buffer[i] != '\0'; i++) {
        ascii_write_char(buffer[i]); // write each character
        x++; // move x position by 1 for next character
    }
}


void main(void) /*Huvudfunktionen main skapar två pekare till objekten victim (bollen) och creature (spindeln).*/
{
	
	char c;
	char *s;
	char test1[] = "Styr med 2,4,5,6,8";
	char test2[] = "Resetta spelet med D";
	
	init_app();
	ascii_init();
	ascii_gotoxy(1,1);
	s = test1;
	while( *s )
		ascii_write_char( *s++ );
	ascii_gotoxy(1,2);
	s = test2;
	while( *s )
		ascii_write_char( *s++ );
    POBJECT victim = &ballobject;
    POBJECT creature = &spider;
    init_app();
    graphic_initalize();
    graphic_clear_screen();
    victim->set_speed(victim, 5, 2);
    while(1) { /*Flyttar bollen och spindeln med hjälp av move-funktionen.
	 * Läser in ett tecken från tangentbordet med hjälp av keyb-funktionen.
	 * Ändrar spindelns hastighet beroende på vilken tangent som trycks med hjälp av set_speed-funktionen.
	 * Bryter loopen om spindeln kolliderar med kanten av skärmen eller med bollen, vilket innebär att spelet är slut.
	 * */
		victim->move(victim);
		creature->move(creature);
		draw_score(score); //Draw score
		draw_lives(lives); //Draw lives
		ascii_write_cmd(0x01); // clear the display
		score ++;
		c = keyb();
		switch(c) {
			case 6:
				creature->set_speed(creature, 2, 0);
				break;
			case 4:
				creature->set_speed(creature, -2, 0);
				break;
			case 5:
				creature->set_speed(creature, 0, 0);
				break;
			case 2:
				creature->set_speed(creature, 0, -2);
				break;
			case 8:
				creature->set_speed(creature, 0, 2);
				break;
			case 3:
				creature->set_speed(creature, 0, 2);
				break;
			case 0xD:
				creature_reset(victim);
				victim_reset(creature);
			default:
				creature->set_speed(creature, 0, 0);
				break;
			}
		if(creature->posx < 1 || creature->posx + creature->geo->sizex > 128 || creature->posy < 1 ||
	    creature->posy + creature->geo->sizey > 64) {
	    lives --;
		if (lives == 0) { //Game Over
			break;
		}
		else{
			creature_reset(victim); //Spelet fortsätter
			victim_reset(creature);
	
		}
		}
		if(objects_contact(victim, creature)) {
			    lives --;
		if (lives == 0) { //Game Over
			break;
		}
		else{
			creature_reset(victim); //Spelet fortsätter
			victim_reset(creature);
	
		}
		
		
		
	}
		delay_milli(1);
	}
}
