/*
 * File:   main.c
 * Author: Akechi
 *
 * Created on November 11, 2022, 9:44 PM
 */

#include "main.h"
#include "lcd2004.c"
#include "ds1307.c"

volatile mode_t mode = manual;
mode_t prevMode = manual;
volatile setTime_t setTime;
volatile int1 hasChange = 0;
volatile time_t currentTime;
volatile timer_t nightTimer;
int1 IR1_flag = 0;
int1 IR2_flag = 0;
signed int8 person = 1;
volatile int8 ledin_count = 0;
volatile int8 ledout_count = 0;
volatile int8 menu_count = 0;
volatile int8 enter_count = 0;
volatile int8 up_count = 0;
volatile int8 down_count = 0;
volatile int8 light_count = 0;
volatile int8 h_PIR_count = 0;
volatile int8 l_PIR_count = 0;
volatile int1 blink_state = 1;
volatile int8 blink_count = 0;

#INT_TIMER1
void timer1_isr(void) {  
   // Reset timer 1
   clear_interrupt(INT_TIMER1);
   set_timer1(3036);
   // just for testing
   //output_toggle(PIN_A0);
   // Change mode (MENU is pushed)
   if (!MENU) {
      menu_count++;
      if (menu_count == 16) {
         mode--;
         if (mode > 3) mode = 3;
         if (mode == set_time) {      
            setTime.type = set_night_timer;
            setTime.stage = choose_type;
            setTime.nightTimer.stage = set_start_hour;
            setTime.currentTime.stage = set_hour;
         }
      }
   }
   else {
      if (menu_count > 1 && menu_count < 16) {
         mode++;
         if (mode > 3) mode = 0;
         if (mode == set_time) {
            setTime.type = set_night_timer;
            setTime.stage = choose_type;
            setTime.nightTimer.stage = set_start_hour;
            setTime.currentTime.stage = set_hour;
         }
      }
      menu_count = 0;
   }
   
   // Manual mode (LIGHT is pushed)
   if (mode == manual) {
       if (!LIGHT)  {
           light_count++;
       }
       else {
           if (light_count > 1) {
               LIGHT_CONTROL = !LIGHT_CONTROL;
           }
           light_count = 0;
       }
   }
   
   // Set time mode
   if (mode == set_time) {
       // UP is pushed
       if (!UP) {
           // stop blinking
           blink_state = 1;
           blink_count = 0;
           //
           up_count++;
           if (up_count >= 16 && up_count % 4 == 0) {
               upHandler();        
           }
       }
       else {
           if (up_count > 1 && up_count < 16) {
               upHandler();
           }
           up_count = 0;
       }
       // DOWN is pushed
       if (!DOWN) {
           // stop blinking
           blink_state = 1;
           blink_count = 0;
           //
           down_count++;
           if (down_count >= 16 && down_count % 4 == 0) {
               downHandler();        
           }
       }
       else {
           if (down_count > 1 && down_count < 16) {
               downHandler();
           }
           down_count = 0;
       }
       // ENTER is pushed
       if (!ENTER)  {
           enter_count++;
           if (enter_count == 16) {
               longEnterHandler();
           }
       }
       else {
           if (enter_count > 1 && enter_count < 16) {
               enterHandler();
           }
           enter_count = 0;
       }
   }
   
   // LED in-out
   if (!LED_IN) {
       ledin_count++;
       if (ledin_count > 20) {
           LED_IN = 1;
           ledin_count = 0;
       }
   } 
   if (!LED_OUT) {
       ledout_count++;
       if (ledout_count > 20) {
           LED_OUT = 1;
           ledout_count = 0;
       }
   }
   // Night mode
   if (mode == night) {
      if (!PIR) {
         l_PIR_count++;
         h_PIR_count = 0;
         if (l_PIR_count > 70) {
            LIGHT_CONTROL = 1;
            l_PIR_count--;
         }
      }
      else {
         h_PIR_count++;
         l_PIR_count = 0;
         if (h_PIR_count > 120) {
            LIGHT_CONTROL = 0;
            h_PIR_count--;
         }
      }
   }
}

