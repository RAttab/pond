/* errors.c
   Rémi Attab (remi.attab@gmail.com), 01 Mar 2016
   FreeBSD-style copyright and disclaimer apply
*/

#include "errors.h"
#include "process.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <execinfo.h>
#include <syslog.h>


// -----------------------------------------------------------------------------
// error
// -----------------------------------------------------------------------------

__thread struct pond_error pond_errno = { 0 };

void pond_abort()
{
    pond_perror(&pond_errno);
    abort();
}

void pond_error_exit()
{
    pond_perror(&pond_errno);
    exit(1);
}


size_t pond_strerror(struct pond_error *err, char *dest, size_t len)
{
    size_t i = 0;

    if (!err->errno_) {
        i = snprintf(dest, len, "<%d:%lu> %s:%d: %s\n",
                getpid(), pond_tid(), err->file, err->line, err->msg);
    }
    else {
        i = snprintf(dest, len, "<%d:%lu> %s:%d: %s - %s(%d)\n",
                getpid(), pond_tid(), err->file, err->line, err->msg,
                strerror(err->errno_), err->errno_);
    }

    if (err->backtrace_len > 0) {
        char **symbols = backtrace_symbols(err->backtrace, err->backtrace_len);
        for (int j = 0; j < err->backtrace_len; ++j) {
            i += snprintf(dest + i, len - i, "  {%d} %s\n", j, symbols[j]);
        }

        free(symbols);
    }

    return i;
}

static bool dump_to_syslog = false;

void pond_syslog()
{
    openlog(NULL, LOG_CONS | LOG_NDELAY | LOG_PID, LOG_USER);
    dump_to_syslog = true;
}

void pond_perror(struct pond_error *err)
{
    char buf[128 + pond_err_msg_cap + 80 * pond_err_backtrace_cap];
    size_t len = pond_strerror(err, buf, sizeof(buf));

    if (!dump_to_syslog) {
        if (write(2, buf, len) == -1)
            fprintf(stderr, "pond_perror failed: %s", strerror(errno));
    }
    else syslog(err->warning ? LOG_WARNING : LOG_ERR, "%s", buf);
}


static void pond_backtrace(struct pond_error *err)
{
    err->backtrace_len = backtrace(err->backtrace, pond_err_backtrace_cap);
    if (err->backtrace_len == -1)
        printf("unable to sample backtrace: %s", strerror(errno));
}


// -----------------------------------------------------------------------------
// fail
// -----------------------------------------------------------------------------

static bool abort_on_fail = 0;
void pond_dbg_abort_on_fail() { abort_on_fail = true; }

void pond_vfail(const char *file, int line, const char *fmt, ...)
{
    pond_errno = (struct pond_error)
        { .warning = false, .errno_ = 0, .file = file, .line = line };

    va_list args;
    va_start(args, fmt);
    (void) vsnprintf(pond_errno.msg, pond_err_msg_cap, fmt, args);
    va_end(args);

    pond_backtrace(&pond_errno);
    if (abort_on_fail) pond_abort();
}

void pond_vfail_errno(const char *file, int line, const char *fmt, ...)
{
    pond_errno = (struct pond_error)
        { .warning = false, .errno_ = errno, .file = file, .line = line };

    va_list args;
    va_start(args, fmt);
    (void) vsnprintf(pond_errno.msg, pond_err_msg_cap, fmt, args);
    va_end(args);

    pond_backtrace(&pond_errno);
    if (abort_on_fail) pond_abort();
}


// -----------------------------------------------------------------------------
// warn
// -----------------------------------------------------------------------------

static bool abort_on_warn = 0;
void pond_dbg_abort_on_warn() { abort_on_warn = true; }

void pond_vwarn_va(const char *file, int line, const char *fmt, va_list args)
{
    struct pond_error err = { .warning = true, .file = file, .line = line };
    (void) vsnprintf(err.msg, pond_err_msg_cap, fmt, args);
    pond_backtrace(&err);

    pond_perror(&err);
}


void pond_vwarn(const char *file, int line, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    pond_vwarn_va(file, line, fmt, args);

    va_end(args);
}

void pond_vwarn_errno(const char *file, int line, const char *fmt, ...)
{

    struct pond_error err =
        { .warning = true, .errno_ = errno, .file = file, .line = line };

    va_list args;
    va_start(args, fmt);
    (void) vsnprintf(err.msg, pond_err_msg_cap, fmt, args);
    va_end(args);

    pond_backtrace(&err);

    pond_perror(&err);
}
