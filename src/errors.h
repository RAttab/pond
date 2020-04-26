/* errors.h
   Rémi Attab (remi.attab@gmail.com), 25 Feb 2016
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "compiler.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>

enum {
    pond_err_msg_cap = 1024,
    pond_err_backtrace_cap = 256,
};


struct pond_error
{
    bool warning;

    const char *file;
    int line;

    int errno_; // errno can be a macro hence the underscore.
    char msg[pond_err_msg_cap];

    void *backtrace[pond_err_backtrace_cap];
    int backtrace_len;
};

extern __thread struct pond_error pond_errno;

void pond_perror(struct pond_error *err);
size_t pond_strerror(struct pond_error *err, char *dest, size_t len);


// -----------------------------------------------------------------------------
// dump
// -----------------------------------------------------------------------------

void pond_syslog();

// -----------------------------------------------------------------------------
// abort
// -----------------------------------------------------------------------------

void pond_abort() pond_noreturn;
void pond_error_exit() pond_noreturn;


// -----------------------------------------------------------------------------
// fail
// -----------------------------------------------------------------------------

void pond_dbg_abort_on_fail();

void pond_vfail(const char *file, int line, const char *fmt, ...)
    pond_printf(3, 4);

void pond_vfail_errno(const char *file, int line, const char *fmt, ...)
    pond_printf(3, 4);

#define pond_fail(...)                                \
    pond_vfail(__FILE__, __LINE__, __VA_ARGS__)

#define pond_fail_errno(...)                          \
    pond_vfail_errno(__FILE__, __LINE__, __VA_ARGS__)

// useful for pthread APIs which return the errno.
#define pond_fail_ierrno(err, ...)                            \
    do {                                                        \
        errno = err;                                            \
        pond_vfail_errno(__FILE__, __LINE__, __VA_ARGS__);    \
    } while (false)


// -----------------------------------------------------------------------------
// warn
// -----------------------------------------------------------------------------

void pond_dbg_abort_on_warn();

void pond_vwarn(const char *file, int line, const char *fmt, ...)
    pond_printf(3, 4);

void pond_vwarn_errno(const char *file, int line, const char *fmt, ...)
    pond_printf(3, 4);

void pond_vwarn_va(const char *file, int line, const char *fmt, va_list args);

#define pond_warn(...)                                \
    pond_vwarn(__FILE__, __LINE__, __VA_ARGS__)

#define pond_warn_errno(...)                          \
    pond_vwarn_errno(__FILE__, __LINE__, __VA_ARGS__)

#define pond_warn_va(fmt, args)                       \
    pond_vwarn_va(__FILE__, __LINE__, fmt, args);

// useful for pthread APIs which return the errno.
#define pond_warn_ierrno(err, ...)                            \
    do {                                                        \
        errno = err;                                            \
        pond_vwarn_errno(__FILE__, __LINE__, __VA_ARGS__);    \
    } while (false)


// -----------------------------------------------------------------------------
// assert
// -----------------------------------------------------------------------------

#define pond_assert(p, ...)                   \
    do {                                        \
        if (pond_likely(p)) break;            \
        pond_fail(__VA_ARGS__);               \
        pond_abort();                         \
    } while (0)

#define pond_todo(msg)                        \
    do {                                        \
        pond_fail("TODO: " msg);              \
        pond_abort();                         \
    } while (0)

#define pond_assert_alloc(p)                  \
    pond_assert((p) != NULL, "out-of-memory");
