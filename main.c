/*
 * File:   main.c
 * Author: Akechi
 *
 * Created on November 11, 2022, 9:44 PM
 */

#include "main.h"
#include "lcd2004.c"
#include "ds1307.c"

volatile mode_enum mode = manual;
mode_enum previous_mode = manual;
volatile set_time_type_enum set_time_type = set_night_mode_time;
volatile set_time_stage_enum set_time_stage = choose_type_st;
set_time_stage_enum set_time_previous_stage = choose_type_st;
volatile set_nm_time_stage_enum set_nm_time_stage = set_start_hour;
volatile set_current_time_stage_enum set_current_time_stage = set_hour;
set_current_time_stage_enum set_current_time_previous_stage = set_hour;
volatile time current_time;
volatile timer night_timer;
BOOLEAN IR1_flag = 0;
BOOLEAN IR2_flag = 0;
uint8_t person = 1;
volatile uint8_t led_in_count = 0;
volatile uint8_t led_out_count = 0;
volatile uint8_t MENU_count = 0;
volatile uint8_t ENTER_count = 0;
volatile uint8_t UP_count = 0;
volatile uint8_t DOWN_count = 0;
volatile uint8_t LIGHT_count = 0;
volatile BOOLEAN PIR_state = 1;
volatile uint8_t PIR_count = 0;

#INT_TIMER1
void timer1_isr(void) {  
   // Reset timer 1
   clear_interrupt(INT_TIMER1);
   set_timer1(TIMER1_PREVALUE);
   // just for testing
   output_toggle(PIN_A0);
   // Change mode (MENU is pushed)
   if (!MENU) {
      MENU_count++;
      if (MENU_count == 2) {
         mode++;
         if (mode > 3) mode = 0;
         if (mode == set_time) {
            set_time_type = set_night_mode_time;
            set_time_stage = choose_type_st;
            set_nm_time_stage = set_start_hour;
            set_current_time_stage = set_hour;
         };
      }
   }
   else {
       MENU_count = 0;
   }
   
   // Manual mode (LIGHT is pushed)
   if (mode == manual) {
       if (!LIGHT)  {
           LIGHT_count++;
           if (LIGHT_count == 2) {
               output_toggle(LIGHT_PIN);
           }
       }
       
       else {
           LIGHT_count = 0;
       }
   }
   
   // Set time mode
   if (mode == set_time) {
       // UP is pushed
       if (!UP) {
           UP_count++;
           if (UP_count == 2) {
               up_handler();
           }
           else if (UP_count > 14 && UP_count % 3 == 0) {
               up_handler();
               DOWN_count--;
           }
       }
       else {
           UP_count = 0;
       }
       // DOWN is pushed
       if (!DOWN) {
           DOWN_count++;
           if (DOWN_count == 2) {
               down_handler();
           }
           else if (DOWN_count > 14 && DOWN_count % 3 == 0) {
               down_handler();
               DOWN_count--;
           }
       }
       else {
           DOWN_count = 0;
       }
       // ENTER is pushed
       if (!ENTER)  {
           ENTER_count++;
           if (ENTER_count == 2) {
               enter_handler();
           }
       }
       else if (ENTER) {
           ENTER_count = 0;
       }
   }
   
   // LED in-out
   if (!LED_IN) {
       led_in_count++;
       if (led_in_count > 20) {
           output_high(LED_IN_PIN);
           led_in_count = 0;
       }
   } 
   if (!LED_OUT) {
       led_out_count++;
       if (led_out_count > 20) {
           output_high(LED_OUT_PIN);
           led_out_count = 0;
       }
   }
   // Night mode
   if (mode == night) {
      if (PIR == PIR_state) {
         PIR_count++;
         if (PIR_count > 60) {
            if (!PIR) {
               output_high(LIGHT_PIN);
            }
            else {
               output_low(LIGHT_PIN);
            }
            PIR_count--;
         }
      }
      else {
         PIR_count = 0;
      }
      PIR_state = PIR;
   }
}

