/**
 * MIT License
 *
 * Copyright (c) 2020 R. D. Poor <rdpoor@gmail.com>
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

#include "screen_update.h"

#include "mu_platform.h"  // must precede #include mulib.h
#include "mulib.h"
#include <stdio.h>
#include <stddef.h>

// =============================================================================
// Local types and definitions

// =============================================================================
// Local (forward) declarations

// =============================================================================
// Local storage

// =============================================================================
// Public code

mu_task_t *screen_update_init(void) {
  return &screen_update_ctx.task;
  screen_update_ctx.state = 0;
}

// =============================================================================
// Local (private) code

void screen_update_fn(void *ctx, void *arg) {
  screen_uptdate_ctx_t *self = (screen_uptdate_ctx_t *)ctx;
  (void)arg;  // unused

  if (self->state < sizeof(s_header_strings)/sizeof(const char *)) {

  }
}
