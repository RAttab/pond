/* buf.c
   Rémi Attab (remi.attab@gmail.com), 25 Apr 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "buf.h"
#include "bits.h"
#include "math.h"
#include "errors.h"

#include <string.h>

// -----------------------------------------------------------------------------
// bytes
// -----------------------------------------------------------------------------

struct pond_bin *pond_bin_alloc(size_t cap)
{
    struct pond_bin *bin = calloc(1, sizeof(*bin) + cap);
    pond_assert_alloc(bin);

    bin->cap = cap;
    return bin;
}

void pond_bin_free(struct pond_bin *bin)
{
    free(bin);
}


size_t pond_bin_read(const struct pond_bin *bin, uint8_t *dst, size_t len)
{
    len = pond_min(len, bin->len);
    memcpy(dst, bin->d, len);
    return len;
}

struct pond_it pond_bin_it(const struct pond_bin *bin)
{
    return (struct pond_it) { .it = bin->d, .end = bin->d + bin->len };
}


size_t pond_bin_write(struct pond_bin *bin, const uint8_t *src, size_t len)
{
    bin->len = pond_min(len, bin->cap);
    memcpy(bin->d, src, bin->len);
    return bin->len;
}

size_t pond_bin_append(struct pond_bin *bin, const uint8_t *src, size_t len)
{
    size_t rem = bin->cap - bin->len;
    len = pond_min(len, rem);

    memcpy(bin->d, src, bin->len);
    bin->len += len;

    return len;
}


// -----------------------------------------------------------------------------
// buf
// -----------------------------------------------------------------------------


void pond_buf_reset(struct pond_buf *buf)
{
    free(buf->d);
    *buf = (struct pond_buf) {0};
}

void pond_buf_reserve(struct pond_buf *buf, size_t cap)
{
    if (buf->cap >= cap) return;
    cap = pond_ceil_pow2(cap);

    if (!buf->cap) {
        buf->d = calloc(1, cap);
        pond_assert_alloc(buf->d);

    }
    else {
        buf->d = realloc(buf->d, cap);
        pond_assert_alloc(buf->d);
    }

    buf->cap = cap;
}


size_t pond_buf_read(const struct pond_buf *buf, uint8_t *dst, size_t len)
{
    len = pond_min(len, buf->len);
    memcpy(dst, buf->d, len);
    return len;
}

struct pond_it pond_buf_it(const struct pond_buf *buf)
{
    return (struct pond_it) { .it = buf->d, .end = buf->d + buf->len };
}


void pond_buf_write(struct pond_buf *buf, const uint8_t *src, size_t len)
{
    if (pond_unlikely(len > buf->cap)) pond_buf_reserve(buf, len);
    pond_assert(len <= buf->cap, "unexpected len: %zu > %zu", len, buf->cap);

    memcpy(buf->d, src, len);
    buf->len = len;
}

void pond_buf_append(struct pond_buf *buf, const uint8_t *src, size_t len)
{
    size_t new_len = buf->len + len;

    if (pond_unlikely(new_len > buf->cap)) pond_buf_reserve(buf, new_len);
    pond_assert(new_len <= buf->cap, "unexpected len: %zu > %zu", new_len, buf->cap);

    memcpy(buf->d + buf->len, src, len);
    buf->len = new_len;
}



// -----------------------------------------------------------------------------
// it
// -----------------------------------------------------------------------------

size_t pond_it_read(struct pond_it *it, uint8_t *dst, size_t len)
{
    pond_assert(it->it <= it->end, "inverted ptr: %p > %p", it->it, it->end);

    len = pond_min(len, (size_t) (it->end - it->it));
    memcpy(dst, it->it, len);
    it->it += len;

    return len;
}
