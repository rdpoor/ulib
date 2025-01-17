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

#include "mu_test_utils.h"
#include "core/mu_task.h"
#include <string.h>

// =============================================================================
// private types and definitions

// =============================================================================
// private declarations

static int s_task1_count;
static int s_task2_count;

static void reset(void) {
  s_task1_count = 0;
}
static void task_fn1(void *self, void *arg) {
  s_task1_count += 1;
  ASSERT(self != NULL);
  ASSERT(arg == self);
}

static void task_fn2(void *self, void *arg) {
  s_task2_count += 1;
  ASSERT(self == NULL);
  ASSERT(arg == NULL);
}

// =============================================================================
// local storage

// =============================================================================
// public code

void mu_task_test() {
  mu_task_t t1;
  mu_task_t t2;

  reset();

  ASSERT(mu_task_init(&t1, task_fn1, &t1, "Task1") == &t1);
  ASSERT(mu_task_init(&t2, task_fn2, NULL, "Task2") == &t2);
#if (MU_TASK_PROFILING)
  ASSERT(strcmp(mu_task_name(&t1), "Task1") == 0);
  ASSERT(strcmp(mu_task_name(&t1), "Task2") == 0);
#endif

  mu_task_call(&t1, &t1);
  ASSERT(s_task1_count == 1);
  ASSERT(s_task2_count == 0);

  mu_task_call(&t2, NULL);
  ASSERT(s_task1_count == 1);
  ASSERT(s_task2_count == 1);
}

// =============================================================================
// private code
