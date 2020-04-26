/* bits.h
   Rémi Attab (remi.attab@gmail.com), 03 May 2014
   FreeBSD-style copyright and disclaimer apply

   Bit ops header.
*/

#pragma once

// -----------------------------------------------------------------------------
// builtin
// -----------------------------------------------------------------------------

// The ctz and clz builtins are undefined for 0 because they're undefined on x86
inline size_t pond_clz(uint64_t x) { return x ? __builtin_clzll(x) : 64; }
inline size_t pond_ctz(uint64_t x) { return x ? __builtin_ctzll(x) : 64; }
inline size_t pond_pop(uint64_t x) { return __builtin_popcountll(x); }


// -----------------------------------------------------------------------------
// custom
// -----------------------------------------------------------------------------

inline uint64_t pond_leading_bit(uint64_t x)
{
    if (!x) return 0;

    // clang-tidy: thinks that the << op is undefined which it would be if we
    // didn't test x against zero. Unfortunately, it looks like the the clz
    // builtin confuses it.
    return x & (1ULL << (63 - pond_clz(x))); // NOLINT
}

inline int pond_is_pow2(uint64_t x) { return pond_pop(x) == 1; }
inline uint64_t pond_ceil_pow2(uint64_t x)
{
    return x > 1 ? pond_leading_bit(x - 1) << 1 : 1;
}

inline size_t pond_ceil_div(size_t n, size_t d)
{
    return n ? ((n - 1) / d) + 1 : 0;
}

inline size_t pond_bit_align(size_t n, size_t align)
{
    return pond_ceil_div(n, align) * align;
}


// -----------------------------------------------------------------------------
// bitfields
// -----------------------------------------------------------------------------

// for (size_t i = bitfield_next(bf, 0); i < 64; i = bitfield_next(bf, i + 1))
inline size_t pond_bitfield_next(uint64_t bf, size_t bit)
{
    return pond_ctz(bf & ~((1UL << bit) -1));
}

// for (size_t i = bitfields_next(bf, 0, n); i < n; i = bitfields_next(bf, i + 1, n))
inline size_t pond_bitfields_next(const uint64_t *bf, size_t bit, size_t n)
{
    for (; bit < n; bit += 64) {
        size_t low = bit % 64;
        size_t high = bit / 64;

        if (bf[high]) {
            size_t i = pond_bitfield_next(bf[high], low);
            if (i < 64) return i + (high * 64);
        }

        bit &= ~(64 - 1);
    }

    return n;
}
