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

__attribute__((naked)) void graphic_initialize(void)
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

/* Defenitioner för de olika portarna */
#define PORT_BASE 	 0x40021000
#define portModer 	((volatile unsigned long *) 	(PORT_BASE))
#define portOtyper	((volatile unsigned short *) 	(PORT_BASE+0x4))
#define portOspeedr	((volatile unsigned long *) 	(PORT_BASE+0x8))
#define portPupdr 	((volatile unsigned long *)		(PORT_BASE+0xC))
#define portIdrLow 	((volatile unsigned char *) 	(PORT_BASE+0x10))
#define portIdrHigh ((volatile unsigned char *) 	(PORT_BASE+0x11))
#define portOdrLow 	((volatile unsigned char *) 	(PORT_BASE+0x14))
#define portOdrHigh ((volatile unsigned char *) 	(PORT_BASE+0x15))
/* Bitar i styrregistret för användning av LC-Display modulen */
#define LCD_ON			0x3F
#define LCD_OFF			0x3E
#define LCD_SET_ADD		0x40
#define LCD_SET_PAGE	0xB8
#define LCD_DISP_START	0xC0
#define LCD_BUSY		0x80
#define B_E 			0x40
#define B_RST			0x20
#define B_CS2			0x10
#define B_CS1			0x8
#define B_SELECT 		0x4
#define B_RW 			0x2
#define B_RS 			0x1



/* Defenitioner för de olika portarna */
#define PORT_BASE 	 0x40021000
#define portModer 	((volatile unsigned long *) 	(PORT_BASE))
#define portOtyper	((volatile unsigned short *) 	(PORT_BASE+0x4))
#define portOspeedr	((volatile unsigned long *) 	(PORT_BASE+0x8))
#define portPupdr 	((volatile unsigned long *)		(PORT_BASE+0xC))
#define portIdrLow 	((volatile unsigned char *) 	(PORT_BASE+0x10))
#define portIdrHigh ((volatile unsigned char *) 	(PORT_BASE+0x11))
#define portOdrLow 	((volatile unsigned char *) 	(PORT_BASE+0x14))
#define portOdrHigh ((volatile unsigned char *) 	(PORT_BASE+0x15))
/* Bitar i styrregistret för användning av LC-Display modulen */
#define B_E 		0x40
#define B_SELECT 	0x4
#define B_RW 		0x2
#define B_RS 		0x1

// addessera ASCII-display och ettställ de bitar som är 1 i x
void ascii_ctrl_bit_set( unsigned char x ){
	unsigned char c;
	c = *portOdrLow;
	c |= ( B_SELECT | x);
	*portOdrLow = c;
}
// addessera ASCII-display och nollställ de bitar som är 1 i x
void ascii_ctrl_bit_clear( unsigned char x ){
	unsigned char c;
	c = *portOdrLow;
	c = B_SELECT | ( c & ~x );
	*portOdrLow = c;
}

// Funktion för att skriva till displayen
void ascii_write_controller( unsigned char byte ){
	//Delay 40 ns
	ascii_ctrl_bit_set(B_E);
	*portOdrHigh = byte;
	delay_250ns();
	ascii_ctrl_bit_clear(B_E);
	//Delay 10 ns
}

// Skriv ett kommando till displayen
void ascii_write_cmd( unsigned char command ){
	ascii_ctrl_bit_clear( B_RS );
	ascii_ctrl_bit_clear( B_RW );
	ascii_write_controller( command );
}

// Skriv data till displayen
void ascii_write_data( data ){
	ascii_ctrl_bit_set( B_RS );
	ascii_ctrl_bit_clear( B_RW );
	ascii_write_controller( data );
}

// Läs från displayen
unsigned char ascii_read_controller( void ){
	unsigned char c;
	ascii_ctrl_bit_set( B_E );
	delay_250ns();
	delay_250ns();
	c = *portIdrHigh;
	ascii_ctrl_bit_clear( B_E );
	return c;
}

