////////////////////////////////////////////////////////////////////////////
////                             lcd2004.c                             ////
////            Driver for common 4x20 LCD modules (Custom)             ////
////                                                                    ////
////  lcd_init()   Must be called before any other function.            ////
////                                                                    ////
////  lcd_putc(c)  Will display c on the next position of the LCD.      ////
////                     The following have special meaning:            ////
////                      \f  Clear display                             ////
////                      \n  Go to start of second line                ////
////                      \b  Move back one position                    ////      
////                                                                    ////
////  lcd_prints(str)  Display str on the next position of the LCD      ////
////                                                                    ////
////  lcd_print_delay(str, ms) Display each character of str in turn    ////
////                             on the next position                   ////
////                                                                    ////
////  lcd_print_center(str, y)  Display str in the center               ////
////                                                                    ////
////  lcd_gotoxy(x,y) Set write position on LCD (upper left is 1,1)     ////
////                                                                    ////
////  lcd_getc(x,y)   Returns character at position x,y on LCD          ////
////                                                                    ////
////////////////////////////////////////////////////////////////////////////
////        (C) Copyright 1996,1997 Custom Computer Services            ////
//// This source code may only be used by licensed users of the CCS C   ////
//// compiler.  This source code may only be distributed to other       ////
//// licensed users of the CCS C compiler.  No other use, reproduction  ////
//// or distribution is permitted without written permission.           ////
//// Derivative programs created using this software in object code     ////
//// form are not restricted in any way.                                ////
////////////////////////////////////////////////////////////////////////////

#define MAX_X 20
#define MAX_Y 4

#define LCD_DB4   PIN_D4
#define LCD_DB5   PIN_D5
#define LCD_DB6   PIN_D6
#define LCD_DB7   PIN_D7

#define LCD_RS    PIN_E0
#define LCD_RW    PIN_E1
#define LCD_E     PIN_E2

/*
// To prove that the driver can be used with random
// pins, I also tested it with these pins:
#define LCD_DB4   PIN_D4
#define LCD_DB5   PIN_B1
#define LCD_DB6   PIN_C5
#define LCD_DB7   PIN_B5

#define LCD_RS    PIN_E2
#define LCD_RW    PIN_B2
#define LCD_E     PIN_D6
*/

// If you want only a 6-pin interface to your LCD, then
// connect the R/W pin on the LCD to ground, and comment
// out the following line.  Doing so will save one PIC
// pin, but at the cost of losing the ability to read from
// the LCD.  It also makes the write time a little longer
// because a static delay must be used, instead of polling
// the LCD's busy bit.  Normally a 6-pin interface is only
// used if you are running out of PIC pins, and you need
// to use as few as possible for the LCD.
#define USE_RW_PIN   1


// These are the line addresses for most 4x20 LCDs.
#define LCD_LINE_1_ADDRESS 0x00
#define LCD_LINE_2_ADDRESS 0x40
#define LCD_LINE_3_ADDRESS 0x14
#define LCD_LINE_4_ADDRESS 0x54

// These are the line addresses for LCD's which use
// the Hitachi HD66712U controller chip.
/*
#define LCD_LINE_1_ADDRESS 0x00
#define LCD_LINE_2_ADDRESS 0x20
#define LCD_LINE_3_ADDRESS 0x40
#define LCD_LINE_4_ADDRESS 0x60
*/


//========================================

#define lcd_type 2   // 0=5x7, 1=5x10, 2=2 lines(or more)


int8 lcd_line;

int8 const LCD_INIT_STRING[4] =
{
 0x20 | (lcd_type << 2),  // Set mode: 4-bit, 2+ lines, 5x8 dots
 0xc,                     // Display on
 1,                       // Clear display
 6                        // Increment cursor
 };


//-------------------------------------
void lcd_send_nibble(int8 nibble)
{
// Note:  !! converts an integer expression
// to a boolean (1 or 0).
 output_bit(LCD_DB4, !!(nibble & 1));
 output_bit(LCD_DB5, !!(nibble & 2));
 output_bit(LCD_DB6, !!(nibble & 4));
 output_bit(LCD_DB7, !!(nibble & 8));

 delay_cycles(1);
 output_high(LCD_E);
 delay_us(2);
 output_low(LCD_E);
}

//-----------------------------------
// This sub-routine is only called by lcd_read_byte().
// It's not a stand-alone routine.  For example, the
// R/W signal is set high by lcd_read_byte() before
// this routine is called.

