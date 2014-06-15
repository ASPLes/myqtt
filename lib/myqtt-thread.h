/* 
 *  MyQtt: A high performance open source MQTT implementation
 *  Copyright (C) 2014 Advanced Software Production Line, S.L.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation; either version 2.1
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this program; if not, write to the Free
 *  Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307 USA
 *  
 *  You may find a copy of the license under this software is released
 *  at COPYING file. This is LGPL software: you are welcome to develop
 *  proprietary applications using this library without any royalty or
 *  fee but returning back any change, improvement or addition in the
 *  form of source code, project image, documentation patches, etc.
 *
 *  For commercial support on build MQTT enabled solutions contact us:
 *          
 *      Postal address:
 *         Advanced Software Production Line, S.L.
 *         C/ Antonio Suarez Nº 10, 
 *         Edificio Alius A, Despacho 102
 *         Alcalá de Henares 28802 (Madrid)
 *         Spain
 *
 *      Email address:
 *         info@aspl.es - http://www.aspl.es/mqtt
 *                        http://www.aspl.es/myqtt
 */
#ifndef __MYQTT_THREAD_H__
#define __MYQTT_THREAD_H__

#include <myqtt.h>

BEGIN_C_DECLS

/**
 * \addtogroup myqtt_thread
 * @{
 */

axl_bool           myqtt_thread_create   (MyQttThread      * thread_def,
					   MyQttThreadFunc    func,
					   axlPointer          user_data,
					   ...);

axl_bool           myqtt_thread_destroy  (MyQttThread      * thread_def, 
					   axl_bool            free_data);

void               myqtt_thread_set_create (MyQttThreadCreateFunc  create_fn);

void               myqtt_thread_set_destroy(MyQttThreadDestroyFunc destroy_fn);

axl_bool           myqtt_mutex_create    (MyQttMutex       * mutex_def);

axl_bool           myqtt_mutex_destroy   (MyQttMutex       * mutex_def);

void               myqtt_mutex_lock      (MyQttMutex       * mutex_def);

void               myqtt_mutex_unlock    (MyQttMutex       * mutex_def);

axl_bool           myqtt_cond_create     (MyQttCond        * cond);

void               myqtt_cond_signal     (MyQttCond        * cond);

void               myqtt_cond_broadcast  (MyQttCond        * cond);

/** 
 * @brief Useful macro that allows to perform a call to
 * myqtt_cond_wait registering the place where the call was started
 * and ended.
 * 
 * @param c The cond variable to use.
 * @param mutex The mutex variable to use.
 */
#define MYQTT_COND_WAIT(c, mutex) do{\
myqtt_cond_wait (c, mutex);\
}while(0);

axl_bool           myqtt_cond_wait       (MyQttCond        * cond, 
					   MyQttMutex       * mutex);

/** 
 * @brief Useful macro that allows to perform a call to
 * myqtt_cond_timewait registering the place where the call was
 * started and ended. 
 * 
 * @param r Wait result
 * @param c The cond variable to use.
 * @param mutex The mutex variable to use.
 * @param m The amount of microseconds to wait.
 */
#define MYQTT_COND_TIMEDWAIT(r, c, mutex, m) do{\
r = myqtt_cond_timedwait (c, mutex, m);\
}while(0)


axl_bool           myqtt_cond_timedwait  (MyQttCond        * cond, 
					   MyQttMutex       * mutex,
					   long                microseconds);

void               myqtt_cond_destroy    (MyQttCond        * cond);

MyQttAsyncQueue * myqtt_async_queue_new       (void);

axl_bool           myqtt_async_queue_push      (MyQttAsyncQueue * queue,
						 axlPointer         data);

axl_bool           myqtt_async_queue_priority_push  (MyQttAsyncQueue * queue,
						      axlPointer         data);

axl_bool           myqtt_async_queue_unlocked_push  (MyQttAsyncQueue * queue,
						      axlPointer         data);

axlPointer         myqtt_async_queue_pop          (MyQttAsyncQueue * queue);

axlPointer         myqtt_async_queue_unlocked_pop (MyQttAsyncQueue * queue);

axlPointer         myqtt_async_queue_timedpop  (MyQttAsyncQueue * queue,
						 long               microseconds);

int                myqtt_async_queue_length    (MyQttAsyncQueue * queue);

int                myqtt_async_queue_waiters   (MyQttAsyncQueue * queue);

int                myqtt_async_queue_items     (MyQttAsyncQueue * queue);

axl_bool           myqtt_async_queue_ref       (MyQttAsyncQueue * queue);

int                myqtt_async_queue_ref_count (MyQttAsyncQueue * queue);

void               myqtt_async_queue_unref      (MyQttAsyncQueue * queue);

void               myqtt_async_queue_release    (MyQttAsyncQueue * queue);

void               myqtt_async_queue_safe_unref (MyQttAsyncQueue ** queue);

void               myqtt_async_queue_foreach   (MyQttAsyncQueue         * queue,
						 MyQttAsyncQueueForeach    foreach_func,
						 axlPointer                 user_data);

axlPointer         myqtt_async_queue_lookup    (MyQttAsyncQueue         * queue,
						 axlLookupFunc              lookup_func,
						 axlPointer                 user_data);

void               myqtt_async_queue_lock      (MyQttAsyncQueue * queue);

void               myqtt_async_queue_unlock    (MyQttAsyncQueue * queue);

END_C_DECLS

#endif

/**
 * @}
 */ 