// Läs statusbit från displayen (ger 0x80 om man inte kan skriva och 0 om man kan skriva)
unsigned char ascii_read_status( void ){
	*portModer = 0x00005555;
	ascii_ctrl_bit_clear( B_RS );
	ascii_ctrl_bit_set( B_RW );
	unsigned char rv = ascii_read_controller();
	*portModer = 0x55555555;
	return rv;
}

// Läsa data från displayen
unsigned char ascii_read_data( void ){
	*portModer = 0x00005555;
	ascii_ctrl_bit_set( B_RS );
	ascii_ctrl_bit_set( B_RW );
	unsigned char rv = ascii_read_controller();
	*portModer = 0x55555555;
	return rv;
}

// Fördröjningsrutin
void delay_250ns( void ){
	* ( (unsigned long *) 0xE000E010) = 0x0;
	* ( (unsigned long *) 0xE000E014) = ( (168/4) -1 );
	* ( (unsigned long *) 0xE000E018) = 0x0;
	* ( (unsigned char *) 0xE000E010) = 0x5;
	while(* ( (unsigned char *) 0xE000E010) & 0x10000 == 0){}
	* ( (unsigned long *) 0xE000E010) = 0x0;
}

// Fördröj en mikro sekund
void delay_mikro(unsigned int us){
	for(int i = 0; i < us; i++){
		for(int j = 0; j < 4; j++){
			delay_250ns();
		}
	}
}

// Fördröj en milli sekund
void delay_milli(unsigned int ms){
	int i;
	while( ms-- ){
		for( i = 0; i < 1000; i++){
			delay_250ns();
			delay_250ns();
			delay_250ns();
			delay_250ns();
		}
	}
}

// Initiera ascii-display så att man kan skriva och läsa data ifrån den
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
void ascii_gotoxy( int x, int y ){
	int address = x-1;
	if( y == 2 ){
		address = address + 0x40;
	}
	ascii_write_cmd( 0x80 | address );
	delay_mikro( 43 );
}

// Rutin för att skriva data till ASCII-diplayen
void ascii_write_char( unsigned char c ){
	while ( (ascii_read_status() & 0x80) == 0x80 ){}
	delay_mikro( 8 );
	ascii_write_data( c );
	delay_mikro( 43 );
}

// Initera port 15E-8E och 7E-0E som utportar
void init_appASCII(){
	*portModer   = 0x55555555;
	*portOtyper  = 0x0000;
	*portOspeedr = 0x55555555;
}

// Testa ascii displayen
/*
int main(int argc, char **argv){
	char *s;
	char test1[] = "Alfanumerisk ";
	char test2[] = "Display - test";
	
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
	return 0;
}
*/



void delay_500ns( void ){
	* ( (unsigned long *) 0xE000E010) = 0x0;
	* ( (unsigned long *) 0xE000E014) = ( (168/2) -1 );
	* ( (unsigned long *) 0xE000E018) = 0x0;
	* ( (unsigned char *) 0xE000E010) = 0x5;
	while(* ( (unsigned char *) 0xE000E010) & 0x10000 == 0){}
	* ( (unsigned long *) 0xE000E010) = 0x0;
}




typedef unsigned char uint8_t;
/* Välj grafik-display och ettställ de bitar som är 1 i x */
void graphic_ctrl_bit_set( uint8_t x ){
	uint8_t c;
	c = *portOdrLow;
	c &= ~B_SELECT;
	c |= ( ~B_SELECT & x);
	*portOdrLow = c;
}

/* Välj grafisk-diaplay och nollställ de bitar som är 1 i x */
void graphic_ctrl_bit_clear( uint8_t x ){
	uint8_t c;
	c = *portOdrLow;
	c &= ~B_SELECT;
	c &= ~x;
	*portOdrLow = c;
}

