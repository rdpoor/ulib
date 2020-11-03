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

#include "mu_config.h"    // must come first

#include "mu_sched.h"
#include "mu_list.h"
#include "mu_spscq.h"
#include "mu_thunk.h"
#include "mu_time.h"
#include <stdbool.h>
#include <stddef.h>
#include <string.h> // memmove

// =============================================================================
// local types and definitions

typedef struct {
  mu_list_t task_list;     // time ordered list of tasks (soonest first)
  mu_spscq_t isr_queue;    // interrupt-safe queue of tasks to be added
  mu_clock_fn clock_fn;    // function to call to get the current time
  mu_thunk_t *idle_task;    // the idle thunk
  mu_thunk_t *current_task; // the thunk currently being processed
} mu_sched_t;

// =============================================================================
// local (forward) declarations

static void *default_idle_fn(void *self, void *arg);

static mu_sched_err_t sched_task_at(mu_thunk_t *thunk, mu_time_t time);

static void *sched_task_at_aux(mu_list_t *ref, void *arg);

// =============================================================================
// local storage

// Singleton instance of the scheduler
static mu_sched_t s_sched;

static mu_thunk_t s_default_idle_task;

// =============================================================================
// public code

void mu_sched_init(mu_spscq_item_t *isr_queue_store, uint16_t isr_queue_capacity) {
  mu_spscq_init(&s_sched.isr_queue, isr_queue_store, isr_queue_capacity);
  s_sched.clock_fn = mu_time_now;
  s_sched.idle_task = &s_default_idle_task;
  mu_thunk_init(&s_default_idle_task, default_idle_fn, NULL, "Idle");

  mu_sched_reset();
}

void mu_sched_reset(void) {
  mu_spscq_reset(&s_sched.isr_queue);
  s_sched.current_task = NULL;
  s_sched.task_list.next = NULL;
}

mu_sched_err_t mu_sched_step(void) {
  mu_thunk_t *thunk;
  mu_time_t now = mu_sched_get_current_time();

  // first, transfer any tasks in the isr queue into the main queue
  while (mu_spscq_get(&s_sched.isr_queue, (void **)&thunk) !=
         MU_CQUEUE_ERR_EMPTY) {
    mu_sched_err_t err = mu_sched_task_at(thunk, now);
    if (err != MU_SCHED_ERR_NONE) {
      return err;
    }
  }

  // process one thunk in the main queue
  thunk = mu_sched_get_next_task(); // peek at next thunk.
  if (thunk != NULL) {
    if (!mu_time_follows(mu_thunk_get_time(thunk), now)) {
      // time to run the thunk: pop from queue, make current, run it...
      s_sched.current_task =
          MU_LIST_CONTAINER(mu_list_pop(&s_sched.task_list), mu_thunk_t, link);

      mu_thunk_call(s_sched.current_task, NULL);
      // set current thunk to null to signify "not running thunk"
      s_sched.current_task = NULL;
      return MU_SCHED_ERR_NONE;
    }
  }

  // arrive here if there was nothing to run
  mu_thunk_call(s_sched.idle_task, NULL);
  return MU_SCHED_ERR_NONE;
}

mu_list_t mu_sched_task_list(void) { return s_sched.task_list; }

mu_spscq_t *mu_sched_isr_queue(void) { return &s_sched.isr_queue; }

mu_thunk_t *mu_sched_get_idle_task(void) { return s_sched.idle_task; }

mu_thunk_t *mu_sched_get_default_idle_task(void) { return &s_default_idle_task; }

void mu_sched_set_idle_task(mu_thunk_t *thunk) { s_sched.idle_task = thunk; }

mu_clock_fn mu_sched_get_clock_source(void) { return s_sched.clock_fn; }

void mu_sched_set_clock_source(mu_clock_fn clock_fn) {
  s_sched.clock_fn = clock_fn;
}

mu_time_t mu_sched_get_current_time(void) { return s_sched.clock_fn(); }

int mu_sched_task_count(void) { return mu_list_length(&s_sched.task_list); }

bool mu_sched_is_empty(void) { return mu_list_is_empty(&s_sched.task_list); }

mu_thunk_t *mu_sched_get_current_task(void) { return s_sched.current_task; }

