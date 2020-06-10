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

#include "mu_cbuf.h"
#include "mu_test_utils.h"
#include <stddef.h>

// =============================================================================
// private types and definitions

#define POOL_SIZE 4
// =============================================================================
// private declarations

// =============================================================================
// local storage

static void *pool[POOL_SIZE];
static int item0 = 0;
static int item1 = 1;
static int item2 = 2;
static int item3 = 3;

// =============================================================================
// public code

void mu_cbuf_test() {
  mu_cbuf_t cqi;
  mu_cbuf_t *cq = &cqi;
  mu_cbuf_item_t item;

  // mu_cbuf_err_t mu_cbuf_init(mu_cbuf_t *cq, mu_cbuf_obj_t *pool, unsigned int capacity);
  // pool size must be a power of two
  ASSERT(mu_cbuf_init(cq, pool, POOL_SIZE-1) == MU_CQUEUE_ERR_SIZE);
  ASSERT(mu_cbuf_init(cq, pool, POOL_SIZE) == MU_CQUEUE_ERR_NONE);
  ASSERT(mu_cbuf_capacity(cq) == POOL_SIZE-1);
  ASSERT(mu_cbuf_count(cq) == 0);
  ASSERT(mu_cbuf_is_empty(cq) == true);

  // mu_cbuf_err_t mu_cbuf_put(mu_cbuf_t *cq, mu_cbuf_obj_t obj);
  ASSERT(mu_cbuf_put(cq, (mu_cbuf_item_t)&item0) == MU_CQUEUE_ERR_NONE);
  ASSERT(mu_cbuf_count(cq) == 1);
  ASSERT(mu_cbuf_is_empty(cq) == false);
  ASSERT(mu_cbuf_put(cq, (mu_cbuf_item_t)&item1) == MU_CQUEUE_ERR_NONE);
  ASSERT(mu_cbuf_count(cq) == 2);
  ASSERT(mu_cbuf_put(cq, (mu_cbuf_item_t)&item2) == MU_CQUEUE_ERR_NONE);
  ASSERT(mu_cbuf_count(cq) == 3);
  // put into a full queue fails
  ASSERT(mu_cbuf_put(cq, (mu_cbuf_item_t)&item3) == MU_CQUEUE_ERR_FULL);
  ASSERT(mu_cbuf_count(cq) == 3);
  ASSERT(mu_cbuf_is_empty(cq) == false);

  // mu_cbuf_err_t mu_cbuf_get(mu_cbuf_t *cq, mu_cbuf_obj_t *obj);
  ASSERT(mu_cbuf_get(cq, &item) == MU_CQUEUE_ERR_NONE);
  ASSERT(mu_cbuf_count(cq) == 2);
  ASSERT(item == &item0);
  ASSERT(mu_cbuf_get(cq, &item) == MU_CQUEUE_ERR_NONE);
  ASSERT(mu_cbuf_count(cq) == 1);
  ASSERT(item == &item1);
  ASSERT(mu_cbuf_get(cq, &item) == MU_CQUEUE_ERR_NONE);
  ASSERT(mu_cbuf_count(cq) == 0);
  ASSERT(item == &item2);
  ASSERT(mu_cbuf_is_empty(cq) == true);
  // get from an empty queue fails
  ASSERT(mu_cbuf_get(cq, &item) == MU_CQUEUE_ERR_EMPTY);
  ASSERT(mu_cbuf_count(cq) == 0);
  ASSERT(item == NULL);
  ASSERT(mu_cbuf_is_empty(cq) == true);

  // mu_cbuf_err_t mu_cbuf_reset(mu_cbuf_t *cq);
  ASSERT(mu_cbuf_put(cq, (mu_cbuf_item_t)&item0) == MU_CQUEUE_ERR_NONE);
  ASSERT(mu_cbuf_count(cq) == 1);
  ASSERT(mu_cbuf_is_empty(cq) == false);
  ASSERT(mu_cbuf_reset(cq) == MU_CQUEUE_ERR_NONE);
  ASSERT(mu_cbuf_count(cq) == 0);
  ASSERT(mu_cbuf_is_empty(cq) == true);
}

// =============================================================================
// private code