/* konfigurera CS-signalerna */
void select_controller( uint8_t controller){
	switch(controller){
		case 0: 
			graphic_ctrl_bit_clear(B_CS1|B_CS2);
			break;
		case B_CS1:
			graphic_ctrl_bit_set(B_CS1);
			graphic_ctrl_bit_clear(B_CS2);
			break;
		case B_CS2:
			graphic_ctrl_bit_clear(B_CS1);
			graphic_ctrl_bit_set(B_CS2);
			break;
		case B_CS1|B_CS2:
			graphic_ctrl_bit_set(B_CS1);
			graphic_ctrl_bit_set(B_CS2);
			break;
	}	
}

void graphic_wait_ready(void){
	uint8_t c;
	graphic_ctrl_bit_clear(B_E);
	*portModer = 0x00005555;
	graphic_ctrl_bit_clear(B_RS); graphic_ctrl_bit_set(B_RW);
	delay_500ns();
	while(1){
		graphic_ctrl_bit_set(B_E);
		delay_500ns();
		c = *portIdrHigh & LCD_BUSY;
		graphic_ctrl_bit_clear(B_E);
		delay_500ns();
		if(c == 0){break;} 
	}
	graphic_ctrl_bit_set(B_E);
	*portModer = 0x55555555;
}

uint8_t graphic_read(uint8_t controller){
	uint8_t c;
	graphic_ctrl_bit_clear(B_E);
	*portModer = 0x00005555;
	graphic_ctrl_bit_set(B_RS|B_RW);
	select_controller(controller);
	delay_500ns();
	graphic_ctrl_bit_set(B_E);
	delay_500ns();
	c = *portIdrHigh;
	graphic_ctrl_bit_clear(B_E);
	*portModer = 0x55555555;
	if(controller & B_CS1){
		select_controller(B_CS1);
		graphic_wait_ready();
	}
	// Får man samma resultat med "==" ist för &?
	if(controller & B_CS2){
		select_controller(B_CS2);
		graphic_wait_ready();
	}
	return c;
}

void graphic_write(uint8_t value, uint8_t controller){
	*portOdrHigh = value;
	select_controller(controller);
	delay_500ns();
	graphic_ctrl_bit_set(B_E);
	delay_500ns();
	graphic_ctrl_bit_clear(B_E);
	
	if(controller && B_CS1){
		select_controller(B_CS1);
		graphic_wait_ready();
	}
	if(controller && B_CS2){
		select_controller(B_CS2);
		graphic_wait_ready();
	}
	*portOdrHigh = 0;
	graphic_ctrl_bit_set(B_E);
	select_controller(0);
	
}

void graphic_write_command(uint8_t command, uint8_t controller){
	graphic_ctrl_bit_clear(B_E);
	select_controller(controller);
	graphic_ctrl_bit_clear(B_RS);
	graphic_ctrl_bit_clear(B_RW);
	graphic_write(command, controller);
}

void graphic_write_data(uint8_t data, uint8_t controller){
	graphic_ctrl_bit_clear(B_E);
	select_controller(controller);
	graphic_ctrl_bit_set(B_RS);
	graphic_ctrl_bit_clear(B_RW);
	graphic_write(data, controller);
}

uint8_t graphic_read_data(uint8_t controller){
	(void) graphic_read(controller);
	return graphic_read(controller);
}

void init_app(){
	*portModer = 0x55555555;
}


