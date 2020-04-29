/* net.h
   Rémi Attab (remi.attab@gmail.com), 25 Apr 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>


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
// udp
// -----------------------------------------------------------------------------

struct pond_udp;

struct pond_udp_opt
{
    bool reuse_port;
    size_t queue_len;
};

struct pond_udp *pond_udp_server(const struct pond_host *host, const struct pond_udp_opt *opt);
void pond_udp_close(struct pond_udp *);

int pond_udp_fd(struct pond_udp *);
bool pond_udp_recv(struct pond_udp *, struct pond_it *dst, size_t len);
bool pond_udp_send(struct pond_udp *, const struct pond_it *src, size_t len);
