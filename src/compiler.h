/* compiler.h
   Rémi Attab (remi.attab@gmail.com), 03 May 2014
   FreeBSD-style copyright and disclaimer apply

   Compiler related utilities and attributes.
*/

#pragma once

// -----------------------------------------------------------------------------
// attributes
// -----------------------------------------------------------------------------

#define pond_unused       __attribute__((unused))
#define pond_noreturn     __attribute__((noreturn))
#define pond_align(x)     __attribute__((aligned(x)))
#define pond_packed       __attribute__((__packed__))
#define pond_pure         __attribute__((pure))
#define pond_printf(x,y)  __attribute__((format(printf, x, y)))
#define pond_malloc       __attribute__((malloc))
#define pond_noinline     __attribute__((noinline))
#define pond_likely(x)    __builtin_expect(x, 1)
#define pond_unlikely(x)  __builtin_expect(x, 0)


// -----------------------------------------------------------------------------
// builtin
// -----------------------------------------------------------------------------

#define pond_unreachable() __builtin_unreachable()


// -----------------------------------------------------------------------------
// asm
// -----------------------------------------------------------------------------

#define pond_asm __asm__

#define pond_no_opt()         pond_asm volatile ("")
#define pond_no_opt_val(x)    pond_asm volatile ("" : "+r" (x))
#define pond_no_opt_clobber() pond_asm volatile ("" : : : "memory")


// -----------------------------------------------------------------------------
// utils
// -----------------------------------------------------------------------------

#define pond_static_assert(p) _Static_assert(p)
#define pond_sizeof_member(type, member) sizeof(((type *) 0)->member)