void pixel(int x, int y, int set){
	uint8_t mask, c, controller;
	int index;
	
	if( (x < 1) || (x > 128) || (y < 1) || (y > 64)){
		return;
	}
	index = (y-1) / 8;
	
	switch((y-1)%8){
		case 0: mask = 1; break;
		case 1: mask = 2; break; 
		case 2: mask = 4; break; 
		case 3: mask = 8; break; 
		case 4: mask = 0x10; break; 
		case 5: mask = 0x20; break; 
		case 6: mask = 0x40; break; 
		case 7: mask = 0x80; break; 
	}
	if(set == 0){
		mask = ~mask;
	}
	if(x > 64){
		controller = B_CS2;
		x = x - 65;
	}
	else{
		controller = B_CS1;
		x = x - 1;
	}
	graphic_write_command(LCD_SET_ADD | x, controller);
	graphic_write_command(LCD_SET_PAGE | index, controller);
	c = graphic_read_data(controller);
	graphic_write_command(LCD_SET_ADD | x, controller);
	if(set){
		mask = mask | c;
	}
	else{
		mask = mask & c;
	}
	graphic_write_data(mask, controller);
}


//Pong 
#define MAX_POINTS 20

typedef struct tPoint{
	unsigned char x;
	unsigned char y;
}POINT;

typedef struct tGeometry{
	int numpoints;
	int sizex;
	int sizey;
	POINT px[ MAX_POINTS ];
}GEOMETRY, *PGEOMETRY;

// Ball
GEOMETRY ball_geometry = {
	12,		/* numpoints */
	4,4,	/* sizex, sizey */
	{
		/* px[0,1,2 ...] */
		{0,1}, {0,2}, {1,0}, {1,1}, {1,2},
		{1,3}, {2,0}, {2,1}, {2,2}, {2,3},
		{3,1},
		{3,2}
	}
};

// Ball
GEOMETRY player_geometry = {
	8,		/* numpoints */
	1,8,	/* sizex, sizey */
	{
		/* px[0,1,2 ...] */
		{0,1}, {0,2}, {0,3}, {0,4}, {0,5},{0,6},{0,7},{0,8}
	}
};


typedef struct tObj{
	PGEOMETRY geo;
	int dirx, diry;
	int posx, posy;
	void (* draw) (struct tObj *);
	void (* clear) (struct tObj *);
	void (* move) (struct tObj *);
	void (* set_speed) (struct tObj *, int, int);
}OBJECT, *POBJECT;

void set_object_speed(POBJECT o, int speedx, int speedy){
	 o->dirx = speedx;
	 o->diry = speedy;
}

void draw_object( POBJECT o ){
	for(int i = 0; i < sizeof(o->geo->px)/sizeof(o->geo->px[0]); i++){
		pixel(o->geo->px[i].x + o->posx, o->geo->px[i].y + o->posy, 1);
	}
}

void clear_object( POBJECT o ){
	for(int i = 0; i < sizeof(o->geo->px)/sizeof(o->geo->px[0]); i++){
		pixel(o->geo->px[i].x + o->posx, o->geo->px[i].y + o->posy, 0);
	}
}

void move_object( POBJECT o ){
	clear_object(o);
	o->posx = o->posx + o->dirx;
	o->posy = o->posy + o->diry;
	if(o->posx < 1){
		o->dirx = -o->dirx;
	}
	if(o->posx > 128){
		o->dirx = -o->dirx;
	}
	if(o->posy < 1){
		o->diry = -o->diry;
	}
	if(o->posy > 64){
		o->diry = -o->diry;
	}
	draw_object(o);
}

void reverse_direction( POBJECT o ){
	clear_object(o);
	o->diry = -o->diry;
	o->dirx = -o->dirx;
	draw_object(o);
}

static OBJECT ball = 
{
	&ball_geometry,
	0,0,
	1,1,
	draw_object,
	clear_object,
	move_object,
	set_object_speed
};


static OBJECT player1 = 
{
	&player_geometry,
	0,0,
	1,1,
	draw_object,
	clear_object,
	move_object,
	set_object_speed
};

static OBJECT player2 = 
{
	&player_geometry,
	0,0,
	1,1,
	draw_object,
	clear_object,
	move_object,
	set_object_speed
};


