/* 
 * File:   main.h
 * Author: Akechi
 *
 * Created on November 11, 2022, 9:52 PM
 */

#ifndef MAIN_H
#define  MAIN_H

#include <16f877a.h>
#include <def_877a.h>

// configurations
#device *=16 ADC=10
#fuses NOWDT, HS, NOPUT, NOPROTECT, NODEBUG, NOBROWNOUT, NOLVP, NOCPD, NOWRT
#use delay(crystal = 20M)
#use i2c(master, fast, sda = PIN_C4, scl = PIN_C3)

#define PIR RB1
#define IR1 RB2
#define IR2 RB3
#define LIGHT_CONTROL RC0
#define LED_IN RC1
#define LED_OUT RC2
#define LIGHT RB0
#define MENU RB4
#define UP RB5
#define DOWN RB6
#define ENTER RB7

typedef enum {manual, set_time, automatic, night} mode_t;

typedef struct {
   enum {choose_type, change_value} stage, prevStage;
   enum {set_night_timer, set_current_time} type;
   struct {
      enum {
         set_start_hour,
         set_start_minute,
         set_end_hour,
         set_end_minute,
         set_state
      } stage;
   } nightTimer;
   struct {
      enum {
         set_hour,
         set_minute,
         set_second
      } stage;
   } currentTime;
} setTime_t;

typedef struct {
    signed int8 hour;
    signed int8 minute;
    signed int8 second;
} time_t;

typedef struct {
   signed int8 startHour;
   signed int8 startMinute;
   signed int8 endHour;
   signed int8 endMinute;
   int1 state;
} timer_t;

// Prototype
void checkInOut();
void upHandler();
void downHandler();
void enterHandler();
void longEnterHandler();
void displayManualMode();
void displayAutomaticMode();
void displayNightMode();
void displaySetTimeMode();
void displaySetCurrentTime();
void displaySetNightTimer();
void readNightTimerData();
void writeNightTimerData();

#endif   /* MAIN_H */


