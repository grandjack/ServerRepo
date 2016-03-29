#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "timer.h"
#include <unistd.h>


typedef struct {
	int id;
	int sec;
	int u_sec;
}timer_struct;

#define MAX_SIZE (1000)
timer_struct _t_timer[MAX_SIZE];

void timer_test(void * arg)
{
	timer_struct *ptr = (timer_struct *)arg;
	printf("##################### id[%d], sec[%d], u_sec[%d]  %d\n", ptr->id, ptr->sec, ptr->u_sec, time(NULL));
	//printf("%d\n", time(NULL));
	return;
}

void timer_test2(void * arg)
{

	printf("Just test %d\n", time(NULL));
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
	for (i = 0; i < MAX_SIZE -2; i++) {
		_t_timer[i].sec = 2;
		_t_timer[i].u_sec= i*100;
		
		_t_timer[i].id = timer_add(_t_timer[i].sec, 0, timer_test,(void *)&_t_timer[i]);
		printf("timer_add id[%d] sec[%d], now[%d]\n", _t_timer[i].id, _t_timer[i].sec, time(0));
		//sleep(1);

	}

//	timer_rem(2, timer_cancle);
//	timer_rem(3, timer_cancle);
//	_t_timer[MAX_SIZE -2].id = timer_add(3, 0, timer_test2, NULL);
//	_t_timer[MAX_SIZE -1].id = timer_add(4, 0, timer_test2, NULL);
	timer_rem(7, NULL);
sleep(22);
	timer_add(7, 0, timer_test2, NULL);
	
//	timer_clear();
	sleep(MAX_SIZE +20);
	printf("sleep over\n");
	return 0;
}
