/* 
 * File:   main.h
 * Author: Akechi
 *
 * Created on November 11, 2022, 9:52 PM
 */

#ifndef MAIN_H
#define   MAIN_H

#include <16f877a.h>
#include <def_877a.h>
#include <stdint.h>

// configurations
#device *=16 ADC=10
#fuses NOWDT, HS, NOPUT, NOPROTECT, NODEBUG, NOBROWNOUT, NOLVP, NOCPD, NOWRT
#use delay(crystal = 20M)
#use i2c(master, fast, sda = PIN_C4, scl = PIN_C3)

//!#define LCD_HIGH_TIME 300
//!#define LCD_LOW_TIME 100
//!#define TIMER1_PREVALUE 3036    //10ms for prescale 4

// Only simulate on Proteus
#define LCD_HIGH_TIME 60
#define LCD_LOW_TIME 20
#define TIMER1_PREVALUE 53036   // 50ms for prescale 4

#define PIR RB1
#define IR1 RB2
#define IR2 RB3
#define LIGHT_PIN PIN_C0
#define LED_IN_PIN PIN_C1
#define LED_OUT_PIN PIN_C2
#define LIGHT_CONTROL RC0
#define LED_IN RC1
#define LED_OUT RC2
#define LIGHT RB0
#define MENU RB4
#define UP RB5
#define DOWN RB6
#define ENTER RB7

typedef enum {manual, set_time, automatic, night} mode_enum;

typedef enum {
   set_night_mode_time, 
   set_current_time
} set_time_type_enum;

typedef enum {
   choose_type_st,
   set_time_st
} set_time_stage_enum;

typedef enum {
   set_hour, 
   set_minute, 
   set_second
} set_current_time_stage_enum;

typedef enum {
   set_start_hour, 
   set_start_minute, 
   set_end_hour, 
   set_end_minute, 
   set_state
} set_nm_time_stage_enum;

typedef struct {
    int8_t hour;
    int8_t minute;
    int8_t second;
} time;

typedef struct {
   int8_t start_hour;
   int8_t start_minute;
   int8_t end_hour;
   int8_t end_minute;
   BOOLEAN state;
} timer;

// Prototype
void check_in_out();
void up_handler();
void down_handler();
void enter_handler();
void display_manual_mode();
void display_automatic_mode();
void display_night_mode();
void display_set_time_mode();
void display_set_current_time();
void display_set_night_mode_time();
void read_night_timer_data();
void write_night_timer_data();

#endif   /* MAIN_H */


