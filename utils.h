#ifndef __UTILS_HEAD__
#define __UTILS_HEAD__

#ifdef __cplusplus
extern "C" {
#endif


#include <event.h>

typedef unsigned int    u_int32;
typedef unsigned short  u_int16;
typedef unsigned char   u_int8;

void signal_handle(evutil_socket_t sig, short events, void *arg);
int AddEventForBase(struct event_base *base, struct event *ev, evutil_socket_t fd, short events,void (*callback)(evutil_socket_t, short, void *), void *arg);
int sig_ignore(int sig);

#ifdef __cplusplus
}
#endif

#endif /*__UTILS_HEAD__*/