void main() {
    // LCD
   lcd_init();
   lcd_putc('\f');   // Clear LCD display
   // DS1307
   ds1307_init();
   ds1307_get_time(current_time.hour, current_time.minute, current_time.second);
   night_timer.start_hour = 23;
   night_timer.start_minute = 00;
   night_timer.end_hour = 7;
   night_timer.end_minute = 00;
   night_timer.state = 1;
   write_night_timer_data();
   read_night_timer_data();
   // GPIO
   TRISB = 0xFF;
   PORTB = 0xFF;
   OUTPUT_LOW(LIGHT_PIN);
   OUTPUT_HIGH(LED_IN_PIN);
   OUTPUT_HIGH(LED_OUT_PIN);
   // Timer 1 interrupt
   setup_timer_1 ( T1_INTERNAL | T1_DIV_BY_4 ); // Internal clock and prescaler 8                           
   set_timer1(TIMER1_PREVALUE);                 // Preload value
   clear_interrupt(INT_TIMER1);                 // Clear Timer1 interrupt flag bit
   enable_interrupts(INT_TIMER1);               // Enable Timer1 interrupt
   enable_interrupts(GLOBAL);                   // Enable global interrupts
   // Waiting for setup
   // delay_ms(3000);
   while(TRUE) {
      check_in_out();
      if (mode == set_time) {
         if (set_current_time_previous_stage != set_current_time_previous_stage) {
            ds1307_set_time(current_time.hour, current_time.minute, current_time.second);
            set_current_time_previous_stage = set_current_time_stage;
         }
         if (set_time_stage != set_time_previous_stage) {
            // Clear LCD display when change set time stage
            lcd_putc('\f');
            // Store night timer data
            if (set_time_type == set_current_time) {
               write_night_timer_data();
            }
            set_time_previous_stage = set_time_stage;
         }
      }
      else if (mode == automatic) {
         if (person > 0) {
            output_high(LIGHT_PIN);
         }
         else {
            output_low(LIGHT_PIN);
         }
         if (night_timer.state) {
            if (night_timer.start_hour == current_time.hour && night_timer.start_minute == current_time.minute) {
               mode = night;
            }
         }
      }
      else if (mode == night) {
         if (night_timer.state) {
            if (night_timer.end_hour == current_time.hour && night_timer.end_minute == current_time.minute) {
               mode = automatic;
            }
         }
      }
      ds1307_get_time(current_time.hour, current_time.minute, current_time.second);
      // Clear LCD display when change mode
      if (previous_mode != mode) {
         lcd_putc('\f');
         previous_mode = mode;
      }
      switch (mode) {
         case manual:
            display_manual_mode();
            break;
         case set_time:
            display_set_time_mode();
            break;
         case automatic:
            display_automatic_mode();
            break;
         case night:
            display_night_mode();
            break;
      }
   }
}

void check_in_out() {
    if (!IR1 && !IR1_flag) {
      IR1_flag = 1;
      if (!IR2_flag) {
         person++;
         output_low(LED_IN_PIN);
      }
    }
    if (!IR2 && !IR2_flag) {
      IR2_flag = 1;
      if (!IR1_flag) {
         person--;
         output_low(LED_OUT_PIN);
      }
    }
    if (IR1 && IR2 && IR1_flag && IR2_flag) {
      IR1_flag = 0;
      IR2_flag = 0;
    }
}

void display_manual_mode() {
   lcd_gotoxy(4, 1);
   printf(lcd_putc, "MANUAL MODE");
   lcd_gotoxy(1, 2);
   if (LIGHT_CONTROL) {
      printf(lcd_putc, "LIGHT:ON ",);
   }
   else {
      printf(lcd_putc, "LIGHT:OFF",);
   }
   lcd_gotoxy(12, 2);
   if (night_timer.state) {
      printf(lcd_putc, "TIMER:ON ");
   }
   else {
      printf(lcd_putc, "TIMER:OFF");
   }
   lcd_gotoxy(7, 3);
   printf(lcd_putc, "PERSON:%d", person);
   lcd_gotoxy(15, 4);
   printf(lcd_putc, "%02d:%02d", current_time.hour, current_time.minute);
   //delay_ms(100);
}

void display_automatic_mode() {
   lcd_gotoxy(2, 1);
   printf(lcd_putc, "AUTOMATIC MODE");
   lcd_gotoxy(1, 2);
   if (LIGHT_CONTROL) {
      printf(lcd_putc, "LIGHT:ON ",);
   }
   else {
      printf(lcd_putc, "LIGHT:OFF",);
   }
   lcd_gotoxy(12, 2);
   if (night_timer.state) {
      printf(lcd_putc, "TIMER:ON ");
   }
   else {
      printf(lcd_putc, "TIMER:OFF");
   }
   lcd_gotoxy(7, 3);
   printf(lcd_putc, "PERSON:%d", person);
   lcd_gotoxy(1, 4);
   printf(lcd_putc, "%02d:%02d:%02d", current_time.hour, current_time.minute, current_time.second);
   //delay_ms(100);
}

