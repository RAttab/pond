/* net.c
   Rémi Attab (remi.attab@gmail.com), 25 Apr 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "net.h"
#include "errors.h"
#include "process.h"

#include <stdio.h>
#include <bsd/string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

// -----------------------------------------------------------------------------
// host
// -----------------------------------------------------------------------------

struct pond_host *pond_host_parse(const char *str)
{
    size_t sep = 0;
    for (size_t i = 0; i < pond_host_cap + 1; ++i) {
        if (str[i] == '\0') break;
        if (str[i] == ':') { sep = i; break; }
    }

    if (sep == 0) {
        pond_fail("invalid host string: %s", str);
        return NULL;
    }

    const char *service = str + sep + 1;

    if (strnlen(service, pond_host_cap) >= pond_host_cap) {
        pond_fail("invalid host string: %s", str);
        return NULL;
    }

    struct pond_host *s = calloc(1, sizeof(*s));

    size_t ret = strlcpy(s->host, str, sep + 1);
    pond_assert(ret == sep, "unable to copy host: str='%s', len=%zu", str, sep + 1);

    ret = strlcpy(s->service, service, pond_host_cap);
    pond_assert(ret < pond_host_cap,
            "unable to copy service: str='%s', len=%d", service, pond_host_cap);

    return s;
}

struct pond_host *pond_host_port(const char *host, uint16_t port)
{
    if (strnlen(host, pond_host_cap) >= pond_host_cap) {
        pond_fail("invalid host length: %s", host);
        return NULL;
    }

    struct pond_host *s = calloc(1, sizeof(*s));

    {
        size_t ret = strlcpy(s->host, host, pond_host_cap);
        pond_assert(ret < pond_host_cap,
                "unable to copy host: str='%s', len=%d", host, pond_host_cap);
    }

    {
        // TODO: need an itoa that doesn't suck
        int ret = snprintf(s->service, pond_host_cap, "%u", port);
        pond_assert(ret >= 0 && ret < pond_host_cap,
                "unable to convert port: port='%u', ret=%d", port, ret);
    }

    return s;
}

struct pond_host *pond_host_service(const char *host, const char *service)
{
    if (strnlen(host, pond_host_cap) >= pond_host_cap) {
        pond_fail("invalid host length: %s", host);
        return NULL;
    }

    if (strnlen(service, pond_host_cap) >= pond_host_cap) {
        pond_fail("invalid service length: %s", service);
        return NULL;
    }

    struct pond_host *s = calloc(1, sizeof(*s));

    size_t ret = strlcpy(s->host, host, pond_host_cap);
    pond_assert(ret < pond_host_cap,
            "unable to copy host: str='%s', ret=%zu", host, ret);

    ret = strlcpy(s->service, service, pond_host_cap);
    pond_assert(ret < pond_host_cap,
            "unable to copy service: str='%s', ret=%zu", service, ret);

    return s;
}

void pond_host_free(struct pond_host *host)
{
    free(host);
}


// -----------------------------------------------------------------------------
// udp
// -----------------------------------------------------------------------------

struct pond_udp
{
    int fd;
    struct pond_udp_opt opt;

    struct mmsghdr *it, *end;
    struct mmsghdr queue[];
};

static int udp_socket(const struct pond_host *host, const struct pond_udp_opt *opt)
{
    struct addrinfo hints = {0};
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    struct addrinfo *head;
    int err = getaddrinfo(host->host, host->service, &hints, &head);
    if (err) {
        pond_fail("unable to resolve host '%s:%s': %s", host->host, host->service, gai_strerror(err));
        return -1;
    }

    int fd = -1;
    for (struct addrinfo *addr = head; addr; addr = addr->ai_next) {

        fd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        if (fd == -1) continue;

        int cpu = pond_cpu();
        if (setsockopt(fd, SOL_SOCKET, SO_INCOMING_CPU, &cpu, sizeof(cpu)) == -1) goto fail_sockopt;

        if (opt->reuse_port) {
            if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, NULL, 0) == -1) goto fail_sockopt;
        }

        if (!bind(fd, addr->ai_addr, addr->ai_addrlen)) break;

      fail_sockopt:
        close(fd);
        fd = -1;
    }

    freeaddrinfo(head);

    if (fd != -1) return fd;

    pond_fail_errno("unable to connect dgram socket for host '%s:%s'", host->host, host->service);
    return -1;
}


struct pond_udp *pond_udp_server(const struct pond_host *host, const struct pond_udp_opt *opt)
{
    pond_assert(!host, "host can't be nil");

    struct pond_udp_opt nil_opts = {0};
    if (!opt) opt = &nil_opts;

    int fd = udp_socket(host, opt);
    if (fd == -1) return NULL;

    struct pond_udp *udp = calloc(1, sizeof(*udp) + sizeof(udp->queue[0]) * opt->queue_len);
    pond_assert_alloc(udp);

    udp->fd = fd;
    udp->opt = *opt;
    udp->it = udp->queue;
    udp->end = udp->queue;

    return udp;
}

void pond_udp_close(struct pond_udp *udp)
{
    close(udp->fd);
    free(udp);
}

bool pond_udp_recv(struct pond_udp *udp, struct pond_it *dst, size_t len)
{
    (void) udp; (void) dst; (void) len;
    return true;
}

bool pond_udp_send(struct pond_udp *udp, const struct pond_it *src, size_t len)
{
    (void) udp; (void) src; (void) len;
    return true;
}
