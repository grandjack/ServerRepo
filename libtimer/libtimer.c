/*
 * Copyright (C) 2014
 *
 * This program is free software; 
 *
 * Author(s): GrandJack
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>
#include <errno.h>		/* ETIMEDOUT */
#include <string.h>		/* memcpy */
#include <unistd.h>
#include <fcntl.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h> 
#include <sys/sysinfo.h>
#include "timer.h"
#include "unixsktapi.h"

/* for conversion purpose */
#define NANO_PER_MICRO	1000
#define MICRO_PER_SEC	1000000

/* max number of microsecond in a timeval struct */
#define MAX_USEC	999999

/* microseconds in a millisecond */
#define ALMOST_NOW	1000

#ifndef MAX_SOCKET_PENDING_CONNECTIONS
#define MAX_SOCKET_PENDING_CONNECTIONS	64
#endif

#ifndef TIMER_SOCKET_FILE_PATH
#define TIMER_SOCKET_FILE_PATH				"/var/run"
#endif

#define TIMER_SOCKET_FILE				TIMER_SOCKET_FILE_PATH"/timer_%d.socket"

#ifndef	TIMER_LOG_PATH
#define TIMER_LOG_PATH					"/var/log"
#endif

#ifdef TIMER_DEBUG
#define TIMER_LOG_FILE					TIMER_LOG_PATH"/timer.log"
#else
#define TIMER_LOG_FILE					"/dev/null"
#endif

#define MAX_BUFFER_SIZE	64
static char socket_file[MAX_BUFFER_SIZE] = { 0 };

/*
 * evals true if struct timeval t1 defines a time strictly before t2;
 * false otherwise.
 */
#define TV_LESS_THAN(t1,t2)	(((t1).tv_sec < (t2).tv_sec) ||\
				(((t1).tv_sec == (t2).tv_sec) &&\
				 ((t1).tv_usec < (t2).tv_usec)))

/*
 * Assign struct timeval tgt the time interval between the absolute
 * times t1 and t2.
 * IT IS ASSUMED THAT t1 > t2, use TV_LESS_THAN macro to test it.
 */
#define TV_MINUS(t1,t2,tgt)	if ((t1).tv_usec >= (t2).tv_usec) {\
					(tgt).tv_sec = (t1).tv_sec -\
						       (t2).tv_sec;\
					(tgt).tv_usec = (t1).tv_usec -\
       						        (t2).tv_usec;\
				}\
				else {\
					(tgt).tv_sec = (t1).tv_sec -\
						       (t2).tv_sec -1;\
					(tgt).tv_usec = (t1).tv_usec +\
						(MAX_USEC - (t2).tv_usec);\
				}
				
#define mysleep(sec, usec)\
	do{\
		struct timespec slptm;\
		slptm.tv_sec = sec;\
		slptm.tv_nsec = (1000 * usec)%1000000000;\
		if (nanosleep(&slptm, NULL) == -1) {\
			fprintf(stderr, "Failed to nanosleep[%s]\n", strerror(errno));\
		}\
	} while (0)
	
typedef enum {
	e_ACTION_TYPY_MIN,
	e_ACTION_TYPY_ADD = e_ACTION_TYPY_MIN,
	e_ACTION_TYPY_DEL,
	e_ACTION_TYPY_DEL_ALL,
	e_ACTION_TYPY_DESTRORY,
	e_ACTION_TYPY_MAX
} e_action_type;

typedef struct timer tl_timer_t;

struct timer {
	e_action_type action;
	struct timeval timeout;
	void (*handler) (void *);
	void *handler_arg;
	int id;
	tl_timer_t *next;
	tl_timer_t *prev;
};

static struct {
	tl_timer_t *first;
	tl_timer_t *last;
	pthread_mutex_t mutex;
	int cur_id;
} timerq;

static struct {
	tl_timer_t *first;
	tl_timer_t *last;
} timeout_list;

#define MAX_BUF_SIZE	(sizeof(unix_domain_msg) + sizeof(tl_timer_t))

static pthread_mutex_t timeout_mutex;
static pthread_cond_t timeout_cond;

static unsigned char timeout_thread_is_on = 0;
static FILE *std_err = NULL;
static unsigned char beInitialed = 0;