void display_night_mode() {
   lcd_gotoxy(4, 1);
   printf(lcd_putc, "NIGHT MODE");
   lcd_gotoxy(1, 2);
   if (LIGHT_CONTROL) {
      printf(lcd_putc, "LIGHT:ON ",);
   }
   else {
      printf(lcd_putc, "LIGHT:OFF",);
   }
   lcd_gotoxy(12, 2);
   if (night_timer.state) {
      printf(lcd_putc, "TIMER:ON ");
   }
   else {
      printf(lcd_putc, "TIMER:OFF");
   }
   lcd_gotoxy(7, 3);
   printf(lcd_putc, "PERSON:%d", person);
   lcd_gotoxy(15, 4);
   printf(lcd_putc, "%02d:%02d", current_time.hour, current_time.minute);
   //delay_ms(100);
}

void display_set_time_mode() {
   if (!set_time_stage) {
      lcd_gotoxy(4, 1);
      printf(lcd_putc, "SET TIME MODE");
      lcd_gotoxy(4, 3);
      printf(lcd_putc, "NIGHT MODE TIME");
      lcd_gotoxy(5, 4);
      printf(lcd_putc, "CURRENT TIME");
      delay_ms(LCD_HIGH_TIME);

      if (!set_time_type) {
         lcd_gotoxy(4, 3);
         printf(lcd_putc, "               ");
      }
      else {
         lcd_gotoxy(5, 4);
         printf(lcd_putc, "            ");
      }
      delay_ms(LCD_LOW_TIME);
   }
   else {
      if (!set_time_type) {
         display_set_night_mode_time();
      }
      else {
         display_set_current_time();
      }
   }
}

void display_set_current_time() {
   lcd_gotoxy(3, 1);
   printf(lcd_putc, "SET CURRENT TIME");
   lcd_gotoxy(1, 3);
   printf(lcd_putc, "HOUR");
   lcd_gotoxy(7, 3);
   printf(lcd_putc, "MINUTE");
   lcd_gotoxy(15, 3);
   printf(lcd_putc, "SECOND");
   lcd_gotoxy(2, 4);
   printf(lcd_putc, "%02d", current_time.hour);
   lcd_gotoxy(9, 4);
   printf(lcd_putc, "%02d", current_time.minute);
   lcd_gotoxy(17, 4);
   printf(lcd_putc, "%02d", current_time.second);
   delay_ms(LCD_HIGH_TIME);
   switch (set_current_time_stage) {
      case set_hour:
         lcd_gotoxy(2, 4);
         printf(lcd_putc, "  ");
         break;
      case set_minute:
         lcd_gotoxy(9, 4);
         printf(lcd_putc, "  ");
         break;
      case set_second:
         lcd_gotoxy(17, 4);
         printf(lcd_putc, "  ");
         break;
   }
   delay_ms(LCD_LOW_TIME);
}

void display_set_night_mode_time() {
   lcd_gotoxy(1, 1);
   printf(lcd_putc, "SET NIGHT MODE TIME");
   lcd_gotoxy(1, 2);
   printf(lcd_putc, "START");
   lcd_gotoxy(15, 2);
   printf(lcd_putc, "END");
   lcd_gotoxy(1, 3);
   printf(lcd_putc, "%02d:%02d", night_timer.start_hour, night_timer.start_minute);
   lcd_gotoxy(14, 3);
   printf(lcd_putc, "%02d:%02d", night_timer.end_hour, night_timer.end_minute);
   lcd_gotoxy(8, 4);
   printf(lcd_putc, "ON/OFF");
   delay_ms(LCD_HIGH_TIME);
   
   switch (set_nm_time_stage) {
      case set_start_hour:
         lcd_gotoxy(1, 3);
         printf(lcd_putc, "  :%02d", night_timer.start_minute);
         break;
      case set_start_minute:
         lcd_gotoxy(1, 3);
         printf(lcd_putc, "%02d:  ", night_timer.start_hour);
         break;    
      case set_end_hour:
         lcd_gotoxy(14, 3);
         printf(lcd_putc, "  :%02d", night_timer.end_minute);
         break;
      case set_end_minute:
         lcd_gotoxy(14, 3);
         printf(lcd_putc, "%02d:  ", night_timer.end_hour);
         break;  
      case set_state:
         lcd_gotoxy(8, 4);
         if (night_timer.state) {
            printf(lcd_putc, "  /OFF");
         }
         else {
           printf(lcd_putc, "ON/   ");
         }
         break;
   }
   delay_ms(LCD_LOW_TIME);
}

