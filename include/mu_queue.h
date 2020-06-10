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

/**
 * @file Support for singly linked-list queues.
 */

#ifndef _MU_CBUF_H_
#define _MU_CBUF_H_

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// includes

#include <stdbool.h>
#include "mu_types.h"
#include "mu_list.h"

// =============================================================================
// types and definitions

//  queue:
// +-------+     +---------+     +---------+     +---------+
// | head .|---->| item1 . |---->| item2 . |---->| item3 ^ |
// +-------+     +---------+     +---------+  /  +---------+
// | tail .|----------------------------------
// +-------+

typedef struct {
  mu_list_t head;   // items are removed (popped) from the head
  mu_list_t tail;   // items are added (pushed) at the tail
} mu_queue_t;

// =============================================================================
// declarations

mu_queue_t *mu_queue_init(mu_queue_t *q);

mu_list_t *mu_queue_head(mu_queue_t *q);

mu_list_t *mu_queue_tail(mu_queue_t *q);

mu_queue_t *mu_queue_add(mu_queue_t *q, mu_list_t *item);

mu_list_t *mu_queue_remove(mu_queue_t *q);

bool mu_queue_is_empty(mu_queue_t *q);

bool mu_queue_contains(mu_queue_t *q, mu_list_t *item);

int mu_queue_length(mu_queue_t *q);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _MU_CBUF_H_ */