static void timer_free(tl_timer_t * /*t */ );
static void timer_dequeue(tl_timer_t * /*t */ );
static void * timer_server_task(void *arg);
static int my_clock_gettime(clockid_t clk_id, struct timespec *tp);
static int handle_proc_req(int socket);
static int handle_msg(tl_timer_t *msg);
static int add_timernode_to_list(tl_timer_t *msg);
static int del_timernode_from_list(tl_timer_t *msg);
static int get_rest_time(struct timeval *abs_rest);
static int del_all_timer(int need_destrory);
static void timer_execute();
static long get_uptime();
static int send_recv_msg(tl_timer_t *req_info, int need_recv);
static int add_timeout_list(void (*hdl) (void *), void *hdl_arg);
static int get_pending_list_count();
static tl_timer_t *get_pending_list();
static int get_waite_interval();
static void *time_out_task(void *arg);

static long get_uptime()
{
	int ret = 0;
	long sec = 0;
	struct sysinfo info;
	ret = sysinfo(&info);
	if(ret < 0) {
		fprintf(std_err, "sysinfo error[%s]", strerror(errno));
		return sec;
	}
	sec = info.uptime;
	return sec;
}

int my_gettimeofday(struct timeval *timeval, void *unused)
{
	int ret = 0;	
	if(NULL == timeval) {
		fprintf(std_err, "invalid argument.\n");
		return -1;
	}
	
	struct timespec ts;
	memset(&ts, 0, sizeof(struct timespec));
	ret = my_clock_gettime(CLOCK_MONOTONIC, &ts);
	if(ret < 0) {
		fprintf(std_err, "my_clock_gettime error[%s]", strerror(errno));
		return -1;
	}
	timeval->tv_sec = ts.tv_sec;
	timeval->tv_usec = ts.tv_nsec / 1000;
	
	return (0);
}

int my_clock_gettime(clockid_t clk_id, struct timespec *tp)
{	
	static int flag = 0;
	
	if(NULL == tp) {
		fprintf(std_err, "invalid argument.");
		return -1;
	}
	
#ifdef CONFIG_LOW_VERSION_LIBC
	tp->tv_sec = get_uptime();
	tp->tv_nsec = 0;
#else
	int ret = 0;
	int retry_count = 0;
	if (!flag) {
		while(((ret = clock_gettime(clk_id, tp)) < 0 ) && (retry_count ++ < 3)) {
			mysleep(0, 100);
		}

		if(ret < 0) {
			flag = 1;
			fprintf(std_err, "clock_gettime error[%s]", strerror(errno));
		}
	}
	
	if(flag) {
		tp->tv_sec = get_uptime();
		tp->tv_nsec = 0;
		flag = 1;
	}
#endif

	return 0;
}
static void timer_free(tl_timer_t * t)
{
	if (t == NULL)
		return;
	free(t);
	t = NULL;
	return;
}

static void timer_dequeue(tl_timer_t * t)
{

	if (t == NULL)
		return;

	if (t->prev == NULL)
		/* t is the first of the queue */
		timerq.first = t->next;
	else
		t->prev->next = t->next;

	if (t->next == NULL)
		/* t is the last of the queue */
		timerq.last = t->prev;
	else
		t->next->prev = t->prev;

	timer_free(t);
	return;
}


/*
 * Initialize the timerq struct, spwan the ticker thread,
 * wait its initialization and return.
 */
int timer_init()
{
	int ret;
	pthread_t thread_id;
	timerq.first = timerq.last = NULL;
	timerq.cur_id = 0;
	timeout_list.first = timeout_list.last = NULL;

	pthread_mutex_init(&timeout_mutex, NULL);
	pthread_cond_init(&timeout_cond, NULL);
	pthread_mutex_init(&timerq.mutex, NULL);
	
	std_err = fopen(TIMER_LOG_FILE, "a");
	if (std_err == NULL) {
		fprintf(stderr, "fopen %s failed.\n", TIMER_LOG_FILE);
		return -1;
	}

	ret = pthread_create(&thread_id, NULL, timer_server_task, NULL);
	if (ret != 0) {
		fprintf(std_err, "pthread_create() failed, err: %d,[%s]", errno, strerror(errno));
		return -1;
	}

    pthread_mutex_lock(&timeout_mutex);
    while(beInitialed == 0) {
        pthread_cond_wait(&timeout_cond, &timeout_mutex);
    }    
    pthread_mutex_unlock(&timeout_mutex);

	return 0;
}

