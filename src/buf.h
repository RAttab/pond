/* buf.h
   Rémi Attab (remi.attab@gmail.com), 25 Apr 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "compiler.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>


// -----------------------------------------------------------------------------
// bytes
// -----------------------------------------------------------------------------

struct pond_bin
{
    size_t len, cap;
    uint8_t d[];
};

struct pond_bin *pond_bin_alloc(size_t cap) pond_malloc;
void pond_bin_free(struct pond_bin *);

size_t pond_bin_read(const struct pond_bin *, uint8_t *dst, size_t len);
struct pond_it pond_bin_it(const struct pond_bin *);

size_t pond_bin_write(struct pond_bin *, const uint8_t *src, size_t len);
size_t pond_bin_append(struct pond_bin *, const uint8_t *src, size_t len);



// -----------------------------------------------------------------------------
// buf
// -----------------------------------------------------------------------------

struct pond_buf
{
    size_t len, cap;
    uint8_t *d;
};

void pond_buf_reset(struct pond_buf *);
void pond_buf_reserve(struct pond_buf *, size_t cap);

size_t pond_buf_read(const struct pond_buf *, uint8_t *dst, size_t len);
struct pond_it pond_buf_it(const struct pond_buf *);

void pond_buf_write(struct pond_buf *, const uint8_t *src, size_t len);
void pond_buf_append(struct pond_buf *, const uint8_t *src, size_t len);


// -----------------------------------------------------------------------------
// it
// -----------------------------------------------------------------------------

struct pond_it
{
    const uint8_t *it;
    const uint8_t *end;
};

inline bool pond_it_end(struct pond_it it) { return it.it == it.end; }
size_t pond_it_read(struct pond_it *, uint8_t *dst, size_t len);