void main() {
   // Initalization
   setTime.stage = choose_type;
   setTime.prevStage = choose_type;
   setTime.type = set_night_timer;
   setTime.nightTimer.stage = set_start_hour;
   setTime.currentTime.stage = set_hour;
   // LCD
   lcd_init();
   lcd_putc('\f');   // Clear LCD display
   // DS1307
   ds1307_init();
   ds1307_get_time(currentTime.hour, currentTime.minute, currentTime.second);
//!   nightTimer.startHour = 23;
//!   nightTimer.startMinute = 00;
//!   nightTimer.endHour = 7;
//!   nightTimer.endMinute = 00;
//!   nightTimer.state = 1;
//!   writeNightTimerData();
   readNightTimerData();
   // GPIO
   TRISB = 0xFF;
   TRISC &= 0xF8;
   LIGHT_CONTROL = 0;
   LED_IN = 1;
   LED_OUT = 1;
   // Timer 1 interrupt
   setup_timer_1 ( T1_INTERNAL | T1_DIV_BY_4 ); // Internal clock and prescaler 4                           
   set_timer1(3036);                 // Preload value
   clear_interrupt(INT_TIMER1);                 // Clear Timer1 interrupt flag bit
   enable_interrupts(INT_TIMER1);               // Enable Timer1 interrupt
   enable_interrupts(GLOBAL);                   // Enable global interrupts
   // Waiting for setup
   lcd_gotoxy(6, 2);
   printf(lcd_putc, "Loading...");
   for (int8 i = 0; i < 100; i++) {
      lcd_gotoxy(9, 3);
      printf(lcd_putc, "%2d%%", i);
      delay_ms(20);
   }
   delay_ms(200);
   lcd_putc('\f');
   while(TRUE) {
      checkInOut();
      // Clear LCD display when change mode
      if (prevMode != mode) {
         lcd_putc('\f');
         prevMode = mode;
      }
      switch (mode) {
         case manual:
            ds1307_get_time(currentTime.hour, currentTime.minute, currentTime.second);
            displayManualMode();
            break;
         case set_time:
            if (setTime.stage != setTime.prevStage) {
               // Clear LCD display when change set time stage
               lcd_putc('\f');
               setTime.prevStage = setTime.stage;
            }
            if (setTime.stage == change_value && hasChange) {
               if (setTime.type == set_night_timer) {
                  writeNightTimerData();
               }
               else {
                  ds1307_set_time(currentTime.hour, currentTime.minute, currentTime.second);
               }
               hasChange = 0;
            }
            ds1307_get_time(currentTime.hour, currentTime.minute, currentTime.second);
            displaySetTimeMode();
            break;
         case automatic:
            if (person > 0) {
               LIGHT_CONTROL = 1;
            }
            else {
               LIGHT_CONTROL = 0;
            }
            if (nightTimer.state) {
               if (nightTimer.startHour == currentTime.hour && nightTimer.startMinute == currentTime.minute) {
                  mode = night;
               }
            }
            ds1307_get_time(currentTime.hour, currentTime.minute, currentTime.second);
            displayAutomaticMode();
            break;
         case night:
            if (nightTimer.state) {
               if (nightTimer.endHour == currentTime.hour && nightTimer.endMinute == currentTime.minute) {
                  mode = automatic;
               }
            }
            ds1307_get_time(currentTime.hour, currentTime.minute, currentTime.second);
            displayNightMode();
            break;
      }
   }
}

void checkInOut() {
   if (!IR1_flag && !IR2_flag) {
      if (!IR1) {
         IR1_flag = 1;
      }
      else if (!IR2) {
         IR2_flag = 1;
      }
   }
   else {
      if (IR1_flag) {
         if (!IR2 && !IR2_flag) {
            IR2_flag = 1;
            person++;
            LED_IN = 0;
         }
      }
      else if (IR2_flag) {
         if (!IR1 && !IR1_flag) {
            IR1_flag = 1;
            person--;
            if (person < 0) person = 0;
            LED_OUT = 0;
         }
      }
   }
   if (IR1 && IR2 && IR1_flag && IR2_flag) {
      IR1_flag = 0;
      IR2_flag = 0;
   }
}

void displayManualMode() {
   lcd_gotoxy(5, 1);
   printf(lcd_putc, "MANUAL MODE");
   lcd_gotoxy(1, 2);
   if (LIGHT_CONTROL) {
      printf(lcd_putc, "LIGHT:ON ",);
   }
   else {
      printf(lcd_putc, "LIGHT:OFF",);
   }
   lcd_gotoxy(12, 2);
   if (nightTimer.state) {
      printf(lcd_putc, "TIMER:ON ");
   }
   else {
      printf(lcd_putc, "TIMER:OFF");
   }
   lcd_gotoxy(7, 3);
   printf(lcd_putc, "PERSON:%d ", person);
   lcd_gotoxy(15, 4);
   printf(lcd_putc, "%02d:%02d", currentTime.hour, currentTime.minute);
   delay_ms(10);
}

void displayAutomaticMode() {
   lcd_gotoxy(4, 1);
   printf(lcd_putc, "AUTOMATIC MODE");
   lcd_gotoxy(1, 2);
   if (LIGHT_CONTROL) {
      printf(lcd_putc, "LIGHT:ON ",);
   }
   else {
      printf(lcd_putc, "LIGHT:OFF",);
   }
   lcd_gotoxy(12, 2);
   if (nightTimer.state) {
      printf(lcd_putc, "TIMER:ON ");
   }
   else {
      printf(lcd_putc, "TIMER:OFF");
   }
   lcd_gotoxy(7, 3);
   printf(lcd_putc, "PERSON:%d ", person);     
   lcd_gotoxy(15, 4);
   printf(lcd_putc, "%02d:%02d", currentTime.hour, currentTime.minute);
   delay_ms(10);
}