void * timer_server_task(void *arg)
{
	int ret;
	int socket, acpt_socket;
	struct timeval timeout;
	struct timeval *time_ptr = NULL;

	pthread_detach(pthread_self());
	
	snprintf(socket_file, sizeof(socket_file), TIMER_SOCKET_FILE, getpid());
	
	/* Init socket  */
	ret = unix_domain_socket_init(&socket, socket_file);
	if (ret != 0) {
		fprintf(std_err, "cmsvc_init() failed.");
		goto out;
	}

    pthread_mutex_lock(&timeout_mutex);
    beInitialed = 1;
    pthread_cond_signal(&timeout_cond);
    pthread_mutex_unlock(&timeout_mutex);

	/* select socket, proccess when got a message. */
	while (1) {

		ret = unix_domain_accept_req(socket, &acpt_socket, time_ptr);
		if (ret == 0) {//timeout
			timer_execute();
			goto check;
		} else if (ret < 0) {
			goto check;
		}

		ret = handle_proc_req(acpt_socket);
		if (ret != 0) {
			fprintf(std_err, "handle_proc_req() failed.\n");
		}

check:
		if (get_rest_time(&timeout) < 0 ) {
			time_ptr = NULL;
		} else {
			time_ptr = &timeout;
		}
		
	}

out:
	unix_domain_socket_clean(socket, socket_file);
	return (void *)0;

}

int handle_proc_req(int socket)
{
	int ret;
	struct timeval timeout;
	unix_domain_msg *msg = NULL;
	char msg_buf[MAX_BUF_SIZE] = { 0 };

	msg = (unix_domain_msg *)msg_buf;
	timeout.tv_sec = 3;
	timeout.tv_usec = 0;

	ret = unix_domain_recv_msg(socket, msg, MAX_BUF_SIZE, &timeout);
	if (ret != 0) {
		fprintf(std_err, "recv_msg() failed.\n");
		goto done;
	}

	ret = handle_msg((tl_timer_t *)msg->data);
	if (ret != 0) {
		fprintf(std_err, "handle_msg() failed.\n");
	}
	
	msg->size = sizeof(tl_timer_t);
	ret = unix_domain_send_msg(socket, (void *)msg, sizeof(unix_domain_msg) + msg->size, &timeout);
	if (ret != 0) {
		fprintf(std_err, "send_msg() failed.\n");
	}

done:

	if ((void *)msg != (void *)msg_buf) {
		free(msg);
		msg = NULL;
	}
	
	if (socket >= 0) {
		close(socket);
	}

	return (ret);

}

int handle_msg(tl_timer_t *msg)
{
	int ret = 0;
	
	if (msg == NULL) {
		fprintf(std_err, "invalid arg.\n");
		return -1;
	}
	
	switch(msg->action) {

		case e_ACTION_TYPY_ADD:
			if ((ret = add_timernode_to_list(msg)) < 0) {
				fprintf(std_err, "add_timernode_to_list failed.\n");
			}
			break;
		case e_ACTION_TYPY_DEL:
			if ((ret = del_timernode_from_list(msg)) < 0) {
				fprintf(std_err, "del_timernode_from_list failed.\n");
			}
			break;
		case e_ACTION_TYPY_DESTRORY:
			if ((ret = del_all_timer(1)) < 0) {
				fprintf(std_err, "del_all_timer failed.\n");
			}
			break;
		case e_ACTION_TYPY_DEL_ALL:
			if ((ret = del_all_timer(0)) < 0) {
				fprintf(std_err, "del_all_timer failed.\n");
			}
			break;
		default:
			fprintf(std_err, "handle_msg failed.\n");
			break;
	}
	
	return ret;
}
int add_timernode_to_list(tl_timer_t *msg)
{
	struct timeval new_time;
	tl_timer_t *tmp = NULL;
	tl_timer_t *app = NULL;
	int id = 0, ret = 0;

	if (msg == NULL)
		return -1;

	/* ensure timeout is in the future */
	if ((msg->timeout.tv_sec < 0) || (msg->timeout.tv_usec < 0) || ((msg->timeout.tv_sec == 0) && (msg->timeout.tv_usec == 0))) {
		msg->id = -1;
		ret = -1;
		goto done;
	}

	app = (tl_timer_t *) malloc(sizeof(tl_timer_t));
	if (app == NULL) {
		msg->id = -1;
		ret = -1;
		goto done;
	}

	/* relative to absolute time for timer */
	my_gettimeofday(&new_time, NULL);

	/* add 10^6 microsecond units to seconds */
	new_time.tv_sec += msg->timeout.tv_sec + (new_time.tv_usec + msg->timeout.tv_usec) / MICRO_PER_SEC;
	/* keep microseconds inside allowed range */
	new_time.tv_usec = (new_time.tv_usec + msg->timeout.tv_usec) % MICRO_PER_SEC;

	memcpy(&app->timeout, &new_time, sizeof(struct timeval));
	app->handler = msg->handler;
	app->handler_arg = msg->handler_arg;

	pthread_mutex_lock(&timerq.mutex);

	id = timerq.cur_id++;
	if (id == -1) {
		id = timerq.cur_id++;
	}
	app->id = id;

	if (timerq.first == NULL) {
		/* timer queue empty */
		timerq.first = app;
		timerq.last = app;
		app->prev = NULL;
		app->next = NULL;
		msg->id = id;
		goto done;
	}

	/* there is at least a timer in the queue */
	tmp = timerq.first;

	/* find the first timer that expires before app */
	while (tmp != NULL) {

		if (TV_LESS_THAN(app->timeout, tmp->timeout))
			break;

		tmp = tmp->next;
	}

	if (tmp == NULL) {
		/* app is the longest timer */
		app->prev = timerq.last;
		app->next = NULL;
		timerq.last->next = app;
		timerq.last = app;
		msg->id = id;
		goto done;
	}

	if (tmp->prev == NULL) {
		/* app is the shoprtest timer */
		app->prev = NULL;
		app->next = tmp;
		tmp->prev = app;
		timerq.first = app;

	} else {
		app->prev = tmp->prev;
		app->next = tmp;
		tmp->prev->next = app;
		tmp->prev = app;
	}

	msg->id = id;

done:
	pthread_mutex_unlock(&timerq.mutex);
	return ret;
}

