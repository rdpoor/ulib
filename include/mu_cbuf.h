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

#ifndef _MU_CQUEUE_H_
#define _MU_CQUEUE_H_

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// includes

#include <stdint.h>
#include <stdbool.h>

// =============================================================================
// types and definitions

typedef enum {
  MU_CQUEUE_ERR_NONE,
  MU_CQUEUE_ERR_EMPTY,
  MU_CQUEUE_ERR_FULL,
  MU_CQUEUE_ERR_SIZE,
} mu_cbuf_err_t;

// mu_cbuf manages pointer-sized objects
typedef void * mu_cbuf_item_t;

typedef struct {
  uint16_t mask;
  volatile uint16_t head;
  volatile uint16_t tail;
  mu_cbuf_item_t *store;
} mu_cbuf_t;

// =============================================================================
// declarations

/**
 * @brief initialize a cqueue with a backing store.  capacity must be a power
 * of two.
 */
mu_cbuf_err_t mu_cbuf_init(mu_cbuf_t *q, mu_cbuf_item_t *store, uint16_t capacity);

/**
 * @brief reset the cqueue to empty.
 */
mu_cbuf_err_t mu_cbuf_reset(mu_cbuf_t *q);

/**
 * @brief return the maximum number of items that can be stored in the cqueue.
 */
uint16_t mu_cbuf_capacity(mu_cbuf_t *q);

/**
 * @brief Return the number of items in the cqueue.
 */
uint16_t mu_cbuf_count(mu_cbuf_t *q);

/**
 * @brief Return true if there are no items in the queue.
 *
 * Note: this is a tiny bit faster than mu_cbuf_count() == 0
 */
bool mu_cbuf_is_empty(mu_cbuf_t *q);

/**
 * @brief Return true if the queue is full.
 */
bool mu_cbuf_is_full(mu_cbuf_t *q);

/**
 * @brief Insert an item at the tail of the queue.
 */
mu_cbuf_err_t mu_cbuf_put(mu_cbuf_t *q, mu_cbuf_item_t item);

/**
 * @brief Remove an item from the head of the queue.
 */
mu_cbuf_err_t mu_cbuf_get(mu_cbuf_t *q, mu_cbuf_item_t *item);

#ifdef __cplusplus
}
#endif

#endif // #ifndef _MU_CQUEUE_H_