void displayNightMode() {
   lcd_gotoxy(6, 1);
   printf(lcd_putc, "NIGHT MODE");
   lcd_gotoxy(1, 2);
   if (LIGHT_CONTROL) {
      printf(lcd_putc, "LIGHT:ON ",);
   }
   else {
      printf(lcd_putc, "LIGHT:OFF",);
   }
   lcd_gotoxy(12, 2);
   if (nightTimer.state) {
      printf(lcd_putc, "TIMER:ON ");
   }
   else {
      printf(lcd_putc, "TIMER:OFF");
   }
   lcd_gotoxy(7, 3);
   printf(lcd_putc, "PERSON:%d ", person);
   lcd_gotoxy(15, 4);
   printf(lcd_putc, "%02d:%02d", currentTime.hour, currentTime.minute);
   delay_ms(10);
}

void displaySetTimeMode() {
   // Blink handler
   if (blink_state) {
      blink_count++;
      if (blink_count == 50) {         
         blink_state = 0;
         blink_count = 0;
      }
   }
   else {
      blink_count++;
      if (blink_count == 30) {         
         blink_state = 1;
         blink_count = 0;
      }
   }
   
   if (!setTime.stage) {
      if (blink_state) {
         lcd_gotoxy(4, 1);
         printf(lcd_putc, "SET TIME MODE");
         lcd_gotoxy(5, 3);
         printf(lcd_putc, "NIGHT TIMER");
         lcd_gotoxy(5, 4);
         printf(lcd_putc, "CURRENT TIME");
      }
      else {
         if (!setTime.type) {
            lcd_gotoxy(5, 3);
            printf(lcd_putc, "           ");
         }
         else {
            lcd_gotoxy(5, 4);
            printf(lcd_putc, "            ");
         }
      }
   }
   else {
      if (!setTime.type) {
         displaySetNightTimer();
      }
      else {
         displaySetCurrentTime();
      }
   }
   delay_ms(10);
}