int del_timernode_from_list(tl_timer_t *msg)
{
	tl_timer_t *t;

	if (msg->id == -1) {
		return -1;
	}

	pthread_mutex_lock(&timerq.mutex);
	t = timerq.first;

	/* look for timer id */
	while (t != NULL) {

		if (t->id == msg->id)
			break;
		t = t->next;
	}

	if (t == NULL) {
		/* timer id is not in queue (maybe empty) */
		pthread_mutex_unlock(&timerq.mutex);
		return -1;
	}

	/* timer id is in the queue */
	if (msg->handler) {
		msg->handler(t->handler_arg);
		t->handler_arg = NULL;
	}
	
	timer_dequeue(t);

	pthread_mutex_unlock(&timerq.mutex);

	return 0;
}
int del_all_timer(int need_destrory)
{
	tl_timer_t *t;

	pthread_mutex_lock(&timerq.mutex);
	t = timerq.last;
	while (t != NULL) {
		timer_dequeue(t);
		t = timerq.last;
	}
	pthread_mutex_unlock(&timerq.mutex);
	
	if (need_destrory) {
		pthread_mutex_destroy(&timerq.mutex);
		pthread_mutex_destroy(&timeout_mutex);
		pthread_cond_destroy(&timeout_cond);
		fclose(std_err);
		pthread_exit(NULL);
	}
	return 0;
}
void timer_execute()
{
	void (*hdl) (void *);
	void *hdl_arg = NULL;
	int ret = 0;

	if (timerq.first != NULL) {
		hdl = timerq.first->handler;
		hdl_arg = timerq.first->handler_arg;
		pthread_mutex_lock(&timerq.mutex);
		timer_dequeue(timerq.first);
		pthread_mutex_unlock(&timerq.mutex);
		
		if (hdl) {
			ret = add_timeout_list(hdl, hdl_arg);
			if (ret != 0) {
				fprintf(std_err, "add_timeout_list() failed, err: %d,[%s]", errno, strerror(errno));
				return;
			}

			pthread_mutex_lock(&timeout_mutex);
			if (timeout_thread_is_on == 0) {
				timeout_thread_is_on = 1;
				pthread_mutex_unlock(&timeout_mutex);
				
				pthread_t thread_id;
				pthread_attr_t attr;
				pthread_attr_init(&attr);
				
				pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
				
				ret = pthread_create(&thread_id, &attr, time_out_task, NULL);
				if (ret != 0) {
					fprintf(std_err, "pthread_create() failed, err: %d,[%s]", errno, strerror(errno));
					pthread_mutex_lock(&timeout_mutex);
					timeout_thread_is_on = 0;
					pthread_mutex_unlock(&timeout_mutex);
					return;
				}
			}else {
				pthread_cond_signal(&timeout_cond);
				pthread_mutex_unlock(&timeout_mutex);
			}
		}
	} else {
		fprintf(std_err, "timerq.first is null.\n");
	}

	return ;
}
int get_rest_time(struct timeval *abs_rest)
{
	struct timeval abs_now;
	struct timeval *abs_to = NULL;

	if (abs_rest == NULL || timerq.first == NULL) {
		return -1;
	}

	abs_to = &(timerq.first->timeout);
	memset(abs_rest, 0, sizeof(struct timeval));
	/* absolute to relative time */
	my_gettimeofday(&abs_now, NULL);
	
	if (TV_LESS_THAN(abs_now, *abs_to)) {
		/* ok, timeout is in the future */
		TV_MINUS(*abs_to, abs_now, *abs_rest);
	} else {
		abs_rest->tv_sec = 0;
		abs_rest->tv_usec = ALMOST_NOW;
	}

	return 0;
}

