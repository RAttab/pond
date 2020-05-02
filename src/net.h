/* net.h
   Rémi Attab (remi.attab@gmail.com), 25 Apr 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "compiler.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stddef.h>

#include <sys/socket.h>


// -----------------------------------------------------------------------------
// fwd decl
// -----------------------------------------------------------------------------

struct pond_it;

// -----------------------------------------------------------------------------
// host
// -----------------------------------------------------------------------------

enum { pond_host_cap = 250 };

struct pond_host
{
    char host[pond_host_cap];
    char service[pond_host_cap];
};

struct pond_host *pond_host_from_str(const char *str);
struct pond_host *pond_host_from_port(const char *host, uint16_t port);
struct pond_host *pond_host_from_service(const char *host, const char *service);

void pond_host_free(struct pond_host *addr);

// -----------------------------------------------------------------------------
// iovec
// -----------------------------------------------------------------------------

struct pond_iov
{
    uint8_t *bin;
    size_t len, cap;
};

pond_static_assert(
        offsetof(struct pond_iov, bin) == offsetof(struct iovec, iov_base) &&
        pond_sizeof_member(struct pond_iov, bin) == pond_sizeof_member(struct iovec, iov_base));

pond_static_assert(
        offsetof(struct pond_iov, len) == offsetof(struct iovec, iov_len) &&
        pond_sizeof_member(struct pond_iov, len) == pond_sizeof_member(struct iovec, iov_len));

#define pond_iov_sys(iov) ((struct iovec *) (iov))

size_t pond_iov_write(struct pond_iov *, const uint8_t *src, size_t len);
size_t pond_iov_append(struct pond_iov *, const uint8_t *src, size_t len);

size_t pond_iov_read(const struct pond_iov *, uint8_t *dst, size_t len);
struct pond_it pond_iov_it(struct pond_iov *);


struct pond_iovec
{
    size_t len, cap;
    struct pond_iov vec[];
};

pond_malloc
struct pond_iovec *pond_iovec_alloc(const size_t *sizes, size_t cap);
void pond_iovec_free(struct pond_iovec *);


// -----------------------------------------------------------------------------
// mmsg
// -----------------------------------------------------------------------------

struct pond_mmsg;

pond_malloc
struct pond_mmsg *pond_mmsg_alloc(size_t msg_cap, const size_t *iov_sizes, size_t iov_cap);
void pond_mmsg_free(struct pond_mmsg *);

size_t pond_mmsg_len(const struct pond_mmsg *);
size_t pond_mmsg_cap(const struct pond_mmsg *);

struct msghdr *pond_mmsg_header(struct pond_mmsg *, size_t i);
struct pond_iovec *pond_mmsg_iovec(struct pond_mmsg *, size_t i);


// -----------------------------------------------------------------------------
// udp
// -----------------------------------------------------------------------------

struct pond_udp;

struct pond_udp_opt
{
    bool cpu_affinity;
    bool reuse_port;
};

struct pond_udp *pond_udp_server(const struct pond_host *host, const struct pond_udp_opt *opt) pond_malloc;
void pond_udp_close(struct pond_udp *);

int pond_udp_fd(struct pond_udp *);
bool pond_udp_mrecv(struct pond_udp *, struct pond_mmsg *dst, size_t len);
bool pond_udp_msend(struct pond_udp *, const struct pond_mmsg *dst, size_t len);