void displaySetCurrentTime() {
   if (blink_state) {
      lcd_gotoxy(3, 1);
      printf(lcd_putc, "SET CURRENT TIME");
      lcd_gotoxy(1, 3);
      printf(lcd_putc, "HOUR");
      lcd_gotoxy(7, 3);
      printf(lcd_putc, "MINUTE");
      lcd_gotoxy(15, 3);
      printf(lcd_putc, "SECOND");
      lcd_gotoxy(2, 4);
      printf(lcd_putc, "%02d", currentTime.hour);
      lcd_gotoxy(9, 4);
      printf(lcd_putc, "%02d", currentTime.minute);
      lcd_gotoxy(17, 4);
      printf(lcd_putc, "%02d", currentTime.second);
   }
   else {
      switch (setTime.currentTime.stage) {
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
   }
}

void displaySetNightTimer() {
   if (blink_state) {
      lcd_gotoxy(3, 1);
      printf(lcd_putc, "SET NIGHT TIMER");
      lcd_gotoxy(1, 2);
      printf(lcd_putc, "START");
      lcd_gotoxy(15, 2);
      printf(lcd_putc, "END");
      lcd_gotoxy(1, 3);
      printf(lcd_putc, "%02d:%02d", nightTimer.startHour, nightTimer.startMinute);
      lcd_gotoxy(14, 3);
      printf(lcd_putc, "%02d:%02d", nightTimer.endHour, nightTimer.endMinute);
      lcd_gotoxy(8, 4);
      printf(lcd_putc, "ON/OFF");
   }
   else {
      switch (setTime.nightTimer.stage) {
         case set_start_hour:
            lcd_gotoxy(1, 3);
            printf(lcd_putc, "  :%02d", nightTimer.startMinute);
            break;
         case set_start_minute:
            lcd_gotoxy(1, 3);
            printf(lcd_putc, "%02d:  ", nightTimer.startHour);
            break;    
         case set_end_hour:
            lcd_gotoxy(14, 3);
            printf(lcd_putc, "  :%02d", nightTimer.endMinute);
            break;
         case set_end_minute:
            lcd_gotoxy(14, 3);
            printf(lcd_putc, "%02d:  ", nightTimer.endHour);
            break;  
         case set_state:
            lcd_gotoxy(8, 4);
            if (nightTimer.state) {
               printf(lcd_putc, "  /OFF");
            }
            else {
               printf(lcd_putc, "ON/   ");
            }
            break;
      }  
   }
}

void upHandler() {
    hasChange = 1;
    if (setTime.stage == choose_type) {
        setTime.type = !setTime.type;
    }
    else {
        if (setTime.type == set_night_timer) {
            switch (setTime.nightTimer.stage) {
               case set_start_hour:
                  nightTimer.startHour++;
                  if (nightTimer.startHour > 23) nightTimer.startHour = 0;
                  break;
               case set_start_minute:
                  nightTimer.startMinute++;
                  if (nightTimer.startMinute > 59) nightTimer.startMinute = 0;
                  break;    
               case set_end_hour:
                  nightTimer.endHour++;
                  if (nightTimer.endHour > 23) nightTimer.endHour = 0;
                  break;
               case set_end_minute:
                  nightTimer.endMinute++;
                  if (nightTimer.endMinute > 59) nightTimer.endMinute = 0;
                  break;  
               case set_state:
                  nightTimer.state = !nightTimer.state;
                  break;
             }
        }
        else {
            switch (setTime.currentTime.stage) {
               case set_hour:
                  currentTime.hour++;
                  if (currentTime.hour > 23) currentTime.hour = 0;
                  break;
               case set_minute:
                  currentTime.minute++;
                  if (currentTime.minute > 59) currentTime.minute = 0;
                  break;
               case set_second:
                  currentTime.second++;
                  if (currentTime.second > 59) currentTime.second = 0;
                  break;
             }
        }
    }
}

void downHandler() {
    hasChange = 1;
    if (setTime.stage == choose_type) {
        setTime.type = !setTime.type;
    }
    else {
        if (setTime.type == set_night_timer) {
            switch (setTime.nightTimer.stage) {
               case set_start_hour:
                  nightTimer.startHour--;
                  if (nightTimer.startHour < 0) nightTimer.startHour = 23;
                  break;
               case set_start_minute:
                  nightTimer.startMinute--;
                  if (nightTimer.startMinute < 0) nightTimer.startMinute = 59;
                  break;    
               case set_end_hour:
                  nightTimer.endHour--;
                  if (nightTimer.endHour < 0) nightTimer.endHour = 23;
                  break;
               case set_end_minute:
                  nightTimer.endMinute--;
                  if (nightTimer.endMinute < 0) nightTimer.endMinute = 59;
                  break;  
               case set_state:
                  nightTimer.state = !nightTimer.state;
                  break;
              }
        }
        else {
            switch (setTime.currentTime.stage) {
               case set_hour:
                  currentTime.hour--;
                  if (currentTime.hour < 0) currentTime.hour = 23;
                  break;
               case set_minute:
                  currentTime.minute--;
                  if (currentTime.minute < 0) currentTime.minute = 59;   
                  break;
               case set_second:
                  currentTime.second--;
                  if (currentTime.second < 0) currentTime.second = 59;
                  break;
            }
        }
    }
}

void enterHandler() {
    if (setTime.stage == choose_type) {
        setTime.stage = change_value;
    }
    else {
        if (setTime.type == set_night_timer) {
            setTime.nightTimer.stage++;
            if (setTime.nightTimer.stage > 4) {
                setTime.stage = 0;
                setTime.nightTimer.stage = 0;
                setTime.type = set_current_time;
            }
        }
        else {
            setTime.currentTime.stage++;
            if (setTime.currentTime.stage > 2) {
                setTime.stage = 0;
                setTime.currentTime.stage = 0;
                setTime.type = set_night_timer;
            }
        }
    }
}

void longEnterHandler() {
   if (setTime.stage == change_value) {
        if (setTime.type == set_night_timer) {
            setTime.nightTimer.stage--;
            if (setTime.nightTimer.stage > 4) {
                setTime.nightTimer.stage = 0;
                setTime.stage = choose_type;             
            }
        }
        else {
            setTime.currentTime.stage--;
            if (setTime.currentTime.stage > 2) {
                setTime.currentTime.stage = 0;
                setTime.stage = choose_type;
            }
        }
    }
}

void writeNightTimerData() {
   i2c_start();
   i2c_write(0xD0);          
   i2c_write(0x08);              
   i2c_write(nightTimer.startHour);      
   i2c_write(nightTimer.startMinute);      
   i2c_write(nightTimer.endHour);       
   i2c_write(nightTimer.endMinute);      
   i2c_write(nightTimer.state);     
   i2c_stop();
}

void readNightTimerData() {
   i2c_start();
   i2c_write(0xD0);
   i2c_write(0x08); 
   i2c_start();
   i2c_write(0xD1);
   nightTimer.startHour  = i2c_read();   
   nightTimer.startMinute  = i2c_read();  
   nightTimer.endHour  = i2c_read(); 
   nightTimer.endMinute = i2c_read();
   nightTimer.state = i2c_read();
   i2c_stop();
}

