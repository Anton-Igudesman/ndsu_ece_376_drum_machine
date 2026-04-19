// --------- LCD_PortD.C ---------------------
//
// This routine has LCD driver routines
//
//  LCD_INST:   send an instruction to the LCD
//  LCD_DATA:   send data to the LCD
//  LCD_INIT:   initialize the LCD to 16x4 mode
//
//--- Revision History -----------------
//     5/20/00   JSG
//     9/27/00   Modify LCD_HELLO for Jump messages
//    11/05/00   Clean up LCD routine to use less RAM
//    10/14/14   Modified for new PIC boards

#include <pic18.h>

// Default behavior will be static display mode 
// These will change that display mode 
#define LCD_SCROLL_LEFT		1
#define LCD_SCROLL_RIGHT	2

// Enable static, scroll-left, or scroll-right text modes
typedef unsigned char lcd_mode_t;

void wait_ms(unsigned int x)
{
	unsigned int i, j;

	for (i=0; i<x; i++)
		for (j=0; j<617; j++);
}


void lcd_pause(void)
{
	unsigned char x;
	for (x=0; x<20; x++);
}


void lcd_strobe(void)
{
	RD3 = 0;
	lcd_pause();
	RD3 = 1;
	lcd_pause ();
	RD3 = 0;
	lcd_pause();
}
         
/*  write a byte to the LCD in 4 bit mode 
	bitmask lower bits of PORTD (control bits)
	this is logical OR, s/t lower control bits are preserved
*/
void lcd_inst(unsigned char c)
{
    RD2 = 0;        // send an instruction
	PORTD = (PORTD & 0x0F) |  (c & 0xF0); // send upper nibble
	
	// data is written when E goes high -> low 
	lcd_strobe();
	PORTD = (PORTD & 0x0F) |  ((c<<4) & 0xF0); // send lower nibble shifted as upper nibble
	lcd_strobe();
	wait_ms(10);
}

void lcd_move(unsigned char row, unsigned char col)
{
	/*
	0x80:			1 000 0000
	ex col = 4:		0 000 0100
					1 000 0100
	if 1st bit = 1 => this is a DDRAM instruction
	lower 7 bits in this case => DDRAM address 
	
	ex: set DDRAM address pointer to 0000100
	*/
	if (row == 0) lcd_inst(0x80 + col);
	if (row == 1) lcd_inst(0xC0 + col);
	if (row == 2) lcd_inst(0x94 + col);
	if (row == 3) lcd_inst(0xD4 + col);
}

// Same as lcd_inst subroutine, but RD2 bit is set to 1
// This is for sending data not instruction 

void lcd_write(unsigned char c)
{
	RD2 = 1;        // send data
	PORTD = (PORTD & 0x0F) |  (c & 0xF0);
	lcd_strobe();
	PORTD = (PORTD & 0x0F) |  ((c<<4)  & 0xF0);
	lcd_strobe();
	
}

// Initialization sequence for LCD screen 
// Must run this at start of any program before LCD becomes functional
void lcd_init(lcd_mode_t mode)
{
	TRISD = 0x01;       
	RD1 = 0;  
	lcd_inst(0x33);
	lcd_inst(0x32);
	lcd_inst(0x28);
	lcd_inst(0x0E);
	lcd_inst(0x01);
	
	// Entry mode configuration
	switch (mode)
	{
		case LCD_SCROLL_LEFT: 
			lcd_inst(0x07); // 0000 0111 (I=1, S=1)
			break;
			
		case LCD_SCROLL_RIGHT:
			lcd_inst(0x05); // 0000 0101 (I=0, S=1)
			break;
			
		default: // We can treat a zero entry as default 
			lcd_inst(0x06); // default static (I=1, S=0)
			break;
	}
	
	wait_ms(100);
}

// Initialize for text scroll right

void lcd_out(long int data, unsigned char d, unsigned char n)
{
   unsigned char a[10], i;
   
   if(data < 0) 
   {
      lcd_write('-');
      data = -data;
   }
   else lcd_write(' ');
   
   for (i=0; i<10; i++) 
   {
      a[i] = data % 10;
      data = data / 10;
   }
   
   for (i=d; i>0; i--) 
   {
      if (i == n) lcd_write('.');
      lcd_write(a[i-1] + '0');
   }
}

void sci_out(long int data, unsigned char d, unsigned char n)
{
   unsigned char a[10], i;

   while(!TRMT);   
   if(data < 0) 
   {
      TXREG = '-';
      data = -data;
   }
   else TXREG = ' ';
   
   for (i=0; i<10; i++) 
   {
      a[i] = data % 10;
      data = data / 10;
   }
   
   for (i=d; i>0; i--) 
   {
      if (i == n) { while(!TRMT); TXREG = '.'; }
      while(!TRMT);  TXREG = a[i-1] + 48;
   }
}

void sci_crlf(void)
{
   while(!TRMT);  TXREG = 13;
   while(!TRMT);  TXREG = 10;
}