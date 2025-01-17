/**
 * MIT License
 *
 * Copyright (c) 2021 R. Dunbar Poor <rdpoor@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

// =============================================================================
// Includes

#include "mu_time.h"
#include <msp430.h>

// =============================================================================
// Private types and definitions

// =============================================================================
// Private (forward) declarations

static uint16_t get_timer_l(void);
static void init_timer(void);
static void on_timer_tick(void);

// =============================================================================
// Local storage

static volatile uint16_t s_timer_h;  // high order 16 bits
static uint16_t s_safe_timer_h;      // high order 16 bits, sanitized

// =============================================================================
// Public code

/**
 * @brief Initialize the time system.  Must be called before any other time
 * functions are called.
 */
void mu_time_init(void) {
  init_timer();
  s_timer_h = 0;
  __enable_interrupt();
}

/**
 * @brief Get the current system time.
 *
 * @return A value representing the current time.
 */
mu_time_t mu_time_now(void) {
  uint16_t timer_l;

  do {
    // snapshot high order and low order bits of timer
    s_safe_timer_h = s_timer_h;
    timer_l = get_timer_l();
    // values are safe if high order bits haven't changed
  } while (s_safe_timer_h != s_timer_h);

  // assemble high and low 16 bits into a 32 bit value.
  return ((mu_time_t)s_safe_timer_h << 16) | (timer_l);
}

/**
 * @brief Add a time and a duration.
 *
 * `mu_time_offset` adds a time and a duration to produce a new time object.
 *
 * @param t1 a time object
 * @param dt a duration object
 * @return t1 offset by dt
 */
mu_time_t mu_time_offset(mu_time_t t1, mu_duration_t dt) { return t1 + dt; }

/**
 * @brief Take the difference between two time objects
 *
 * `mu_time_difference` subtracts t2 from t1 to produce a duration object.
 *
 * @param t1 A time object
 * @param t2 A time object
 * @return (t1-t2) as a duration object
 */
mu_duration_t mu_time_difference(mu_time_t t1, mu_time_t t2) { return t1 - t2; }

/**
 * @brief Return true if t1 is strictly before t2
 *
 * Note that if you want to know if t1 is before or equal to t2, you can use the
 * construct `!mu_time_follows(t2, t1)``
 *
 * @param t1 A time object
 * @param t2 A time object
 * @return true if t1 is strictly before t2, false otherwise.
 */
bool mu_time_precedes(mu_time_t t1, mu_time_t t2) { return t1 < t2; }

/**
 * @brief Return true if t1 is equal to t2
 *
 * @param t1 A time object
 * @param t2 A time object
 * @return true if t1 equals t2, false otherwise.
 */
bool mu_time_equals(mu_time_t t1, mu_time_t t2) { return t1 == t2; }

/**
 * @brief Return true if t1 is strictly after t2
 *
 * Note that if you want to know if t1 is equal to or after t2, you can use the
 * construct `!mu_time_precedes(t2, t1)``
 *
 * @param t1 A time object
 * @param t2 A time object
 * @return true if t1 is strictly after t2, false otherwise.
 */
bool mu_time_follows(mu_time_t t1, mu_time_t t2) { return t1 > t2; }

/**
 * @brief Convert a duration to milliseconds.
 *
 * @param dt A duration object
 * @return The duration in seconds
 */
mu_duration_ms_t mu_time_duration_to_ms(mu_duration_t dt) {
  return (dt * 1000) / RTC_FREQUENCY;
}

/**
 * @brief Convert milliseconds to a duration
 *
 * @param ms The duration in milliseconds
 * @return A duration object
 */
mu_duration_t mu_time_ms_to_duration(mu_duration_ms_t ms) {
  return (ms * RTC_FREQUENCY) / 1000;
}

#ifdef MU_FLOAT
/**
 * @brief Convert a duration to seconds.
 *
 * @param dt A duration object
 * @return The duration in seconds
 */
MU_FLOAT mu_time_duration_to_s(mu_duration_t dt){
#error "Provide a platform-specific implementation for mu_time_duration_to_s()"
}

/**
 * @brief Convert seconds to a duration.
 *
 * @param s The duration in seconds
 * @return A duration object
 */
mu_duration_t mu_time_s_to_duration(MU_FLOAT s) {
#error "Provide a platform-specific implementation for mu_time_s_to_duration()"
}

#endif // #ifdef MU_FLOAT

// =============================================================================
// Local (static) code

/**
 *  About the timer:
 *
 *  Goal:
 *
 *  User Timer T0_A in continuous count up mode, driven from the external 32KHz
 *  XTAL.  As a 16 bit counter, it will interrupt on overflow every 2 seconds
 *  (i.e. 2^16/32768).
 *
 *  We implement an "extended 32 bit counter" by incrementing a uint16_t value
 *  at every overflow.  The tick resolution is then 30.518 uSec, the maximum
 *  count is 131,072 seconds.
 */
static void init_timer(void) {
    TA0CTL = TASSEL__ACLK |      // 32768 Hz source
             MC__CONTINUOUS |    // continuous mode
             TAIE |              // Interrupt on overflow
             0; // TACLR;
}

static uint16_t get_timer_l(void) {
    return TA0R;
}

static void on_timer_tick(void) {
  s_timer_h += 1;
}

// Timer B0 interrupt service routine
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector = TIMER0_A1_VECTOR
__interrupt void Timer(void)
#elif defined(__GNUC__)
void __attribute__((interrupt(TIMER0_A1_VECTOR))) Timer_A0(void)
#else
#error Compiler not supported!
#endif
{
  on_timer_tick();
  TA0CTL &= ~TAIFG;                     // clear interrupt flag
  __bic_SR_register_on_exit(LPM3_bits); // Exit LPM3
}