mu_thunk_t *mu_sched_get_next_task(void) {
  if (s_sched.task_list.next == NULL) {
    return NULL;
  }
  return MU_LIST_CONTAINER(s_sched.task_list.next, mu_thunk_t, link);
}

mu_thunk_t *mu_sched_remove_task(mu_thunk_t *thunk) {
  mu_list_t *link = mu_list_delete(&s_sched.task_list, MU_LIST_REF(thunk, link));
  if (link == NULL) {
    return NULL;
  }
  return MU_LIST_CONTAINER(link, mu_thunk_t, link);
}

mu_sched_err_t mu_sched_task_now(mu_thunk_t *thunk) {
  mu_sched_remove_task(thunk);
  return sched_task_at(thunk, mu_sched_get_current_time());
}

mu_sched_err_t mu_sched_task_at(mu_thunk_t *thunk, mu_time_t at) {
  mu_sched_remove_task(thunk);
  return sched_task_at(thunk, at);
}

mu_sched_err_t mu_sched_task_in(mu_thunk_t *thunk, mu_time_dt in) {
  mu_sched_remove_task(thunk);
  return sched_task_at(thunk, mu_time_offset(mu_sched_get_current_time(), in));
}

mu_sched_err_t mu_sched_reschedule_in(mu_time_dt in) {
  mu_thunk_t *thunk = mu_sched_get_current_task();
  if (!thunk) {
    return MU_SCHED_ERR_NOT_FOUND;
  }
  return sched_task_at(thunk, mu_time_offset(mu_thunk_get_time(thunk), in));
}

mu_sched_err_t mu_sched_reschedule_now(void) {
  mu_thunk_t *thunk = mu_sched_get_current_task();
  if (!thunk) {
    return MU_SCHED_ERR_NOT_FOUND;
  }
  return mu_sched_task_now(thunk);
}

mu_sched_err_t mu_sched_task_from_isr(mu_thunk_t *thunk) {
  if (mu_spscq_put(&s_sched.isr_queue, thunk) == MU_CQUEUE_ERR_FULL) {
    return MU_SCHED_ERR_FULL;
  } else {
    return MU_SCHED_ERR_NONE;
  }
}

mu_sched_task_status_t mu_sched_get_task_status(mu_thunk_t *thunk) {
  if (mu_sched_get_current_task() == thunk) {
    // thunk is the current thunk
    return MU_SCHED_TASK_STATUS_ACTIVE;
  }
  if (!mu_list_contains(&s_sched.task_list, MU_LIST_REF(thunk, link))) {
    // thunk is not in the schedule at all
    return MU_SCHED_TASK_STATUS_IDLE;
  }

  mu_time_t now = mu_sched_get_current_time();
  if (!mu_time_follows(mu_thunk_get_time(thunk), now)) {
    // thunk's time has arrived, but it's not yet running
    return MU_SCHED_TASK_STATUS_RUNNABLE;

  } else {
    // thunk is scheduled for some pont in the future
    return MU_SCHED_TASK_STATUS_SCHEDULED;
  }
}

// =============================================================================
// local (static) code

static void *default_idle_fn(void *self, void *arg) {
  // the default idle thunk doesn't do much...
  return self;
}

static mu_sched_err_t sched_task_at(mu_thunk_t *thunk, mu_time_t time) {
  mu_thunk_set_time(thunk, time);
  mu_list_traverse(&s_sched.task_list, sched_task_at_aux, (void *)thunk);
  return MU_SCHED_ERR_NONE;
}

static void *sched_task_at_aux(mu_list_t *ref, void *arg) {
  mu_thunk_t *thunk = (mu_thunk_t *)arg;
  mu_time_t time1 = mu_thunk_get_time(thunk);

  // If at end of list, insert thunk here.
  if (ref->next == NULL) {
    return mu_list_push(ref, MU_LIST_REF(thunk, link));
  }
  // Check relative times of new thunk and incumbent thunk.
  mu_thunk_t *thunk2 = MU_LIST_CONTAINER(ref->next, mu_thunk_t, link);
  mu_time_t time2 = mu_thunk_get_time(thunk2);
  if (mu_time_precedes(time1, time2)) {
    // thunk happens sooner than task2: insert on list
    return mu_list_push(ref, MU_LIST_REF(thunk, link));
  }
  // keep searching ...
  return NULL;
}
