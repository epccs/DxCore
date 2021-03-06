/*
  wiring.c - Partial implementation of the Wiring API for the ATmega8.
  Part of Arduino - http://www.arduino.cc/

  Copyright (c) 2005-2006 David A. Mellis

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General
  Public License along with this library; if not, write to the
  Free Software Foundation, Inc., 59 Temple Place, Suite 330,
  Boston, MA  02111-1307  USA
*/

#include "wiring_private.h"

// the prescaler is set so that timer ticks every 64 clock cycles, and the
// the overflow handler is called every 256 ticks.

#ifndef DISABLE_MILLIS

#ifdef MILLIS_USE_TIMERRTC_XTAL
#define MILLIS_USE_TIMERRTC
#endif

//volatile uint16_t microseconds_per_timer_overflow;
//volatile uint16_t microseconds_per_timer_tick;

#if (defined(MILLIS_USE_TIMERB0)  || defined(MILLIS_USE_TIMERB1) || defined(MILLIS_USE_TIMERB2)|| defined(MILLIS_USE_TIMERB3)|| defined(MILLIS_USE_TIMERB4)) //Now TCB as millis source does not need fraction
  volatile uint32_t timer_millis = 0; //That's all we need to track here

#elif !defined(MILLIS_USE_TIMERRTC) //all of this stuff is not used when the RTC is used as the timekeeping timer
  static uint16_t timer_fract = 0;
  uint16_t fract_inc;
  volatile uint32_t timer_millis = 0;
  #define FRACT_MAX (1000)
  #define FRACT_INC (clockCyclesToMicroseconds(TIME_TRACKING_CYCLES_PER_OVF)%1000);
  #define MILLIS_INC (clockCyclesToMicroseconds(TIME_TRACKING_CYCLES_PER_OVF)/1000);
  volatile uint32_t timer_overflow_count = 0;
#else
  volatile uint16_t timer_overflow_count = 0;
#endif

//overflow count is tracked for all timer options, even the RTC


#if !defined(MILLIS_USE_TIMERRTC)

  inline uint16_t clockCyclesPerMicrosecond(){
  #ifdef MILLIS_USE_TIMERD0
    #if (F_CPU==20000000UL || F_CPU==10000000UL ||F_CPU==5000000UL)
      return ( 20 ); //this always runs off the 20MHz oscillator
    #else
      return ( 16 );
    #endif
  #else
    return ( (F_CPU) / 1000000L );
  #endif
  }


  inline unsigned long clockCyclesToMicroseconds(unsigned long cycles){
    return ( cycles / clockCyclesPerMicrosecond() );
  }

  inline unsigned long microsecondsToClockCycles(unsigned long microseconds){
    return ( microseconds * clockCyclesPerMicrosecond() );
  }



  #if defined(MILLIS_USE_TIMERD0)
    #ifndef TCD0
      #error "Selected millis timer, TCD0, only exists on 1-series parts"
    #endif
  #elif !defined(MILLIS_USE_TIMERA0) && !defined(MILLIS_USE_TIMERA1)
    static volatile TCB_t* _timer =
    #if defined(MILLIS_USE_TIMERB0)
      &TCB0;
    #elif defined(MILLIS_USE_TIMERB1)
      #ifndef TCB1
        #error "Selected millis timer, TCB1 does not exist on this part."
      #endif
      &TCB1;
    #elif defined(MILLIS_USE_TIMERB2)
      #ifndef TCB2
        #error "Selected millis timer, TCB2 does not exist on this part."
      #endif
      &TCB2;
    #elif defined(MILLIS_USE_TIMERB3)
      #ifndef TCB3
        #error "Selected millis timer, TCB3 does not exist on this part."
      #endif
      &TCB3;
    #elif defined(MILLIS_USE_TIMERB4)
      #ifndef TCB4
        #error "Selected millis timer, TCB4 does not exist on this part."
      #endif
      &TCB4;
    #else  //it's not TCB0, TCB1, TCD0, TCA0, TCA1, or RTC
      #error "No millis timer selected, but not disabled - can't happen!".
    #endif
  #else
    #if !defined(TCA1) && defined(MILLIS_USE_TIMERA1)
      #error "Selected millis timer, TCA1 does not exist on this part."
    #endif
  #endif
#endif //end #if !defined(MILLIS_USE_TIMERRTC)


#if defined(MILLIS_USE_TIMERA0)
  ISR(TCA0_HUNF_vect)
#elif defined(MILLIS_USE_TIMERA1)
  ISR(TCA1_HUNF_vect)
#elif defined(MILLIS_USE_TIMERD0)
  ISR(TCD0_OVF_vect)
#elif defined(MILLIS_USE_TIMERRTC)
  ISR(RTC_CNT_vect)
#elif defined(MILLIS_USE_TIMERB0)
  ISR(TCB0_INT_vect)
#elif defined(MILLIS_USE_TIMERB1)
  ISR(TCB1_INT_vect)
#elif defined(MILLIS_USE_TIMERB2)
  ISR(TCB2_INT_vect)
#elif defined(MILLIS_USE_TIMERB3)
  ISR(TCB3_INT_vect)
#elif defined(MILLIS_USE_TIMERB4)
  ISR(TCB4_INT_vect)
#else
  #error "no millis timer selected"
