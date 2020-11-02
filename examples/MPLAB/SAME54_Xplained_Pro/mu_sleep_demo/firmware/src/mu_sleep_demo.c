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

#include "mu_sleep_demo.h"
#include "button_task.h"
#include "definitions.h"
#include "idle_task.h"
#include "led_task.h"
#include "mulib.h"
#include <stddef.h>
#include <stdio.h>

// =============================================================================
// local types and definitions

#define MULIB_SLINKY_VERSION "1.0.0"

#define EVENT_QUEUE_CAPACITY 10
#define ISR_Q_CAPACITY 8           // must be a power of two

// =============================================================================
// local (forward) declarations

// =============================================================================
// local storage

// the scheduler state
static mu_sched_t s_sched;

// the backing store for scheduled events
static mu_sched_event_t s_event_queue[EVENT_QUEUE_CAPACITY];

// the queue to hold tasks queued from interrupt level, and its backing store
static mu_spscq_t s_isr_queue;
static mu_spscq_item_t s_isr_queue_items[ISR_Q_CAPACITY];

// the LED task
static mu_thunk_t s_led_task;
static led_ctx_t s_led_ctx;

// the Button task
static mu_thunk_t s_button_task;
static button_ctx_t s_button_ctx;

// the Idle task.
static mu_thunk_t s_idle_task;
// ... no context required

// =============================================================================
// public code

void mu_sleep_demo_init() {
  printf("\n\n# ===========\n");
  printf("# mu_sleep_demo %s: see https://github.com/rdpoor/mulib\n", MULIB_SLINKY_VERSION);

  // initialize the port-specific interface
  mu_vm_init();

  // set up the isr queue and the scheduler
  mu_spscq_init(&s_isr_queue, s_isr_queue_items, ISR_Q_CAPACITY);
  mu_sched_init(&s_sched, s_event_queue, EVENT_QUEUE_CAPACITY, &s_isr_queue);

  // initialize tasks
  led_task_init(&s_led_task, &s_led_ctx);
  button_task_init(&s_button_task, &s_button_ctx, &s_sched);
  idle_task_init(&s_idle_task, &s_sched);

  // schedule the initial call to the LED task
  mu_sched_task_now(&s_sched, &s_led_task);
}

void mu_sleep_demo_step() {
  // called repeatedly from main(): run the scheduler
  mu_sched_step(&s_sched);
}
