#include <stdio.h>
#include <stdlib.h>
#include "timer.h"
#include <time.h>
#include <unistd.h>


typedef struct {
	int id;
	int sec;
	int u_sec;
}timer_struct;

#define MAX_SIZE 300
timer_struct _t_timer[MAX_SIZE];

void timer_test(void * arg)
{
	timer_struct *ptr = (timer_struct *)arg;
	printf("##################### id[%d], sec[%d], u_sec[%d]  %d\n", ptr->id, ptr->sec, ptr->u_sec, time(NULL));
	//printf("%d\n", time(NULL));
	ptr->id = timer_add(ptr->sec, 0, timer_test,(void *)ptr);
	return;
}

void timer_cancle(void * arg)
{
	printf("timer_cancled!!!!!!!!! \n");
}
int main(int argc, char *argv[])
{
	timer_init();
	sleep(1);
	int i;
	/*for (i = 0; i < MAX_SIZE; i++) {
		_t_timer[i].sec = 10 - i*2;
		_t_timer[i].u_sec= 0;
		
		_t_timer[i].id = timer_add(_t_timer[i].sec, 0, timer_test,(void *)&_t_timer[i]);
		printf("timer_add id[%d] sec[%d], now[%d]\n", _t_timer[i].id, _t_timer[i].sec, time(0));
		//sleep(1);

	}*/
	//timer_rem(1, timer_cancle);
	_t_timer[0].sec = 2;
	_t_timer[0].id = timer_add(_t_timer[0].sec, 0, timer_test,(void *)&_t_timer[0]);

	sleep(15000);
	return 0;
}