#endif
{
  // copy these to local variables so they can be stored in registers
  // (volatile variables must be read from memory on every access)

  #if (defined(MILLIS_USE_TIMERB0)||defined(MILLIS_USE_TIMERB1)|| defined(MILLIS_USE_TIMERB2)|| defined(MILLIS_USE_TIMERB3)|| defined(MILLIS_USE_TIMERB4) )
    #if(F_CPU>1000000)
      timer_millis++; //that's all we need to do!
    #else //if it's 1<Hz, we set the millis timer to only overflow every 2 milliseconds, intentionally sacrificing resolution.
      timer_millis+=2;
    #endif
  #else
    #if !defined(MILLIS_USE_TIMERRTC) //TCA0 or TCD0
      uint32_t m = timer_millis;
      uint16_t f = timer_fract;
      m += MILLIS_INC;
      f += FRACT_INC;
      if (f >= FRACT_MAX) {

        f -= FRACT_MAX;
        m += 1;
      }
      timer_fract = f;
      timer_millis = m;
    #endif
    //if RTC is used as timer, we only increment the overflow count
    timer_overflow_count++;
  #endif
  /* Clear flag */
  #if defined(MILLIS_USE_TIMERA0)
    TCA0.SPLIT.INTFLAGS = TCA_SPLIT_HUNF_bm;
  #elif defined(MILLIS_USE_TIMERA1)
    TCA1.SPLIT.INTFLAGS = TCA_SPLIT_HUNF_bm;
  #elif defined(MILLIS_USE_TIMERD0)
    TCD0.INTFLAGS=TCD_OVF_bm;
  #elif defined(MILLIS_USE_TIMERRTC)
    RTC.INTFLAGS=RTC_OVF_bm;
  #else //timerb
    _timer->INTFLAGS = TCB_CAPT_bm;
  #endif
}

unsigned long millis()
{
  //return timer_overflow_count; //for debugging timekeeping issues where these variables are out of scope from the sketch
  unsigned long m;

  // disable interrupts while we read timer0_millis or we might get an
  // inconsistent value (e.g. in the middle of a write to timer0_millis)
  uint8_t status = SREG;
  cli();
  #if defined(MILLIS_USE_TIMERRTC)
    m=timer_overflow_count;
    if (RTC.INTFLAGS & RTC_OVF_bm) { //there has just been an overflow that hasn't been accounted for by the interrupt
      m++;
    }
    SREG = status;
    m=(m<<16);
    m+=RTC.CNT;
    //now correct for there being 1000ms to the second instead of 1024
    m=m-(m>>5)-(m>>6);
  #else
    m = timer_millis;
    SREG = status;
  #endif
  return m;
}

