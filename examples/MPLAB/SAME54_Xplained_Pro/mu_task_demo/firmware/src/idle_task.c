/**
 * MIT License
 *
 * Copyright (c) 2020 R. Dunbar Poor <rdpoor@gmail.com>
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
// includes

#include "idle_task.h"
#include "definitions.h"
#include "kbd_task.h"
#include "mu_task_demo.h"
#include "mulib.h"
#include <stddef.h>

// =============================================================================
// local types and definitions

// don't sleep for less than 1ms (32 RTC tics)
#define MIN_SLEEP_DURATION mu_time_ms_to_duration(1)

// =============================================================================
// local (forward) declarations

static void *idle_task_fn(void *ctx, void *arg);
static bool is_ready_to_sleep(void);
static void will_sleep(void);
static void go_to_sleep(void);
static void did_wake(void);
static void rtc_callback(RTC_TIMER32_INT_MASK intCause, uintptr_t context);

// =============================================================================
// local storage

// =============================================================================
// public code

mu_task_t *idle_task_init(mu_task_t *idle_task) {

  // E54 XPlained Pro specific: put Ethernet interface into low-power state
  ETH_RESET_Clear();

  // E54 specific: use the low power regulator
  SUPC_SelectVoltageRegulator(SUPC_VREGSEL_BUCK);

  // MPLAB Harmony specific: register RTC callback and enable interrupts
  RTC_Timer32CallbackRegister(rtc_callback, (uintptr_t)NULL);
  RTC_Timer32InterruptEnable(RTC_TIMER32_INT_MASK_CMP0);

  // Initialize the idle task
  mu_task_init(idle_task, idle_task_fn, NULL, "Sleep/Idle");

  return idle_task;
}

// =============================================================================
// local (static) code

static void *idle_task_fn(void *ctx, void *arg) {
  // ctx is unused in idle task
  // scheduler is passed as the second argument.
  mu_sched_t *sched = (mu_sched_t *)arg;
  mu_sched_event_t *next_event = mu_sched_get_next_event(sched);

  if (is_ready_to_sleep()) {
    will_sleep();
    if (next_event) {
      // There is a future event: sleep until it arrives or skip sleeping if
      // the event is imminent.
      mu_time_t now = mu_sched_get_current_time(sched);
      mu_time_t then = next_event->time;
      if (mu_time_difference(then, now) > MIN_SLEEP_DURATION) {
        RTC_Timer32Compare0Set(then);
        go_to_sleep();
      }
    } else {
      // no future events are scheduled -- only an interrupt will wake us
      go_to_sleep();
    }
    did_wake();
  }
  return NULL;
}

static bool is_ready_to_sleep(void) {
  // If you have peripherals that are active or other factors that should
  // prevent the processor from going to sleep, you would check for it here
  // and return false to inhibit going to sleep.
  return true;
}

static void will_sleep(void) {
  // If you have any last-moment cleanup that needs to be done before the
  // processor goes to sleep, you would do it here.
  if (mu_task_demo_is_low_power_mode()) {
    kbd_task_set_low_power_mode(true);
  }
}

static void go_to_sleep(void) {
  // There appears to be a bug in the E54 silicon that causes the DFLL to lose
  // the state of the .FINE bits across STANDBY / wake events.  As a workaround,
  // we save the DFLL state prior to STANDBY and restore it upon waking.
  uint32_t saved_dfllval = OSCCTRL_REGS->OSCCTRL_DFLLVAL;
  PM_StandbyModeEnter();
  OSCCTRL_REGS->OSCCTRL_DFLLVAL = saved_dfllval;
}

static void did_wake(void) {
  // If you have anything that needs to be done when the processor wakes from
  // sleep, you would do it here.
  if (mu_task_demo_is_low_power_mode()) {
    kbd_task_set_low_power_mode(false);
  }
  asm("nop");
}

static void rtc_callback(RTC_TIMER32_INT_MASK intCause, uintptr_t context) {
  // Arrive here on an RTC interrupt, in this case, when the there is a compare
  // match.  We use the interrupt only to wake the processor from Standby mode.
  // Note: The system code has already cleared the interrupt bit.
  asm("nop");
}