#ifdef USE_RW_PIN
int8 lcd_read_nibble(void)
{
int8 retval;
// Create bit variables so that we can easily set
// individual bits in the retval variable.
#bit retval_0 = retval.0
#bit retval_1 = retval.1
#bit retval_2 = retval.2
#bit retval_3 = retval.3

retval = 0;

output_high(LCD_E);
delay_us(1);

retval_0 = input(LCD_DB4);
retval_1 = input(LCD_DB5);
retval_2 = input(LCD_DB6);
retval_3 = input(LCD_DB7);

output_low(LCD_E);
delay_us(1);

return(retval);
}
#endif

//---------------------------------------
// Read a byte from the LCD and return it.

#ifdef USE_RW_PIN
int8 lcd_read_byte(void)
{
int8 low;
int8 high;

output_high(LCD_RW);
delay_cycles(1);

high = lcd_read_nibble();

low = lcd_read_nibble();

return( (high<<4) | low);
}
#endif

//----------------------------------------
// Send a byte to the LCD.
void lcd_send_byte(int8 address, int8 n)
{
output_low(LCD_RS);

#ifdef USE_RW_PIN
while(bit_test(lcd_read_byte(),7)) ;
#else
delay_us(60);
#endif

if(address)
   output_high(LCD_RS);
else
   output_low(LCD_RS);

 delay_cycles(1);

#ifdef USE_RW_PIN
output_low(LCD_RW);
delay_cycles(1);
#endif

output_low(LCD_E);

lcd_send_nibble(n >> 4);
lcd_send_nibble(n & 0xf);
}
//----------------------------

void lcd_init(void)
{
int8 i;

lcd_line = 1;

output_low(LCD_RS);

#ifdef USE_RW_PIN
output_low(LCD_RW);
#endif

output_low(LCD_E);

// Some LCDs require 15 ms minimum delay after
// power-up.  Others require 30 ms.  I'm going
// to set it to 35 ms, so it should work with
// all of them.
delay_ms(35);

for(i=0 ;i < 3; i++)
   {
    lcd_send_nibble(0x03);
    delay_ms(5);
   }

lcd_send_nibble(0x02);

for(i=0; i < sizeof(LCD_INIT_STRING); i++)
   {
    lcd_send_byte(0, LCD_INIT_STRING[i]);

    // If the R/W signal is not used, then
    // the busy bit can't be polled.  One of
    // the init commands takes longer than
    // the hard-coded delay of 50 us, so in
    // that case, lets just do a 5 ms delay
    // after all four of them.
    #ifndef USE_RW_PIN
    delay_ms(5);
    #endif
   }

}

//----------------------------

void lcd_gotoxy(int8 x, int8 y)
{
int8 address;


switch(y)
  {
   case 1:
     address = LCD_LINE_1_ADDRESS;
     break;

   case 2:
     address = LCD_LINE_2_ADDRESS;
     break;

   case 3:
     address = LCD_LINE_3_ADDRESS;
     break;

   case 4:
     address = LCD_LINE_4_ADDRESS;
     break;

   default:
     address = LCD_LINE_1_ADDRESS;
     break;

  }

address += x-1;
lcd_send_byte(0, 0x80 | address);
}

//-----------------------------
void lcd_putc(char c)
{
 switch(c)
   {
    case '\f':
      lcd_send_byte(0,1);
      lcd_line = 1;
      delay_ms(2);
      break;

    case '\n':
       lcd_gotoxy(1, ++lcd_line);
       break;

    case '\b':
       lcd_send_byte(0,0x10);
       break;

    default:
       lcd_send_byte(1,c);
       break;
   }
}

//------------------------------
void lcd_prints(char* str) {
   printf(lcd_putc, str);
}

//------------------------------
void lcd_prints_delay(char* str, int16 ms) {
   for (char* sc = str; *sc != 0; sc++) {
      lcd_putc(*sc);
      delay_ms(ms);
   }
}

//------------------------------
void lcd_print_center(char* str, int8 y) {
   char *sc;
   for (sc = str; *sc != 0; sc++);
   int8 length = sc - str;
   lcd_gotoxy((MAX_X - length) / 2, y);
   printf(lcd_putc, str);
}

//------------------------------
#ifdef USE_RW_PIN
char lcd_getc(int8 x, int8 y)
{
char value;

lcd_gotoxy(x,y);

// Wait until busy flag is low.
while(bit_test(lcd_read_byte(),7));

output_high(LCD_RS);
value = lcd_read_byte();
output_low(LCD_RS);

return(value);
}
#endif


