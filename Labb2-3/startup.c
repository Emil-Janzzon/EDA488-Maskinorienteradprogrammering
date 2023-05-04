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
void init_app(){
	// starta klockor port D och E 
	*((unsigned long *) 0x40023830) = 0x18;
	*portModer   = 0x55555555;
	*portOtyper  = 0x0000;
	*portOspeedr = 0x55555555;
}

// Testa ascii displayen
int main(int argc, char **argv){
	char *s;
	char test1[] = "Hello";
	char test2[] = "World";
	
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
