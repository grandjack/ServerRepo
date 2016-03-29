/*
 * Copyright (C) 2014
 *
 * This program is free software; 
 *
 * Author(s): GrandJack
 */

/*
 * This C module implements a simple timer library.
 */

#ifndef TIMERLIB_H
#define TIMERLIB_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Initializes the timer library.
 * Return: 1 in case of success; 0 otherwise.
 */
int timer_init();

/*
 * Tears down any data allocated for the library.
 */
void timer_destroy();
void timer_clear();

/*
 *
 */
int timer_add(long /*sec */ , long /*usec */ , void (* /*hndlr */ )(void *),
	      void * /*hndlr_arg */ );

/*
 * Remove from the queue the timer of identifier "id", if it retured
 * from a previous succesful call to timer_add(). The function
 * "free_arg" is called with the handler argument specified in "timer_add"
 * call. It should be used to free the memory when the timer is destroyed
 * before expiring.
 *
 * Remarks: if "id" is a value not returned by timer_add() nothing happens.
 */
void timer_rem(int /*id */ , void (* /*free_arg */ )(void *));


#ifdef __cplusplus
}
#endif

#endif /*TIMERLIB_H*/
