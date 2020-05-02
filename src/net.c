/* net.c
   Rémi Attab (remi.attab@gmail.com), 25 Apr 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "net.h"
#include "errors.h"
#include "process.h"
#include "math.h"
#include "buf.h"

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
// iovec
// -----------------------------------------------------------------------------


size_t pond_iov_write(struct pond_iov *iov, const uint8_t *src, size_t len)
{
    iov->len = pond_min(len, iov->cap);
    memcpy(iov->bin, src, iov->len);
    return iov->len;
}

size_t pond_iov_append(struct pond_iov *iov, const uint8_t *src, size_t len)
{
    len = pond_min(len, iov->cap - iov->len);
    memcpy(iov->bin + iov->len, src, len);
    iov->len += len;
    return len;
}


size_t pond_iov_read(const struct pond_iov *iov, uint8_t *dst, size_t len)
{
    len = pond_min(len, iov->len);
    memcpy(dst, iov->bin, len);
    return len;
}

struct pond_it pond_iov_it(struct pond_iov *iov)
{
    return (struct pond_it) {.it = iov->bin, .end = iov->bin + iov->cap };
}


static size_t iovec_len(const size_t *sizes, size_t cap)
{
    size_t sum = 0;
    for (size_t i = 0; i < cap; ++i) sum += sizes[0];

    return sizeof(struct pond_iovec) + sizeof(struct pond_iov) * cap + sum;
}

static void iovec_init(struct pond_iovec *iovec, const size_t *sizes, size_t cap)
{
    *iovec = (struct pond_iovec) { .cap = cap };

    size_t header_len = sizeof(struct pond_iovec) + sizeof(struct pond_iov) * cap;
    uint8_t *bin = ((uint8_t *) iovec) + header_len;

    for (size_t i = 0; i < cap; ++i) {
        struct pond_iov *iov = &iovec->vec[i];
        *iov = (struct pond_iov) {
            .cap = sizes[i],
            .bin = sizes[i] ? bin : NULL
        };
        bin += sizes[i];
    }
}

struct pond_iovec *pond_iovec_alloc(const size_t *sizes, size_t cap)
{
    struct pond_iovec *iovec = calloc(1, iovec_len(sizes, cap));
    pond_assert_alloc(iovec);
    iovec_init(iovec, sizes, cap);
    return iovec;
}

void pond_iovec_free(struct pond_iovec *iovec)
{
    free(iovec);
}

// -----------------------------------------------------------------------------
// mmsg
// -----------------------------------------------------------------------------

struct pond_mmsg
{
    size_t len, cap;
    size_t iovecs_len;
    struct msghdr headers[];
};

struct pond_mmsg *pond_mmsg_alloc(size_t msg_cap, const size_t *iov_sizes, size_t iov_cap)
{
    size_t headers_len = sizeof(struct msghdr) * msg_cap;
    size_t iovecs_len = iovec_len(iov_sizes, iov_cap) * msg_cap;
    struct pond_mmsg *mmsg = calloc(1, sizeof(*mmsg) + headers_len + iovecs_len);
    pond_assert_alloc(mmsg);

    *mmsg = (struct pond_mmsg) { .cap = msg_cap, .iovecs_len = iovecs_len };

    for (size_t i = 0; i < msg_cap; ++i) {
        struct msghdr *hdr = &mmsg->headers[i];
        struct pond_iovec *iov = pond_mmsg_iovec(mmsg, i);

        hdr->msg_iov = pond_iov_sys(iov);
        iovec_init(iov, iov_sizes, iov_cap);
    }

    return mmsg;
}

void pond_mmsg_free(struct pond_mmsg *mmsg)
{
    free(mmsg);
}


size_t pond_mmsg_len(const struct pond_mmsg *mmsg)
{
    return mmsg->len;
}

size_t pond_mmsg_cap(const struct pond_mmsg *mmsg)
{
    return mmsg->cap;
}


struct msghdr *pond_mmsg_header(struct pond_mmsg *mmsg, size_t i)
{
    return &mmsg->headers[i];
}

struct pond_iovec *pond_mmsg_iovec(struct pond_mmsg *mmsg, size_t i)
{
    return ((void *) mmsg) + sizeof(*mmsg) +
        sizeof(mmsg->headers[0]) * mmsg->cap +
        mmsg->iovecs_len * i;
}


// -----------------------------------------------------------------------------
// udp
// -----------------------------------------------------------------------------

struct pond_udp
{
    int fd;
    struct pond_udp_opt opt;
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

        if (opt->cpu_affinity) {
            int cpu = pond_cpu();
            if (setsockopt(fd, SOL_SOCKET, SO_INCOMING_CPU, &cpu, sizeof(cpu)) == -1)
                goto fail_sockopt;
        }

        if (opt->reuse_port) {
            if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, NULL, 0) == -1)
                goto fail_sockopt;
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

    struct pond_udp *udp = calloc(1, sizeof(*udp));
    pond_assert_alloc(udp);

    udp->fd = fd;
    udp->opt = *opt;
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
