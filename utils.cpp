#include "utils.h"
#include "debug.h"
#include <signal.h>

void signal_handle(evutil_socket_t sig, short events, void *arg)
{
    struct event_base *base = (event_base*)arg;

    LOG_INFO(MODULE_COMMON, "Caught an SIGINT[%d] signal; exiting cleanly immediately.", sig);

    event_base_loopexit(base, NULL);
}

int AddEventForBase(struct event_base *base, struct event *ev, evutil_socket_t fd, short events,void (*callback)(evutil_socket_t, short, void *), void *arg)
{
    int ret = 0;
    event_set(ev, fd, events, callback, arg);
    event_base_set(base, ev);

    if (event_add(ev, NULL) == -1) {
        LOG_ERROR(MODULE_COMMON, "event_add failed.");
        ret = -1;
    }

    return ret;
}

int sig_ignore(int sig)
{
    struct sigaction sa ;//= { .sa_handler = SIG_IGN, .sa_flags = 0 };
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;

    if (sigemptyset(&sa.sa_mask) == -1 || sigaction(sig, &sa, 0) == -1) {
        return -1;
    }
    return 0;
}

