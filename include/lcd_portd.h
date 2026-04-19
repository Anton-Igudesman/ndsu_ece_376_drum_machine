#ifndef LCD_PORTD_H
#define LCD_PORTD_H

#define LCD_SCROLL_LEFT 1
#define LCD_SCROLL_RIGHT 2
#define LCD_DEFAULT 0

typedef unsigned char lcd_mode_t;

void wait_ms(unsigned int x);
void lcd_pause(void);
void lcd_strobe(void);
void lcd_inst(unsigned char c);
void lcd_move(unsigned char row, unsigned char col);
void lcd_write(unsigned char c);
void lcd_init(lcd_mode_t mode);
void lcd_out(long int data, unsigned char d, unsigned char n);
void sci_out(long int data, unsigned char d, unsigned char n);
void sci_crlf(void);

#endif