int timer_add(long sec, long usec, void (*hndlr) (void *), void *hndlr_arg)
{
	int ret = 0;
	tl_timer_t req_info;
	int need_recv = 1;

	if (hndlr == NULL)
		return -1;

	/* ensure timeout is in the future */
	if ((sec < 0) || (usec < 0) || ((sec == 0) && (usec == 0))) {
		return -1;
	}

	memset(&req_info, 0, sizeof(tl_timer_t));

	req_info.action = e_ACTION_TYPY_ADD;
	req_info.handler = hndlr;
	req_info.handler_arg = hndlr_arg;
	req_info.timeout.tv_sec = sec;
	req_info.timeout.tv_usec = usec;

	ret = send_recv_msg(&req_info, need_recv);
	if (ret != 0 ) {
		fprintf(std_err, "send_recv_msg() failed.\n");
		return -1;
	}
	
	return req_info.id;
}

void timer_rem(int id, void (*free_arg) (void *))
{
	int ret = 0;
	tl_timer_t req_info;
	int need_recv = 1;

	if (id < 0) {
		return;
	}

	memset(&req_info, 0, sizeof(tl_timer_t));

	req_info.action = e_ACTION_TYPY_DEL;
	req_info.id = id;
	req_info.handler = free_arg;

	ret = send_recv_msg(&req_info, need_recv);
	if (ret != 0 ) {
		fprintf(std_err, "send_recv_msg() failed.\n");
		return ;
	}
	
	return ;
}

void timer_destroy()
{
	int ret = 0;
	tl_timer_t req_info;
	int need_recv = 0;
	
	memset(&req_info, 0, sizeof(tl_timer_t));

	req_info.action = e_ACTION_TYPY_DESTRORY;

	ret = send_recv_msg(&req_info, need_recv);
	if (ret != 0 ) {
		fprintf(std_err, "send_recv_msg() failed.\n");
		return ;
	}
	
	return ;
}

void timer_clear()
{
	int ret = 0;
	tl_timer_t req_info;
	int need_recv = 1;
	
	memset(&req_info, 0, sizeof(tl_timer_t));

	req_info.action = e_ACTION_TYPY_DEL_ALL;

	ret = send_recv_msg(&req_info, need_recv);
	if (ret != 0 ) {
		fprintf(std_err, "send_recv_msg() failed.\n");
		return ;
	}
	
	return ;
}

int send_recv_msg(tl_timer_t *req_info, int need_recv)
{
	int ret = 0, socket;
	struct timeval timeout;
	unix_domain_msg *msg = NULL;
	char msg_buf[MAX_BUF_SIZE] = { 0 };

	if (req_info == NULL) {
		return -1;
	}

	if (strlen(socket_file) == 0) {
		snprintf(socket_file, sizeof(socket_file), TIMER_SOCKET_FILE, getpid());
	}
	
	msg = (unix_domain_msg *)msg_buf;
	memcpy(msg->data, req_info, sizeof(tl_timer_t));
	msg->size = sizeof(tl_timer_t);

	ret = unix_domain_socket_connect(&socket, socket_file);
	if (ret != 0) {
		fprintf(std_err, "unix_domain_socket_connect() failed.\n");
		return -1;
	}

	timeout.tv_sec = 3;
	timeout.tv_usec = 0;
	ret = unix_domain_send_msg(socket, (void *)msg, sizeof(unix_domain_msg) + msg->size, &timeout);
	if (ret != 0) {
		fprintf(std_err, "send_msg() failed.\n");
		close(socket);
		return -1;
	}

	if (need_recv) {
		timeout.tv_sec = 3;
		timeout.tv_usec = 0;
		memset(req_info, 0, sizeof(tl_timer_t));
		ret = unix_domain_recv_msg(socket, msg, MAX_BUF_SIZE, &timeout);
		if (ret != 0) {
			fprintf(std_err, "recv_msg() failed.\n");
			close(socket);
			return -1;
		}
		memcpy(req_info, msg->data, msg->size);
	}

	close(socket);
	
	return 0;
}

