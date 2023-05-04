__attribute__((naked))
__attribute__((section(".start_section")) )
void startup ( void )
{
__asm volatile(" LDR R0,=0x2001C000\n");
__asm volatile(" MOV SP,R0\n");
__asm volatile(" BL main \n");
__asm volatile(" B .\n");
}
#define GPIO_D 0x40020C00
#define GPIO_MODER ((volatile unsigned int *) (GPIO_D)) 
#define GPIO_OTYPER ((volatile unsigned short *) (GPIO_D+0x4)) 
#define GPIO_PUPDR ((volatile unsigned int *) (GPIO_D+0xC)) 
#define GPIO_IDR_LOW ((volatile unsigned char *) (GPIO_D+0x10)) 
#define GPIO_IDR_HIGH ((volatile unsigned char *) (GPIO_D+0x11)) 
#define GPIO_ODR_LOW ((volatile unsigned char *) (GPIO_D+0x14)) 
#define GPIO_ODR_HIGH ((volatile unsigned char *) (GPIO_D+0x15)) 

void app_init ( void ) /* D8-15 till keypad som input och D0-7 till 7segmentsdisplay som output */
{
	*((unsigned int*)0x40020C00)	= 0x55005555; 		//15-12 Write 	11-8 Read  7-0 Write
	*((unsigned char*)0x40020C05)	= 0x0F;				//OUTPUT-Type*((unsigned int*)*0x40020C08 = 0x55555555 		// port D medium speed
	*((unsigned short*)0x40020C0E)	= 0x00AA; 			//  11-8 Pull-Down

	*((unsigned int*)0x40021000)	= 0x55555555; 		// Port E Write 
}



void kbdActivate( unsigned int row )
{ /* Aktivera angiven rad hos tangentbordet, eller
	* deaktivera samtliga */
	switch( row )
	{
	case 1: *GPIO_ODR_HIGH = 0x10; break;
	case 2: *GPIO_ODR_HIGH = 0x20; break;
	case 3: *GPIO_ODR_HIGH = 0x40; break;
	case 4: *GPIO_ODR_HIGH = 0x80; break;
	case 0: *GPIO_ODR_HIGH = 0x00; break;
	}
}

int kbdGetCol ( void )
{ /* Om någon tangent (i aktiverad rad)
* är nedtryckt, returnera dess kolumnnummer,
* annars, returnera 0 */
	unsigned char c;
	c = *GPIO_IDR_HIGH;
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

char segOut[] ={0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F}; // 7seg Output
void out7seg( unsigned char c){
	
	if (c <= 9){
		*GPIO_ODR_LOW  = segOut[c]; 
	}
	else if(c == 0xFF){
		*GPIO_ODR_LOW  = 0;
	}
	else{
		*GPIO_ODR_LOW  = 0xFF;
	}
}	

int main( void ){
	app_init();
	while(1){
		out7seg(keyb());
	}
	return 0;	
}