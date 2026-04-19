#include <pic18.h>
#include "lcd_portd.h"
#include "lcd_helpers.h"

// Write the whole string passed
void lcd_print(const char *msg)
{
   while (*msg != '\0') lcd_write (*msg++);
}

// Write a portion of the string defined by n
void lcd_print_n(const char *msg, unsigned char n)
{
   unsigned char i = 0;

   while ((i < n) && (*msg != '\0'))
   {
      lcd_write(*msg++);
      i++;
   }

   // If i < n and end of message has been reached, add padding
   while (i < n)
   {
      lcd_write(' ');
      i++;
   }

}