int add_timeout_list(void (*hdl) (void *), void *hdl_arg)
{
	tl_timer_t *new_node = NULL;
	
	if (hdl == NULL) {
		return -1;
	}

	pthread_mutex_lock(&timeout_mutex);
	if (timeout_list.first == NULL) {
		timeout_list.first = (tl_timer_t *)malloc(sizeof(tl_timer_t));
		if (timeout_list.first == NULL) {
			pthread_mutex_unlock(&timeout_mutex);
			return -1;
		} else {
			timeout_list.first->handler = hdl;
			timeout_list.first->handler_arg = hdl_arg;
			timeout_list.first->next = NULL;
			timeout_list.first->prev = NULL;
			timeout_list.last = timeout_list.first;
			pthread_mutex_unlock(&timeout_mutex);
			return 0;
		}
	}
	
	new_node = (tl_timer_t *)malloc(sizeof(tl_timer_t));
	if (new_node == NULL) {
		pthread_mutex_unlock(&timeout_mutex);
		return -1;
	}
	
	new_node->handler = hdl;
	new_node->handler_arg = hdl_arg;
	
	new_node->next = timeout_list.first;
	new_node->prev = NULL;
	timeout_list.first->prev = new_node;
	
	timeout_list.first = new_node;
	
	pthread_mutex_unlock(&timeout_mutex);
	
	return 0;
}
int get_pending_list_count()
{
	tl_timer_t *ptr = NULL;
	int count = 0;

	pthread_mutex_lock(&timeout_mutex);
	ptr = timeout_list.first;
	while(ptr) {
		count++;
		ptr = ptr->next;
	}
	pthread_mutex_unlock(&timeout_mutex);
	return count;
}
tl_timer_t *get_pending_list()
{
	tl_timer_t *ptr = NULL;

	pthread_mutex_lock(&timeout_mutex);
	if (timeout_list.last) {
		ptr = timeout_list.last;
		if (timeout_list.last->prev == NULL) {
			timeout_list.last = NULL;
			timeout_list.first = NULL;
		} else {
			timeout_list.last = timeout_list.last->prev;
			timeout_list.last->next = NULL;
		}
	}
	pthread_mutex_unlock(&timeout_mutex);
	return ptr;
}
int get_waite_interval()
{
	int interval = 0;
	struct timeval time, timeintval;
	my_gettimeofday(&time, NULL);

	pthread_mutex_lock(&timerq.mutex);
	if (timerq.last) {
		if (TV_LESS_THAN(time, timerq.last->timeout)) {
			TV_MINUS(timerq.last->timeout, time, timeintval);
		} else {
			memset(&timeintval, 0, sizeof(struct timeval));
		}
		
		interval = timeintval.tv_sec;

		interval = (interval>(60*10)) ? 600 : interval;
		interval = interval < 0 ? 0: interval;
	}
	pthread_mutex_unlock(&timerq.mutex);
	
	return (interval);
}
void *time_out_task(void *arg)
{
	tl_timer_t *first_arg = NULL;
	int ret, waite_interval = 0;
	
	while (1) {
		if (get_pending_list_count() == 0) {
			struct timespec timerout;
			waite_interval = get_waite_interval();
			timerout.tv_sec = time(0) + waite_interval + 5;
			
			pthread_mutex_lock(&timeout_mutex);
			ret = pthread_cond_timedwait(&timeout_cond, &timeout_mutex, &timerout);
			if(ret == ETIMEDOUT) {
				timeout_thread_is_on = 0;
				pthread_mutex_unlock(&timeout_mutex);
				pthread_exit((void *)0);
			} else {
				pthread_mutex_unlock(&timeout_mutex);
				continue;
			}
		} else {
			first_arg = get_pending_list();
			if (first_arg == NULL) {
				continue;
			} else {
				if (first_arg->handler) {
					first_arg->handler(first_arg->handler_arg);
					free(first_arg);
					first_arg = NULL;
				}
			}
		}
	
	}
	return NULL;
}