void app_init( void ){
	//GPIO_MODER	
	*( (unsigned long *) 0x40020C00) = 0x55005555;
	//GPIO_OTYPER
	*( (unsigned long *) 0x40020C04) &= 0xFFFF0FFF;
	*( (unsigned long *) 0x40020C04) |= 0x00000000;
	//GPIO_PUPDR
	*( (unsigned long *) 0x40020C0C) &= 0xFF00FFFF;
	*( (unsigned long *) 0x40020C0C) |= 0x00AA0000;
}


unsigned char keyb( void ){
	int row, col;
	unsigned char key[] = {1,2,3,0xA,4,5,6,0xB,7,8,9,0xC,0xE,0,0xF,0xD};
	for (row = 1; row <= 4; row++) {
		activateRow(row);
		col = readColumn();
		if ( col != 0 ){
			activateRow( 0 );
			return key[4*(row-1) + (col-1)];
		}
	}
	activateRow( 0 );
	return 0xFF;
}

void out7seg( unsigned char c){
	unsigned char nums[] = {0x3F, 0x06,0x5B,0x4F,0x66,0x5D,0x7D,0x7,0x7F,0x67,
							0x77,0x7C,0x39,0x57,0x79,0x71};
	if(0 <= c < 0x10){
		*((unsigned char *) 0x40020C14) = nums[c];
	}
	else{
		*((unsigned char *) 0x40020C14) = 0x0;
	}
}
	

int readColumn ( void ){
	unsigned char c;
	c = *((volatile unsigned char *) 0x40020C11);
	if(c & 0x8) return 4;
	if(c & 0x4) return 3;
	if(c & 0x2) return 2;
	if(c & 0x1) return 1;
	return 0;
}

void activateRow( unsigned int row ){
	//GPIO_ODR
	volatile unsigned char * GPIO_ODR_HIGH =  ((volatile unsigned char * ) 0x40020C15);
	
	switch( row ){
		case 1: *GPIO_ODR_HIGH = 0x10; break;
		case 2: *GPIO_ODR_HIGH = 0x20; break;
		case 3: *GPIO_ODR_HIGH = 0x40; break;
		case 4: *GPIO_ODR_HIGH = 0x80; break;
		case 0: *GPIO_ODR_HIGH = 0x00; break;
	}
}

static int p1Points = 0;
static int p2Points = 0;

checkForCollision(POBJECT play1,POBJECT play2,POBJECT p){
	if((p->posx <= 8) && ((play1->posy+10) >= p->posy) && ((play1->posy-10) <= p->posy)){
		p1Points += 1;
		reverse_direction( p );
	}
	else if((p->posx >= 115) && ((play2->posy+10) >= p->posy) && ((play2->posy-10) <= p->posy)){
		p2Points += 1;		
		*( (unsigned long *) 0x40020C14) = p2Points;
		reverse_direction( p );
	}
	
}

int main(int argc, char **argv)
{
	char c;
	*( (unsigned long *) 0x40020C14) = 0;
	
	/*char *s;
	char test1[] = "Player1 use 1 and 7 ";
	char test2[] = "Player2 use A and C";
	
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
		 */

	POBJECT p = &ball;
	POBJECT play1 = &player1;
	POBJECT play2 = &player2;
	
	app_init();
	init_app();
	graphic_initialize();
	graphic_clear_screen();
	
	play1->posx = 5;
	play1->posy = 32;
	play2->posx = 122;
	play2->posy = 32;
	
	p->posx = 50;
	p->posy = 50;
	
	play2->draw(play2);
	play1->draw(play1);
	
	p->set_speed( p, 4, 0);
	while(1){
		checkForCollision(play1, play2, p);
		play1->move( play1 );
		play2->move( play2 );
		p->move(p);
		delay_milli(1);
		c = keyb();
		switch( c ){
			case 1: play1->set_speed( play1, 0, -2 ); break;
			case 7: play1->set_speed( play1, 0, 2 ); break;
			case 0xA: play1->set_speed( play2, 0, -2 ); break;
			case 0xC: play1->set_speed( play2, 0, 2 ); break;
		}
	}
}
