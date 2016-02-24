#include "utils.h"
#include "debug.h"

void signal_handle(evutil_socket_t sig, short events, void *arg)
{
    struct event_base *base = (event_base*)arg;

    LOG_INFO(MODULE_COMMON, "Caught an SIGINT signal; exiting cleanly immediately.");

    event_base_loopexit(base, NULL);
}

void AddEventForBase(struct event_base *base, struct event *ev, evutil_socket_t fd, short events,void (*callback)(evutil_socket_t, short, void *), void *arg)
{
    event_set(ev, fd, events, callback, arg);
    event_base_set(base, ev);

    if (event_add(ev, NULL) == -1) {
        LOG_ERROR(MODULE_COMMON, "event_add failed.");
        return ;
    }
}


