/* process.c
   Rémi Attab (remi.attab@gmail.com), 25 Apr 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "process.h"
#include "errors.h"

#include <unistd.h>
#include <stdatomic.h>

// -----------------------------------------------------------------------------
// utils
// -----------------------------------------------------------------------------


static atomic_size_t tid_counter = 1;
static __thread size_t tid_store = 0;

size_t pond_tid(void)
{
    if (!tid_store)
        tid_store = atomic_fetch_add_explicit(&tid_counter, 1, memory_order_relaxed);
    return tid_store;
}




size_t pond_cpus(void)
{
    long count = sysconf(_SC_NPROCESSORS_ONLN);
    if (count != -1) return count;

    pond_fail_errno("unable to call sysconf to get cpu count");
    pond_abort();
}