void up_handler() {
    if (set_time_stage == choose_type_st) {
        set_time_type = ~set_time_type;
    }
    else {
        if (set_time_type == set_night_mode_time) {
            switch (set_nm_time_stage) {
               case set_start_hour:
                  night_timer.start_hour++;
                  if (night_timer.start_hour > 23) night_timer.start_hour = 0;
                  break;
               case set_start_minute:
                  night_timer.start_minute++;
                  if (night_timer.start_minute > 59) night_timer.start_minute = 0;
                  break;    
               case set_end_hour:
                  night_timer.end_hour++;
                  if (night_timer.end_hour > 23) night_timer.end_hour = 0;
                  break;
               case set_end_minute:
                  night_timer.end_minute++;
                  if (night_timer.end_minute > 59) night_timer.end_minute = 0;
                  break;  
               case set_state:
                  night_timer.state = ~night_timer.state;
                  break;
             }
        }
        else {
            switch (set_current_time_stage) {
               case set_hour:
                  current_time.hour++;
                  if (current_time.hour > 23) current_time.hour = 0;
                  break;
               case set_minute:
                  current_time.minute++;
                  if (current_time.minute > 59) current_time.minute = 0;
                  break;
               case set_second:
                  current_time.second++;
                  if (current_time.second > 59) current_time.second = 0;
                  break;
             }
        }
    }
}

void down_handler() {
    if (set_time_stage == choose_type_st) {
        set_time_type = ~set_time_type;
    }
    else {
        if (set_time_type == set_night_mode_time) {
            switch (set_nm_time_stage) {
               case set_start_hour:
                  night_timer.start_hour--;
                  if (night_timer.start_hour < 0) night_timer.start_hour = 23;
                  break;
               case set_start_minute:
                  night_timer.start_minute--;
                  if (night_timer.start_minute < 0) night_timer.start_minute = 59;
                  break;    
               case set_end_hour:
                  night_timer.end_hour--;
                  if (night_timer.end_hour < 0) night_timer.end_hour = 23;
                  break;
               case set_end_minute:
                  night_timer.end_minute--;
                  if (night_timer.end_minute < 0) night_timer.end_minute = 59;
                  break;  
               case set_state:
                  night_timer.state = ~night_timer.state;
                  break;
              }
        }
        else {
            switch (set_current_time_stage) {
               case set_hour:
                  current_time.hour--;
                  if (current_time.hour < 0) current_time.hour = 23;
                  break;
               case set_minute:
                  current_time.minute--;
                  if (current_time.minute < 0) current_time.minute = 59;   
                  break;
               case set_second:
                  current_time.second--;
                  if (current_time.second < 0) current_time.second = 59;

                  break;
            }
        }
    }
}

void enter_handler() {
    if (set_time_stage == choose_type_st) {
        set_time_stage = set_time_st;
    }
    else {
        if (set_time_type == set_night_mode_time) {
            set_nm_time_stage++;
            if (set_nm_time_stage > 4) {
                set_time_stage = 0;
                set_nm_time_stage = 0;
                set_time_type = set_current_time;
            }
        }
        else {
            set_current_time_stage++;
            if (set_current_time_stage > 3) {
                set_time_stage = 0;
                set_current_time_stage = 0;
                set_time_type = set_night_mode_time;
            }
        }
    }
}

void write_night_timer_data() {
   i2c_start();
   i2c_write(0xD0);          
   i2c_write(0x08);              
   i2c_write(night_timer.start_hour);      
   i2c_write(night_timer.start_minute);      
   i2c_write(night_timer.end_hour);       
   i2c_write(night_timer.end_minute);      
   i2c_write(night_timer.state);     
   i2c_stop();
}

void read_night_timer_data() {
   BYTE state;
   i2c_start();
   i2c_write(0xD0);
   i2c_write(0x08); 
   i2c_start();
   i2c_write(0xD1);
   night_timer.start_hour  = i2c_read();   
   night_timer.start_minute  = i2c_read();  
   night_timer.end_hour  = i2c_read(); 
   night_timer.end_minute = i2c_read();
   night_timer.state = i2c_read();
   i2c_stop();
   night_timer.state = state & 1;
}