#ifndef MILLIS_USE_TIMERRTC

  unsigned long micros() {

    unsigned long overflows, microseconds;

    #if !defined(MILLIS_USE_TIMERA0) && !defined(MILLIS_USE_TIMERA1)
      uint16_t ticks;
    #else
      uint8_t ticks;
    #endif

    /* Save current state and disable interrupts */
    uint8_t status = SREG;
    cli();



    /* Get current number of overflows and timer count */
    #if defined(MILLIS_USE_TIMERA0) || defined(MILLIS_USE_TIMERA1) || defined(MILLIS_USE_TIMERD0)
      overflows = timer_overflow_count;
    #else
      overflows=timer_millis;
    #endif

    #if defined(MILLIS_USE_TIMERA0)
      ticks = (TIME_TRACKING_TIMER_PERIOD)-TCA0.SPLIT.HCNT;
    #elif defined(MILLIS_USE_TIMERA1)
      ticks = (TIME_TRACKING_TIMER_PERIOD)-TCA1.SPLIT.HCNT;
    #elif defined(MILLIS_USE_TIMERD0)
      TCD0.CTRLE=TCD_SCAPTUREA_bm;
      while(!(TCD0.STATUS&TCD_CMDRDY_bm)); //wait for sync - should be only one iteration of this loop
      ticks=TCD0.CAPTUREA;
    #else
      ticks = _timer->CNT;
    #endif //end getting ticks
    /* If the timer overflow flag is raised, and the ticks we read are low, then the timer has rolled over but
    ISR has not fired. If we already read a high value of ticks, either we read it just before the overflow,
    so we shouldn't increment overflows, or interrupts are disabled and micros isn't expected to work so it doesn't matter
    */
    #if defined(MILLIS_USE_TIMERD0)
      if ((TCD0.INTFLAGS & TCD_OVF_bm) && !(ticks&0xFF00)){
    #elif defined(MILLIS_USE_TIMERA0)
      if ((TCA0.SPLIT.INTFLAGS & TCA_SPLIT_HUNF_bm ) && !(ticks&0x80)){
    #elif defined(MILLIS_USE_TIMERA1)
      if ((TCA1.SPLIT.INTFLAGS & TCA_SPLIT_HUNF_bm ) && !(ticks&0x80)){
    #else //timerb
      if ((_timer->INTFLAGS & TCB_CAPT_bm) && !(ticks&0xFF00)) {
    #endif
    #if ((defined(MILLIS_USE_TIMERB0)||defined(MILLIS_USE_TIMERB1)|| defined(MILLIS_USE_TIMERB2)|| defined(MILLIS_USE_TIMERB3)|| defined(MILLIS_USE_TIMERB4)) &&(F_CPU>1000000))
      overflows++;
    #else
      overflows+=2;
    #endif
    }

    //end getting ticks

    /* Restore state */
    SREG = status;
    #if defined(MILLIS_USE_TIMERD0)
      #error "Timer D is not supported yet"
      #if (F_CPU==20000000UL || F_CPU==10000000UL || F_CPU==5000000UL)
        uint8_t ticks_l=ticks>>1;
        ticks=ticks+ticks_l+((ticks_l>>2)-(ticks_l>>4)+(ticks_l>>7));
        microseconds = overflows * (TIME_TRACKING_CYCLES_PER_OVF/(20))
              + ticks; // speed optimization via doing math with smaller datatypes, since we know high byte is 1 or 0
              // + ticks +(ticks>>1)+(ticks>>3)-(ticks>>5)+(ticks>>8))
      #else
        microseconds = ((overflows * (TIME_TRACKING_CYCLES_PER_OVF/(16)))
              + (ticks * ((TIME_TRACKING_CYCLES_PER_OVF)/(16)/TIME_TRACKING_TIMER_PERIOD)));
      #endif
    #elif (defined(MILLIS_USE_TIMERB0)||defined(MILLIS_USE_TIMERB1)||defined(MILLIS_USE_TIMERB2)||defined(MILLIS_USE_TIMERB3)||defined(MILLIS_USE_TIMERB4))
        // Oddball clock speeds
      #if (F_CPU==44000000UL)
        ticks=ticks>>4;
        microseconds = overflows*1000+(ticks-(ticks>>2)-(ticks>>5)+(ticks>>7));
      #elif (F_CPU==36000000UL)
        ticks=ticks>>4;
        microseconds = overflows*1000+(ticks-(ticks>>3)+(ticks>>6));
      #elif (F_CPU==28000000UL)
        ticks=ticks>>4;
        microseconds = overflows*1000+(ticks+(ticks>>3)+(ticks>>6)+(ticks>>8));
        // Multiples of 12
      #elif (F_CPU==48000000UL)
        ticks=ticks>>5;
        microseconds = overflows*1000+(ticks+(ticks>>2)+(ticks>>4)+(ticks>>5));
      #elif (F_CPU==24000000UL)
        ticks=ticks>>4;
        microseconds = overflows*1000+(ticks+(ticks>>2)+(ticks>>4)+(ticks>>5));
      #elif (F_CPU==12000000UL)
        ticks=ticks>>3;
        microseconds = overflows*1000+(ticks+(ticks>>2)+(ticks>>4)+(ticks>>5));
        // multiples of 10
      #elif (F_CPU==40000000UL)
        ticks=ticks>>4;
        microseconds = overflows*1000+(ticks-(ticks>>2)+(ticks>>4)-(ticks>>6));
      #elif (F_CPU==20000000UL)
        ticks=ticks>>3;
        microseconds = overflows*1000+(ticks-(ticks>>2)+(ticks>>4)-(ticks>>6));
      #elif (F_CPU==10000000UL)
        ticks=ticks>>2;
        microseconds = overflows*1000+(ticks-(ticks>>2)+(ticks>>4)-(ticks>>6));
      #elif (F_CPU==5000000UL)
        ticks=ticks>>1;
        microseconds = overflows*1000+(ticks-(ticks>>2)+(ticks>>4)-(ticks>>6));
        // powers of 2
      #elif (F_CPU==32000000UL)
        microseconds = overflows*1000+(ticks>>4);
      #elif (F_CPU==16000000UL)
        microseconds = overflows*1000+(ticks>>3);
      #elif (F_CPU==8000000UL)
        microseconds = overflows*1000+(ticks>>2);
      #elif (F_CPU==4000000UL)
        microseconds = overflows*1000+(ticks>>1);
      #else //(F_CPU==1000000UL - here clock is running at system clock instead of half system clock.
            // also works at 2MHz, since we use CLKPER for 1MHz vs CLKPER/2 for all others.
       microseconds = overflows*1000+ticks;
      #endif
    #else //TCA
      #if (F_CPU==48000000UL && TIME_TRACKING_TICKS_PER_OVF==255 && TIME_TRACKING_TIMER_DIVIDER==64)
        microseconds = (overflows * clockCyclesToMicroseconds(TIME_TRACKING_CYCLES_PER_OVF))
            + (ticks+(ticks>>2)+(ticks>>4)+(ticks>>5));
      #elif (F_CPU==44000000UL && TIME_TRACKING_TICKS_PER_OVF==255 && TIME_TRACKING_TIMER_DIVIDER==64)
        microseconds = (overflows * clockCyclesToMicroseconds(TIME_TRACKING_CYCLES_PER_OVF))
            + (ticks+(ticks>>1)-(ticks>>4)+(ticks>>6));
      #elif (F_CPU==40000000UL && TIME_TRACKING_TICKS_PER_OVF==255 && TIME_TRACKING_TIMER_DIVIDER==64)
        microseconds = (overflows * clockCyclesToMicroseconds(TIME_TRACKING_CYCLES_PER_OVF))
            + (ticks+(ticks>>1)+(ticks>>3)-(ticks>>5));
      #elif (F_CPU==36000000UL && TIME_TRACKING_TICKS_PER_OVF==255 && TIME_TRACKING_TIMER_DIVIDER==64)
        microseconds = (overflows * clockCyclesToMicroseconds(TIME_TRACKING_CYCLES_PER_OVF))
            + (ticks*2-(ticks>>2)+(ticks>>5));
      #elif (F_CPU==28000000UL && TIME_TRACKING_TICKS_PER_OVF==255 && TIME_TRACKING_TIMER_DIVIDER==64)
        microseconds = (overflows * clockCyclesToMicroseconds(TIME_TRACKING_CYCLES_PER_OVF))
            + (ticks*2+(ticks>>2)+(ticks>>5)+(ticks>>6));
      #elif (F_CPU==24000000UL && TIME_TRACKING_TICKS_PER_OVF==255 && TIME_TRACKING_TIMER_DIVIDER==64)
        microseconds = (overflows * clockCyclesToMicroseconds(TIME_TRACKING_CYCLES_PER_OVF))
            + (ticks*3-(ticks>>2)-(ticks>>4)-(ticks>>5));
      #elif (F_CPU==20000000UL && TIME_TRACKING_TICKS_PER_OVF==255 && TIME_TRACKING_TIMER_DIVIDER==64)
        microseconds = (overflows * clockCyclesToMicroseconds(TIME_TRACKING_CYCLES_PER_OVF))
            + (ticks*3+(ticks>>2)-(ticks>>4));
      #elif (F_CPU==12000000UL && TIME_TRACKING_TICKS_PER_OVF==255 && TIME_TRACKING_TIMER_DIVIDER==64)
        microseconds = (overflows * clockCyclesToMicroseconds(TIME_TRACKING_CYCLES_PER_OVF))
            + (ticks*5+(ticks>>2)+(ticks>>4)+(ticks>>5));
      #elif (F_CPU==10000000UL && TIME_TRACKING_TICKS_PER_OVF==255 && TIME_TRACKING_TIMER_DIVIDER==64)
        microseconds = (overflows * clockCyclesToMicroseconds(TIME_TRACKING_CYCLES_PER_OVF))
            + (ticks*6+(ticks>>1)-(ticks>>3));
      #elif (F_CPU==5000000UL && TIME_TRACKING_TICKS_PER_OVF==255 && TIME_TRACKING_TIMER_DIVIDER==16)
        microseconds = (overflows * clockCyclesToMicroseconds(TIME_TRACKING_CYCLES_PER_OVF))
            + (ticks*3+(ticks>>2)-(ticks>>4));
      #else
        #if (TIME_TRACKING_TIMER_DIVIDER%(F_CPU/1000000))
          #warning "Millis timer divider and frequency unsupported, inaccurate micros times will be returned."
        #endif
        microseconds = ((overflows * clockCyclesToMicroseconds(TIME_TRACKING_CYCLES_PER_OVF))
          + (ticks * (clockCyclesToMicroseconds(TIME_TRACKING_CYCLES_PER_OVF)/TIME_TRACKING_TIMER_PERIOD)));
      #endif
    #endif //end of timer-specific part of micros calculations
    return microseconds;
  }
#endif //end of non-RTC micros code


#endif //end of non-DISABLE_MILLIS code

#if !(defined(DISABLE_MILLIS) || defined(MILLIS_USE_TIMERRTC)) //delay implementation when we do have micros()
void delay(unsigned long ms)
{
  uint32_t start_time = micros(), delay_time = 1000*ms;

  /* Calculate future time to return */
  uint32_t return_time = start_time + delay_time;

  /* If return time overflows */
  if(return_time < delay_time){
    /* Wait until micros overflows */
    while(micros() > return_time);
  }

  /* Wait until return time */
  while(micros() < return_time);
}
#else //delay implementation when we do not
void delay(unsigned long ms)
{
  while(ms--){
    delayMicroseconds(1000);
  }
}
#endif


/* Delay for the given number of microseconds.  Assumes a 1, 4, 5, 8, 10, 16, or 20 MHz clock. */
void delayMicroseconds(unsigned int us)
{
  // call = 4 cycles + 2 to 4 cycles to init us(2 for constant delay, 4 for variable)

  // calling avrlib's delay_us() function with low values (e.g. 1 or
  // 2 microseconds) gives delays longer than desired.
  //delay_us(us);
#if F_CPU >= 20000000L
  // for the 20 MHz clock on rare Arduino boards

  // for a one-microsecond delay, simply return.  the overhead
  // of the function call takes 18 (20) cycles, which is 1us
  __asm__ __volatile__ (
    "nop" "\n\t"
    "nop" "\n\t"
    "nop" "\n\t"
    "nop"); //just waiting 4 cycles
  if (us <= 1) return; //  = 3 cycles, (4 when true)

  // the following loop takes a 1/5 of a microsecond (4 cycles)
  // per iteration, so execute it five times for each microsecond of
  // delay requested.
  us = (us << 2) + us; // x5 us, = 7 cycles

  // account for the time taken in the preceding commands.
  // we just burned 26 (28) cycles above, remove 7, (7*4=28)
  // us is at least 10 so we can subtract 7
  us -= 7; // 2 cycles

#elif F_CPU >= 16000000L
  // for the 16 MHz clock on most Arduino boards

  // for a one-microsecond delay, simply return.  the overhead
  // of the function call takes 14 (16) cycles, which is 1us
  if (us <= 1) return; //  = 3 cycles, (4 when true)

  // the following loop takes 1/4 of a microsecond (4 cycles)
  // per iteration, so execute it four times for each microsecond of
  // delay requested.
  us <<= 2; // x4 us, = 4 cycles

  // account for the time taken in the preceding commands.
  // we just burned 19 (21) cycles above, remove 5, (5*4=20)
  // us is at least 8 so we can subtract 5
  us -= 5; // = 2 cycles,

#elif F_CPU >= 10000000L
  // for 10MHz (20MHz/2)

  // for a 1 microsecond delay, simply return.  the overhead
  // of the function call takes 14 (16) cycles, which is 1.5us
  if (us <= 1) return; //  = 3 cycles, (4 when true)

  // the following loop takes 2/5 of a microsecond (4 cycles)
  // per iteration, so execute it 2.5 times for each microsecond of
  // delay requested.
  us = (us << 1) + (us>>1); // x2.5 us, = 5 cycles

  // account for the time taken in the preceding commands.
  // we just burned 20 (22) cycles above, remove 5, (5*4=20)
  // us is at least 6 so we can subtract 5
  us -= 5; //2 cycles

#elif F_CPU >= 8000000L
  // for the 8 MHz internal clock

  // for a 1 and 2 microsecond delay, simply return.  the overhead
  // of the function call takes 14 (16) cycles, which is 2us
  if (us <= 2) return; //  = 3 cycles, (4 when true)

  // the following loop takes 1/2 of a microsecond (4 cycles)
  // per iteration, so execute it twice for each microsecond of
  // delay requested.
  us <<= 1; //x2 us, = 2 cycles

  // account for the time taken in the preceding commands.
  // we just burned 17 (19) cycles above, remove 4, (4*4=16)
  // us is at least 6 so we can subtract 4
  us -= 4; // = 2 cycles
#elif F_CPU >= 5000000L
  // for 5MHz (20MHz / 4)

  // for a 1 ~ 3 microsecond delay, simply return.  the overhead
  // of the function call takes 14 (16) cycles, which is 3us
  if (us <= 3) return; //  = 3 cycles, (4 when true)

  // the following loop takes 4/5th microsecond (4 cycles)
  // per iteration, so we want to add it to 1/4th of itself
  us +=us>>2;
  us -= 2; // = 2 cycles

#elif F_CPU >= 4000000L
  // for that unusual 4mhz clock...

  // for a 1 ~ 4 microsecond delay, simply return.  the overhead
  // of the function call takes 14 (16) cycles, which is 4us
  if (us <= 4) return; //  = 3 cycles, (4 when true)

  // the following loop takes 1 microsecond (4 cycles)
  // per iteration, so nothing to do here! \o/

  us -= 2; // = 2 cycles


#else
  // for the 1 MHz internal clock (default settings for common AVR microcontrollers)
  // the overhead of the function calls is 14 (16) cycles
  if (us <= 16) return; //= 3 cycles, (4 when true)
  if (us <= 25) return; //= 3 cycles, (4 when true), (must be at least 25 if we want to subtract 22)

  // compensate for the time taken by the preceding and next commands (about 22 cycles)
  us -= 22; // = 2 cycles
  // the following loop takes 4 microseconds (4 cycles)
  // per iteration, so execute it us/4 times
  // us is at least 4, divided by 4 gives us 1 (no zero delay bug)
  us >>= 2; // us div 4, = 4 cycles


#endif



  // busy wait
  __asm__ __volatile__ (
    "1: sbiw %0,1" "\n\t" // 2 cycles
    "brne 1b" : "=w" (us) : "0" (us) // 2 cycles
  );
  // return = 4 cycles
}


#ifndef DISABLE_MILLIS
  void stop_millis()
  { // Disable the interrupt:
    #if defined(MILLIS_USE_TIMERA0)
      TCA0.SPLIT.INTCTRL &= (~TCA_SPLIT_HUNF_bm);
    #elif defined(MILLIS_USE_TIMERA1)
      TCA1.SPLIT.INTCTRL &= (~TCA_SPLIT_HUNF_bm);
    #elif defined(MILLIS_USE_TIMERD0)
      TCD0.INTCTRL&=0xFE;
    #elif defined(MILLIS_USE_TIMERRTC)
      RTC.INTCTRL&=0xFE;
      RTC.CTRLA&=0xFE;
    #else
      _timer->INTCTRL &= ~TCB_CAPT_bm;
    #endif
  }

  void restart_millis()
  {
    // Call this to restart millis after it has been stopped and/or millis timer has been molested by other routines.
    // This resets key registers to their expected states.

    #if defined(MILLIS_USE_TIMERA0)
      TCA0.SPLIT.CTRLA = 0x00;
      TCA0.SPLIT.CTRLD = TCA_SPLIT_SPLITM_bm;
      TCA0.SPLIT.HPER    = PWM_TIMER_PERIOD;
    #elif defined(MILLIS_USE_TIMERA1)
      TCA1.SPLIT.CTRLA = 0x00;
      TCA1.SPLIT.CTRLD = TCA_SPLIT_SPLITM_bm;
      TCA1.SPLIT.HPER    = PWM_TIMER_PERIOD;
    #elif defined(MILLIS_USE_TIMERD0)
      TCD0.CTRLA=0x00;
      while(TCD0.STATUS & 0x01);
    #elif (defined(MILLIS_USE_TIMERB0) || defined(MILLIS_USE_TIMERB1)|| defined(MILLIS_USE_TIMERB2)|| defined(MILLIS_USE_TIMERB3)|| defined(MILLIS_USE_TIMERB4)) //It's a type b timer
      _timer->CTRLB = 0;
    #endif
    init_millis();
  }

  void init_millis()
  {
    #if defined(MILLIS_USE_TIMERA0)
      TCA0.SPLIT.INTCTRL |= TCA_SPLIT_HUNF_bm;
    #elif defined(MILLIS_USE_TIMERA1)
      TCA1.SPLIT.INTCTRL |= TCA_SPLIT_HUNF_bm;
    #elif defined(MILLIS_USE_TIMERD0)
      TCD0.CMPBCLR=TIME_TRACKING_TIMER_PERIOD; //essentially, this is TOP
      TCD0.INTCTRL=0x01;//enable interrupt
      TCD0.CTRLB=0x00; //oneramp mode
      TCD0.CTRLC=0x80;
      TCD0.CTRLA=TIMERD0_PRESCALER|0x01; //set clock source and enable!
    #elif defined(MILLIS_USE_TIMERRTC)
      while(RTC.STATUS); //if RTC is currently busy, spin until it's not.
      // to do: add support for RTC timer initialization
      RTC.PER=0xFFFF;
    #ifdef MILLIS_USE_TIMERRTC_XTAL
      _PROTECTED_WRITE(CLKCTRL.XOSC32KCTRLA,0x03);
      RTC.CLKSEL=2; //external crystal
    #else
      _PROTECTED_WRITE(CLKCTRL.OSC32KCTRLA,0x02);
      //RTC.CLKSEL=0; this is the power on value
    #endif
      RTC.INTCTRL=0x01; //enable overflow interupt
      RTC.CTRLA=(RTC_RUNSTDBY_bm|RTC_RTCEN_bm|RTC_PRESCALER_DIV32_gc);//fire it up, prescale by 32.

    #else //It's a type b timer

      _timer->CCMP = TIME_TRACKING_TIMER_PERIOD;
      // Enable timer interrupt, but clear the rest of register
      _timer->INTCTRL = TCB_CAPT_bm;
      // Clear timer mode (since it will have been set as PWM by init())
      _timer->CTRLB=0;
      // CLK_PER/1 is 0b00,. CLK_PER/2 is 0b01, so bitwise OR of valid divider with enable works
      _timer->CTRLA = TIME_TRACKING_TIMER_DIVIDER|TCB_ENABLE_bm;  // Keep this last before enabling interrupts to ensure tracking as accurate as possible
    #endif
  }

  void set_millis(uint32_t newmillis)
  {
    #if defined(MILLIS_USE_TIMERRTC)
      //timer_overflow_count=newmillis>>16;
      // millis = 61/64(timer_overflow_count<<16 + RTC.CNT)
      uint16_t temp=(newmillis%61)<<6;
      newmillis=(newmillis/61)<<6;
      temp=temp/61;
      newmillis+=temp;
      timer_overflow_count=newmillis>>16;
      while(RTC.STATUS&RTC_CNTBUSY_bm); //wait if RTC busy
      RTC.CNT=newmillis&0xFFFF;
    #else
      timer_millis=newmillis;
    #endif
  }

#endif



void init()
{
  // this needs to be called before setup() or some functions won't
  // work there

/******************************** CLOCK STUFF *********************************/
  // Warning: some of these are WAY outside the specs, see comments
  // For 12, 8. 4, 2, and 1, one can argue whether it should be
  // prescaled from 24 or 16 - the advantage of this is that it
  // would allow use on the PLL to generate high speed PWM
  // which might be particularly appealing at those very low speeds
  // On the other hand, that very low speed might be chosen for low
  // power, in which case running the oscillator faster would be
  // undesirable...

  #if (F_CPU == 48000000)
    /* No division on clock - almost guaranteed not to work */
    _PROTECTED_WRITE(CLKCTRL_OSCHFCTRLA, (CLKCTRL_OSCHFCTRLA & ~CLKCTRL_FREQSEL_gm ) | (0x0F<< CLKCTRL_FREQSEL_gp ));

  #elif (F_CPU == 44000000)
    /* No division on clock - almost guaranteed not to work  */
    _PROTECTED_WRITE(CLKCTRL_OSCHFCTRLA, (CLKCTRL_OSCHFCTRLA & ~CLKCTRL_FREQSEL_gm ) | (0x0E<< CLKCTRL_FREQSEL_gp ));

  #elif (F_CPU == 40000000)
    /* No division on clock - almost guaranteed not to work  */
    _PROTECTED_WRITE(CLKCTRL_OSCHFCTRLA, (CLKCTRL_OSCHFCTRLA & ~CLKCTRL_FREQSEL_gm ) | (0x0D<< CLKCTRL_FREQSEL_gp ));

  #elif (F_CPU == 36000000)
    /* No division on clock  - unlikely to work */
    _PROTECTED_WRITE(CLKCTRL_OSCHFCTRLA, (CLKCTRL_OSCHFCTRLA & ~CLKCTRL_FREQSEL_gm ) | (0x0C<< CLKCTRL_FREQSEL_gp ));

  #elif (F_CPU == 32000000)
    /* No division on clock - seems to work? */
    _PROTECTED_WRITE(CLKCTRL_OSCHFCTRLA, (CLKCTRL_OSCHFCTRLA & ~CLKCTRL_FREQSEL_gm ) | (0x0B<< CLKCTRL_FREQSEL_gp ));

  #elif (F_CPU == 28000000)
    /* No division on clock - seems to work? */
    _PROTECTED_WRITE(CLKCTRL_OSCHFCTRLA, (CLKCTRL_OSCHFCTRLA & ~CLKCTRL_FREQSEL_gm ) | (0x0A<< CLKCTRL_FREQSEL_gp ));

  #elif (F_CPU == 24000000)
    /* No division on clock - fastest speed that's in spec */
    _PROTECTED_WRITE(CLKCTRL_OSCHFCTRLA, (CLKCTRL_OSCHFCTRLA & ~CLKCTRL_FREQSEL_gm ) | (0x09<< CLKCTRL_FREQSEL_gp ));

  #elif (F_CPU == 20000000)
    /* No division on clock */
    _PROTECTED_WRITE(CLKCTRL_OSCHFCTRLA, (CLKCTRL_OSCHFCTRLA & ~CLKCTRL_FREQSEL_gm ) | (0x08<< CLKCTRL_FREQSEL_gp ));

  #elif (F_CPU == 16000000)
    /* No division on clock */
    _PROTECTED_WRITE(CLKCTRL_OSCHFCTRLA, (CLKCTRL_OSCHFCTRLA & ~CLKCTRL_FREQSEL_gm ) | (0x07<< CLKCTRL_FREQSEL_gp ));

  #elif (F_CPU == 12000000)
    /* should it be 24MHz prescaled by 2? */
    _PROTECTED_WRITE(CLKCTRL_OSCHFCTRLA, (CLKCTRL_OSCHFCTRLA & ~CLKCTRL_FREQSEL_gm ) | (0x06<< CLKCTRL_FREQSEL_gp ));

  #elif (F_CPU == 10000000)
    /* 20 prescaled by 2 */
    _PROTECTED_WRITE(CLKCTRL_MCLKCTRLB,0x01);
    _PROTECTED_WRITE(CLKCTRL_OSCHFCTRLA, (CLKCTRL_OSCHFCTRLA & ~CLKCTRL_FREQSEL_gm ) | (0x08<< CLKCTRL_FREQSEL_gp ));

  #elif (F_CPU == 8000000)
    /* Should it be 16MHz prescaled by 2? */
    _PROTECTED_WRITE(CLKCTRL_OSCHFCTRLA, (CLKCTRL_OSCHFCTRLA & ~CLKCTRL_FREQSEL_gm ) | (0x05<< CLKCTRL_FREQSEL_gp ));

  #elif (F_CPU == 5000000)
    /* 20 prescaled by 4 */
    _PROTECTED_WRITE(CLKCTRL_MCLKCTRLB,0x01);
    _PROTECTED_WRITE(CLKCTRL_OSCHFCTRLA, (CLKCTRL_OSCHFCTRLA & ~CLKCTRL_FREQSEL_gm ) | (0x03<< CLKCTRL_FREQSEL_gp ));

  #elif (F_CPU == 4000000)
    /* Should it be 16MHz prescaled by 4? */
    _PROTECTED_WRITE(CLKCTRL_OSCHFCTRLA, (CLKCTRL_OSCHFCTRLA & ~CLKCTRL_FREQSEL_gm ) | (0x03<< CLKCTRL_FREQSEL_gp ));

  #elif (F_CPU == 2000000)
    /* Should it be 16MHz prescaled by 8? */
    _PROTECTED_WRITE(CLKCTRL_OSCHFCTRLA, (CLKCTRL_OSCHFCTRLA & ~CLKCTRL_FREQSEL_gm ) | (0x01<< CLKCTRL_FREQSEL_gp ));

  #elif (F_CPU == 1000000)
    /* Should it be 16MHz prescaled by 16? */
    _PROTECTED_WRITE(CLKCTRL_OSCHFCTRLA, (CLKCTRL_OSCHFCTRLA & ~CLKCTRL_FREQSEL_gm ) | (0x00<< CLKCTRL_FREQSEL_gp ));
  #else

    #ifndef F_CPU
      #error "F_CPU not defined"
    #else
      #error "F_CPU defined as an unsupported value"
    #endif
  #endif


/********************************* ADC ****************************************/

  #if defined(ADC0)
    #ifndef SLOWADC
      /* ADC clock 1 MHz to 1.25 MHz at frequencies supported by megaTinyCore
      Unlike the classic AVRs, which demand 50~200 kHz, for these, the datasheet
      spec's a clock speed of 150 kHz to 2 MHz. We hypothesize that lower clocks provide better
      response to high impedance signals, since the sample and hold circuit will
      be connected to the pin for longer, though the datasheet does not explicitly
      state that this is the case. Just like on megaTinyCore, use samplen to compensate */

      #if F_CPU >= 32000000
        ADC0.CTRLC = ADC_PRESC_DIV32_gc; //1 MHz
      #elif F_CPU >= 28000000
        ADC0.CTRLC = ADC_PRESC_DIV28_gc; //1 MHz
      #elif F_CPU >= 24000000
        ADC0.CTRLC = ADC_PRESC_DIV24_gc; //1 MHz
      #elif F_CPU >= 20000000
        ADC0.CTRLC = ADC_PRESC_DIV20_gc; //1 MHz
      #elif F_CPU >= 16000000
        ADC0.CTRLC = ADC_PRESC_DIV16_gc; //1 MHz
      #elif F_CPU >= 12000000
        ADC0.CTRLC = ADC_PRESC_DIV12_gc; //1 MHz
      #elif F_CPU >= 8000000
        ADC0.CTRLC = ADC_PRESC_DIV8_gc;  //1 MHz
      #elif F_CPU >= 4000000
        ADC0.CTRLC = ADC_PRESC_DIV4_gc;  //1 MHz
      #else  // 1 MHz / 2 = 500 kHz - the lowest setting
        ADC0.CTRLC = ADC_PRESC_DIV2_gc;
      #endif
      ADC0.SAMPCTRL=14; //16 ADC clock sampling time - should be about the same amount of *time* as originally?
    #else //if SLOWADC is defined - as of 2.0.0 this option isn't exposed.
      /* ADC clock around 200 kHz - datasheet spec's 125 kHz to 1.5 MHz */
      #if F_CPU >= 32000000
        ADC0.CTRLC = ADC_PRESC_DIV128_gc; //250 kHz
      #elif F_CPU >= 24000000
        ADC0.CTRLC = ADC_PRESC_DIV96_gc; //250 kHz
      #elif F_CPU >= 16000000
        ADC0.CTRLC = ADC_PRESC_DIV64_gc; //250 kHz
      #elif F_CPU >= 12000000
        ADC0.CTRLC = ADC_PRESC_DIV48_gc; //250 kHz
      #elif F_CPU >= 8000000
        ADC0.CTRLC = ADC_PRESC_DIV32_gc; //250 kHz
      #elif F_CPU >= 4000000
        ADC0.CTRLC = ADC_PRESC_DIV20_gc; //200 kHz
      #else  // 1 MHz / 4 = 250 kHz
        ADC0.CTRLC = ADC_PRESC_DIV4_gc;
      #endif
    #endif

    /* Enable ADC */
    ADC0.CTRLD = ADC_INITDLY_DLY16_gc;
    ADC0.CTRLA = ADC_ENABLE_bm | ADC_RESSEL_10BIT_gc;

#ifdef SIMPLE_ADC_WORKAROUND
    ADC0.MUXPOS=0x7F;
#endif
    analogReference(VDD);
    DACReference(VDD);
  #endif

  setup_timers();

  #ifndef DISABLE_MILLIS
  init_millis();
  #endif //end #ifndef DISABLE_MILLIS
/*************************** ENABLE GLOBAL INTERRUPTS *************************/

  sei();
}



void setup_timers() {

    /*  TYPE A TIMER   */

    /* PORTMUX setting for TCA - don't need to set because using default */
    //PORTMUX.CTRLA = PORTMUX_TCA00_DEFAULT_gc;

    /* Enable Split Mode */
    TCA0.SPLIT.CTRLD = TCA_SPLIT_SPLITM_bm;

    //Only 1 WGM so no need to specifically set up.

    /* Period setting, 8-bit register in SPLIT mode */
    TCA0.SPLIT.LPER    = PWM_TIMER_PERIOD;
    TCA0.SPLIT.HPER    = PWM_TIMER_PERIOD;

    /* Default duty 50%, will re-assign in analogWrite() */
    //TODO: replace with for loop to make this smaller;
    TCA0.SPLIT.LCMP0 = PWM_TIMER_COMPARE;
    TCA0.SPLIT.LCMP1 = PWM_TIMER_COMPARE;
    TCA0.SPLIT.LCMP2 = PWM_TIMER_COMPARE;
    TCA0.SPLIT.HCMP0 = PWM_TIMER_COMPARE;
    TCA0.SPLIT.HCMP1 = PWM_TIMER_COMPARE;
    TCA0.SPLIT.HCMP2 = PWM_TIMER_COMPARE;

    /* Use DIV64 prescaler (giving 250kHz clock), enable TCA timer */
  #if (F_CPU > 5000000) //use 64 divider
    TCA0.SPLIT.CTRLA = (TCA_SPLIT_CLKSEL_DIV64_gc) | (TCA_SINGLE_ENABLE_bm);
  #elif (F_CPU > 1000000)
    TCA0.SPLIT.CTRLA = (TCA_SPLIT_CLKSEL_DIV16_gc) | (TCA_SINGLE_ENABLE_bm);
  #else //TIME_TRACKING_TIMER_DIVIDER==8
    TCA0.SPLIT.CTRLA = (TCA_SPLIT_CLKSEL_DIV8_gc) | (TCA_SINGLE_ENABLE_bm);
  #endif
  #ifdef TCA1
    TCA1.SPLIT.CTRLD = TCA_SPLIT_SPLITM_bm;

    //Only 1 WGM so no need to specifically set up.

    /* Period setting, 8-bit register in SPLIT mode */
    TCA1.SPLIT.LPER    = PWM_TIMER_PERIOD;
    TCA1.SPLIT.HPER    = PWM_TIMER_PERIOD;

    /* Default duty 50%, will re-assign in analogWrite() */
    //TODO: replace with for loop to make this smaller;
    TCA1.SPLIT.LCMP0 = PWM_TIMER_COMPARE;
    TCA1.SPLIT.LCMP1 = PWM_TIMER_COMPARE;
    TCA1.SPLIT.LCMP2 = PWM_TIMER_COMPARE;
    TCA1.SPLIT.HCMP0 = PWM_TIMER_COMPARE;
    TCA1.SPLIT.HCMP1 = PWM_TIMER_COMPARE;
    TCA1.SPLIT.HCMP2 = PWM_TIMER_COMPARE;

    /* Use DIV64 prescaler (giving 250kHz clock), enable TCA timer */
  #if (F_CPU > 5000000) //use 64 divider
    TCA1.SPLIT.CTRLA = (TCA_SPLIT_CLKSEL_DIV64_gc) | (TCA_SINGLE_ENABLE_bm);
  #elif (F_CPU > 1000000) // use 16 divider for 5MHz, 4 MHz and 2MHz
    TCA1.SPLIT.CTRLA = (TCA_SPLIT_CLKSEL_DIV16_gc) | (TCA_SINGLE_ENABLE_bm);
  #else // for 1 MHz or less, only divide by 8
    TCA1.SPLIT.CTRLA = (TCA_SPLIT_CLKSEL_DIV8_gc) | (TCA_SINGLE_ENABLE_bm);
  #endif
  #endif

  PORTMUX.TCAROUTEA = TCA0_PINS
    #if defined(TCA1)
                         | TCA1_PINS
    #endif
          ;

    /*    TYPE B TIMERS  */
  // Set up routing (defined in pins_arduino.h)
  PORTMUX.TCBROUTEA = 0
  #if defined(TCB0)
                        | TCB0_PINS
  #endif
  #if defined(TCB1)
                        | TCB1_PINS
  #endif
  #if defined(TCB2)
                        | TCB2_PINS
  #endif
  #if defined(TCB3)
                        | TCB3_PINS
  #endif
  #if defined(TCB4)
                        | TCB4_PINS
  #endif
        ;

    // Start with TCB0
    TCB_t *timer_B = (TCB_t *)&TCB0;

  // Find end timer
  #if defined(TCB4)
    TCB_t *timer_B_end = (TCB_t *)&TCB4;
  #elif defined(TCB3)
    TCB_t *timer_B_end = (TCB_t *)&TCB3;
  #elif defined(TCB2)
    TCB_t *timer_B_end = (TCB_t *)&TCB2;
  #elif defined(TCB1)
    TCB_t *timer_B_end = (TCB_t *)&TCB1;
  #else
    TCB_t *timer_B_end = (TCB_t *)&TCB0;
  #endif

    // Timer B Setup loop for TCB[0:end]
    do
    {
      // 8 bit PWM mode, but do not enable output yet, will do in analogWrite()
      timer_B->CTRLB = (TCB_CNTMODE_PWM8_gc);

      // Assign 8-bit period
      timer_B->CCMPL = PWM_TIMER_PERIOD;

      // default duty 50%, set when output enabled
      timer_B->CCMPH = PWM_TIMER_COMPARE;

      // Use TCA clock (250kHz) and enable
      // (sync update commented out, might try to synchronize later

      timer_B->CTRLA = (TCB_CLKSEL_TCA0_gc)
                       //|(TCB_SYNCUPD_bm)
                       | (TCB_ENABLE_bm);

      // Increment pointer to next TCB instance
      timer_B++;

      // Stop when pointing to TCB3
    } while (timer_B <= timer_B_end);


  #ifdef TCD0
      #if  (!(defined(MILLIS_USE_TIMERD0_A0) || defined(MILLIS_USE_TIMERD0)))
      PORTMUX.TCDROUTEA=TCD0_PINS;
      TCD0.CMPBCLR=510; //Count to 510
      TCD0.CMPACLR=510;
      TCD0.CTRLC=0x80; //WOD outputs PWM B, WOC outputs PWM A
      TCD0.CTRLB=0x00; //One Slope
      TCD0.CTRLA=TIMERD0_CTRLA_SETTING; //OSC20M prescaled by 32, gives ~1.2 khz PWM at 20MHz.
    #endif
  #endif

}
