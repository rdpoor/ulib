/**
 * MIT License
 *
 * Copyright (c) 2019 R. Dunbar Poor <rdpoor@gmail.com>
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

#include "mu_sched.h"
#include "mu_task.h"
#include "mu_pstore.h"
#include "mu_spscq.h"
#include "mu_time.h"
#include "mu_test_utils.h"

// =============================================================================
// private types and definitions

#define TASK_QUEUE_SIZE 4
#define IRQ_QUEUE_SIZE 2

// =============================================================================
// private declarations

static void reset(void);

// get_now() / set_now() allows us to control the time manually for testing.
static mu_time_t get_now(void);
static void set_now(mu_time_t now);

static void *taska_fn(void *self, void *arg);
static void *taskb_fn(void *self, void *arg);
static void *taski_fn(void *self, void *arg);  // idle task

// =============================================================================
// local storage

static mu_sched_t s_sched;

static mu_time_t s_now;

static mu_pstore_item_t s_task_queue_items[TASK_QUEUE_SIZE];
static mu_pstore_t s_task_queue;

static mu_spscq_item_t s_isr_queue_items[IRQ_QUEUE_SIZE];
static mu_spscq_t s_isr_queue;

static mu_task_t s_task1;
static mu_task_t s_task2;
static mu_task_t s_task3;
static mu_task_t s_task4;
static mu_task_t s_task5;
static mu_task_t s_taski;

// =============================================================================
// public code

// mu_sched_test depends upon profiling functions.
#if (!MU_TASK_PROFILING)
#error mu_sched_test requires MU_TASK_PROFILING to be defined
#endif

void mu_sched_test() {
  mu_sched_t *s = &s_sched;
  mu_time_t time;

  reset();

  // mu_sched_t *mu_sched_init(mu_sched_t *sched, mu_pstore_t *task_queue, mu_spscq *isr_queue);
  ASSERT(mu_sched_init(s, &s_task_queue, &s_isr_queue) == s);

  // mu_sched_t *mu_sched_reset(mu_sched_t *sched);

  // mu_sched_err_t mu_sched_step(void);

  // mu_pstore_t *mu_sched_task_queue(mu_sched_t *sched);
  ASSERT(mu_sched_task_queue(s) == &s_task_queue);

  // mu_spscq_t *mu_sched_isr_queue(mu_sched_t *sched);
  ASSERT(mu_sched_isr_queue(s) == &s_isr_queue);

  // mu_task_t *mu_sched_get_idle_task(mu_sched_t *sched);
  // mu_task_t *mu_sched_get_default_idle_task(mu_sched_t *sched);
  ASSERT(mu_sched_get_idle_task(s) == mu_sched_get_default_idle_task(s));

  // mu_sched_t *mu_sched_set_idle_task(mu_sched_t *sched, mu_task_t *task);
  ASSERT(mu_sched_set_idle_task(s, &s_taski) == s);
  ASSERT(mu_sched_get_idle_task(s) == &s_taski);

  // mu_clock_fn mu_sched_get_clock_source(mu_sched_t *sched);
  // mu_sched_t *mu_sched_set_clock_source(mu_sched_t *sched, mu_clock_fn clock_fn);
  ASSERT(mu_sched_set_clock_source(s, get_now) == s);
  ASSERT(mu_sched_get_clock_source(s) == get_now);

  // test our clock...
  // mu_time_t mu_sched_get_current_time(mu_sched_t *sched);
  set_now(100);
  ASSERT(mu_sched_get_current_time(s) == 100);

  // testing initial conditions
  // bool mu_sched_is_empty(mu_sched_t *sched);
  // size_t mu_sched_task_count(mu_sched_t *sched);
  // bool mu_sched_task_is_scheduled(mu_sched_t *sched, mu_task_t *task);
  ASSERT(mu_sched_is_empty(s) == true);
  ASSERT(mu_sched_get_current_task(s) == NULL);
  ASSERT(mu_sched_task_count(s) == 0);
  ASSERT(mu_sched_task_is_scheduled(s, &s_task1) == false);
  ASSERT(mu_sched_get_next_time(s, &time) == MU_SCHED_ERR_NOT_FOUND);

  // queue tasks, validate ordering
  // mu_sched_err_t mu_sched_task_at(mu_sched_t *sched, mu_task_t *task, mu_time_t at);
  ASSERT(mu_sched_task_at(s, &s_task1, 101) == MU_SCHED_ERR_NONE);
  ASSERT(mu_sched_task_at(s, &s_task3, 103) == MU_SCHED_ERR_NONE);
  ASSERT(mu_sched_task_at(s, &s_task2, 102) == MU_SCHED_ERR_NONE);
  ASSERT(mu_sched_task_at(s, &s_task5, 105) == MU_SCHED_ERR_NONE);
  ASSERT(mu_sched_task_at(s, &s_task4, 104) == MU_SCHED_ERR_FULL);
  // "furthest in the future" task must be at index = 0
  ASSERT(s_task_queue_items[0] == &s_task5);
  ASSERT(((mu_task_t *)s_task_queue_items[0])->time == 105);
  ASSERT(s_task_queue_items[1] == &s_task3);
  ASSERT(((mu_task_t *)s_task_queue_items[1])->time == 103);
  ASSERT(s_task_queue_items[2] == &s_task2);
  ASSERT(((mu_task_t *)s_task_queue_items[2])->time == 102);
  ASSERT(s_task_queue_items[3] == &s_task1);
  ASSERT(((mu_task_t *)s_task_queue_items[3])->time == 101);

  ASSERT(mu_sched_is_empty(s) == false);
  ASSERT(mu_sched_get_current_task(s) == NULL);
  ASSERT(mu_sched_task_count(s) == 4);
  ASSERT(mu_sched_task_is_scheduled(s, &s_task1) == true);
  ASSERT(mu_sched_task_is_scheduled(s, &s_task4) == false);
  ASSERT(mu_sched_get_next_time(s, &time) == MU_SCHED_ERR_NONE);
  ASSERT(time == 101);

  reset();

  // mu_sched_step(mu_sched_t *sched)
  set_now(100);
  ASSERT(mu_sched_step(s) == MU_SCHED_ERR_NONE);
  ASSERT(mu_task_call_count(&s_task1) == 0);
  ASSERT(mu_sched_task_at(s, &s_task1, 101) == MU_SCHED_ERR_NONE);
  // time hasn't arrived yet...
  ASSERT(mu_sched_step(s) == MU_SCHED_ERR_NONE);
  ASSERT(mu_task_call_count(&s_task1) == 0);
  // time has arrived for task1
  set_now(101);
  ASSERT(mu_sched_step(s) == MU_SCHED_ERR_NONE);
  ASSERT(mu_task_call_count(&s_task1) == 1);
  // task queue should be empty
  ASSERT(mu_sched_task_count(s) == 0);

  reset();
  set_now(100);
  ASSERT(mu_sched_task_at(s, &s_task1, 101) == MU_SCHED_ERR_NONE);
  ASSERT(mu_sched_task_at(s, &s_task2, 102) == MU_SCHED_ERR_NONE);
  ASSERT(mu_sched_step(s) == MU_SCHED_ERR_NONE);
  ASSERT(mu_task_call_count(&s_task1) == 0);
  ASSERT(mu_task_call_count(&s_task2) == 0);

  // time jumps -- both task1 and task2 are runnable, but task1 runs first
  set_now(102);
  ASSERT(mu_sched_step(s) == MU_SCHED_ERR_NONE);
  ASSERT(mu_task_call_count(&s_task1) == 1);
  ASSERT(mu_task_call_count(&s_task2) == 0);
  ASSERT(mu_sched_step(s) == MU_SCHED_ERR_NONE);
  ASSERT(mu_task_call_count(&s_task1) == 1);
  ASSERT(mu_task_call_count(&s_task2) == 1);
  ASSERT(mu_sched_task_count(s) == 0);

  // mu_sched_err_t mu_sched_remove_task(mu_sched_t *sched, mu_task_t *task);
  reset();
  set_now(100);   // shouldn't mwtter
  ASSERT(mu_sched_remove_task(s, &s_task1) == MU_SCHED_ERR_NOT_FOUND);
  ASSERT(mu_sched_task_at(s, &s_task1, 101) == MU_SCHED_ERR_NONE);
  ASSERT(mu_sched_task_is_scheduled(s, &s_task1) == true);
  ASSERT(mu_sched_remove_task(s, &s_task1) == MU_SCHED_ERR_NONE);
  ASSERT(mu_sched_task_count(s) == 0);
  ASSERT(mu_sched_task_is_scheduled(s, &s_task1) == false);

  // reschedule a task, assure proper ordering
  reset();
  set_now(100);  //shouldn't matter...
  ASSERT(mu_sched_task_at(s, &s_task1, 101) == MU_SCHED_ERR_NONE);
  ASSERT(mu_sched_task_at(s, &s_task2, 102) == MU_SCHED_ERR_NONE);
  // verify that task1 precedes task2
  ASSERT(s_task_queue_items[0] == &s_task2);
  ASSERT(((mu_task_t *)s_task_queue_items[0])->time == 102);
  ASSERT(s_task_queue_items[1] == &s_task1);
  ASSERT(((mu_task_t *)s_task_queue_items[1])->time == 101);
  // rescheduling task1: task1 now follows task2.  verify queue
  ASSERT(mu_sched_task_at(s, &s_task1, 103) == MU_SCHED_ERR_NONE);
  ASSERT(s_task_queue_items[0] == &s_task1);
  ASSERT(((mu_task_t *)s_task_queue_items[0])->time == 103);
  ASSERT(s_task_queue_items[1] == &s_task2);
  ASSERT(((mu_task_t *)s_task_queue_items[1])->time == 102);
  ASSERT(mu_sched_get_next_time(s, &time) == MU_SCHED_ERR_NONE);
  ASSERT(time == 102);

  // mu_sched_err_t mu_sched_task_now(mu_sched_t *sched, mu_task_t *task);
  reset();
  set_now(100);
  ASSERT(mu_sched_task_at(s, &s_task1, 101) == MU_SCHED_ERR_NONE);
  ASSERT(mu_sched_task_now(s, &s_task2) == MU_SCHED_ERR_NONE);
  ASSERT(mu_task_get_time(&s_task1) == 101);
  ASSERT(mu_task_get_time(&s_task2) == 100);

  // mu_sched_err_t mu_sched_task(mu_sched_t *sched, mu_task_t *task);

  // mu_sched_err_t mu_sched_task_in(mu_sched_t *sched, mu_task_t *task, mu_time_t in);
  reset();
  set_now(100);
  ASSERT(mu_sched_task_in(s, &s_task1, 10) == MU_SCHED_ERR_NONE);
  ASSERT(mu_task_get_time(&s_task1) == 110);

  // mu_sched_err_t mu_sched_task_again(mu_sched_t *sched, mu_task_t *task, mu_time_t dt);
  reset();
  mu_task_set_time(&s_task1, 100);
  ASSERT(mu_sched_task_again(s, &s_task1, 5) == MU_SCHED_ERR_NONE);
  ASSERT(mu_task_get_time(&s_task1) == 105);
  ASSERT(s_task_queue_items[0] == &s_task1);
  ASSERT(((mu_task_t *)s_task_queue_items[0])->time == 105);

  // mu_sched_err_t mu_sched_task_from_isr(mu_sched_t *sched, mu_task_t *task);
  reset();
  set_now(100);
  ASSERT(mu_spscq_count(s->isr_queue) == 0);
  ASSERT(mu_sched_task_from_isr(s, &s_taski) == MU_SCHED_ERR_NONE);
  ASSERT(mu_spscq_count(s->isr_queue) == 1);
  ASSERT(mu_sched_step(s) == MU_SCHED_ERR_NONE);
  // verify that calling step:
  // - set the task's time to the scheduler's current time
  // - called the task
  // - removed it from the isr queue
  ASSERT(mu_task_get_time(&s_taski) == 100);
  ASSERT(mu_task_call_count(&s_taski) == 1);
  ASSERT(mu_spscq_count(s->isr_queue) == 0);
  ASSERT(mu_sched_task_count(s) == 0);
}

// =============================================================================
// private code

static void reset(void) {
  set_now(0);
  mu_pstore_init(&s_task_queue, s_task_queue_items, TASK_QUEUE_SIZE);
  mu_spscq_init(&s_isr_queue, s_isr_queue_items, IRQ_QUEUE_SIZE);
  mu_task_init(&s_task1, taska_fn, &s_task1, "Task1");  // calls taska_fn
  mu_task_init(&s_task2, taska_fn, &s_task2, "Task2");  // ...
  mu_task_init(&s_task3, taska_fn, &s_task3, "Task3");
  mu_task_init(&s_task4, taska_fn, &s_task4, "Task4");
  mu_task_init(&s_task5, taskb_fn, &s_task5, "Task5");  // calls taskb_fn
  mu_task_init(&s_taski, taski_fn, &s_taski, "TaskI");  // calls taski_fn
}

static mu_time_t get_now(void) {
  return s_now;
}

static void set_now(mu_time_t now) {
  s_now = now;
}

// self is set to the task itself, arg is set to the scheduler
static void *taska_fn(void *self, void *arg) {
  ASSERT(arg == &s_sched);
  mu_sched_t *s = (mu_sched_t *)arg;
  // mu_task_t *mu_sched_get_current_task(mu_sched_t *sched);
  ASSERT(mu_sched_get_current_task(s) == self);
  return NULL;
}

static void *taskb_fn(void *self, void *arg) {
  return NULL;
}

static void *taski_fn(void *self, void *arg) {
  return NULL;
}