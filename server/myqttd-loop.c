/* 
 *  MyQtt: A high performance open source MQTT implementation
 *  Copyright (C) 2016 Advanced Software Production Line, S.L.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation version 2.0 of the
 *  License
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 *  02110-1301, USA.
 *  
 *  You may find a copy of the license under this software is released
 *  at COPYING file. This is GPL software 
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
#include <myqttd.h>

/** 
 * \defgroup myqttd_loop MyQttd Loop: socket descriptor watcher
 */

/** 
 * \addtogroup myqttd_loop
 * @{
 */

struct _MyQttdLoop {
	MyQttdCtx      * ctx;
	MyQttThread         thread;
	axlList            * list;
	axlListCursor      * cursor;
	axlPointer           fileset;
	MyQttAsyncQueue   * queue;

	/* read handler */
	MyQttdLoopOnRead on_read;

	/* pointer associated to the descriptor and to be passed to
	   the handler */
	axlPointer           ptr;
	/* second pointer associated to the descriptor and to be
	   passed to the handler */
	axlPointer           ptr2;
};

/** 
 * @internal Type definition used to associate the descriptor to be
 * watched and two user defined pointers to be passed to the on read
 * handler.
 */
typedef struct _MyQttdLoopDescriptor {
	/* descriptor to be watched */
	int                  descriptor;

	/* read handler */
	MyQttdLoopOnRead on_read;

	/* pointer associated to the descriptor and to be passed to
	   the handler */
	axlPointer           ptr;
	/* second pointer associated to the descriptor and to be
	   passed to the handler */
	axlPointer           ptr2;

	/* if set to axl_true, it is a request to remove this
	 * descriptor.
	 */
	axl_bool             remove;
	MyQttAsyncQueue   * queue_reply;
} MyQttdLoopDescriptor;

axl_bool __myqttd_loop_read_first (MyQttdLoop * loop)
{
	MyQttdLoopDescriptor * loop_descriptor;

	loop_descriptor = myqtt_async_queue_pop (loop->queue);

	/* check item received: if null received terminate loop */
	if (PTR_TO_INT (loop_descriptor) == -4)
		return axl_false;

	/* register loop_descriptor on the list */
	axl_list_append (loop->list, loop_descriptor);       

	return axl_true;
}

void __myqttd_loop_discard_broken (MyQttdCtx * ctx, MyQttdLoop * loop)
{
	int  loop_descriptor;
	int  result = -1;
	char bytes[10];

	/* reset cursor */
	axl_list_cursor_first (loop->cursor);
	while (axl_list_cursor_has_item (loop->cursor)) {
		/* get loop descriptor */
		loop_descriptor = PTR_TO_INT (axl_list_cursor_get (loop->cursor));

		/* now add to the waiting socket */
		result = recv (loop_descriptor, bytes, 1, MSG_PEEK);
		if (result == -1 && errno == EBADF) {
			
			/* failed to add descriptor, close it and remove from wait list */
			error ("Discarding descriptor %d because it is broken/invalid (EBADF/%d)", loop_descriptor, errno);

			/* remove cursor */
			axl_list_cursor_remove (loop->cursor);
			continue;
		} /* end if */

		/* get the next item */
		axl_list_cursor_next (loop->cursor);
	} /* end if */

	return;
}

axl_bool __myqttd_loop_read_pending (MyQttdLoop * loop)
{
	MyQttdLoopDescriptor * loop_descriptor, * aux;

	while (axl_true) {
		/* check if there are no pending items */
		if (myqtt_async_queue_items (loop->queue) == 0)
			return axl_true;

		/* get descriptor */
		loop_descriptor = myqtt_async_queue_pop (loop->queue);
			
		/* check item received: if null received terminate loop */
		if (PTR_TO_INT (loop_descriptor) == -4)
			return axl_false;

		/* support for removing loop descriptor */
		if (loop_descriptor->remove) {
			/* reset cursor to the first position */
			axl_list_cursor_first (loop->cursor);
			while (axl_list_cursor_has_item (loop->cursor)) {
				/* get current descriptor */
				aux = axl_list_cursor_get (loop->cursor);
				if (aux && aux->descriptor == loop_descriptor->descriptor) {
					/* element found, remove item */
					axl_list_cursor_remove (loop->cursor);
					break;
				} /* end if */

				/* next cursor */
				axl_list_cursor_next (loop->cursor);
			} /* end if */

			/* notify caller if he is waiting */
			if (loop_descriptor->queue_reply)
				myqtt_async_queue_push (loop_descriptor->queue_reply, INT_TO_PTR (axl_true));

			axl_free (loop_descriptor);
			continue;
		} /* end if */

		/* register loop_descriptor on the list */
		axl_list_append (loop->list, loop_descriptor);       
	} /* end if */

	return axl_true;
}

void __myqttd_loop_descriptor_free (axlPointer __loop_descriptor)
{
	MyQttdLoopDescriptor * loop_descriptor = __loop_descriptor;
	myqtt_close_socket (loop_descriptor->descriptor);
	axl_free (loop_descriptor);
	return;
}

/* build file set to watch */
int __myqttd_loop_build_watch_set (MyQttdLoop * loop)
{
	int                        max_fds = 0;
	MyQttdLoopDescriptor * loop_descriptor;

	/* reset descriptor set */
	__myqtt_io_waiting_default_clear (loop->fileset);
	
	/* reset cursor */
	axl_list_cursor_first (loop->cursor);
	while (axl_list_cursor_has_item (loop->cursor)) {
		/* get loop descriptor */
		loop_descriptor = axl_list_cursor_get (loop->cursor);

		/* now add to the waiting socket */
		if (! __myqtt_io_waiting_default_add_to (loop_descriptor->descriptor, 
							  NULL, 
							  loop->fileset)) {
			
			/* failed to add descriptor, close it and remove from wait list */
			axl_list_cursor_remove (loop->cursor);
			continue;
		} /* end if */

		/* compute max_fds */
		max_fds    = (loop_descriptor->descriptor > max_fds) ? loop_descriptor->descriptor: max_fds;
		
		/* get the next item */
		axl_list_cursor_next (loop->cursor);
	} /* end if */

	return max_fds;
}

void myqttd_loop_handle_descriptors (MyQttdLoop * loop)
{
	MyQttdLoopDescriptor * loop_descriptor;
	MyQttdLoopOnRead       read_handler;
	axlPointer                 ptr;
	axlPointer                 ptr2;

	/* reset cursor */
	axl_list_cursor_first (loop->cursor);
	while (axl_list_cursor_has_item (loop->cursor)) {
		/* get loop descriptor */
		loop_descriptor = axl_list_cursor_get (loop->cursor);

		/* check if the loop descriptor is set */
		if (__myqtt_io_waiting_default_is_set (loop_descriptor->descriptor, loop->fileset, NULL)) {
			/* reset handlers and user pointers */
			read_handler = NULL;
			ptr          = NULL;
			ptr2         = NULL;

			/* configure the read handler to be used. If
			   it is defined the default handler use it */
			if (loop->on_read != NULL) {
				read_handler = loop->on_read;
				ptr          = loop->ptr;
				ptr2         = loop->ptr2;
			}
			/* in the case a particular on read handler is
			   defined, use it instead of default one */
			if (loop_descriptor->on_read != NULL) {
				read_handler = loop_descriptor->on_read;
				ptr          = loop_descriptor->ptr;
				ptr2         = loop_descriptor->ptr2;
			}

			/* call to notify descriptor (if no handler close descriptor to avoid infinite loops) */
			if (read_handler == NULL || 
			    (! read_handler (loop, loop->ctx, loop_descriptor->descriptor, ptr, ptr2))) {
				/* function returned axl_false, remove
				   descriptor from watch set */
				axl_list_cursor_remove (loop->cursor);
				continue;
			} /* end if */

		} /* end if */
		
		/* get the next item */
		axl_list_cursor_next (loop->cursor);
	} /* end if */

	return;
}


axlPointer __myqttd_loop_run (MyQttdLoop * loop)
{
	int                       max_fds;
	int                       result;
#if ! defined(SHOW_FORMAT_BUGS)
	MyQttdCtx           * ctx = loop->ctx;
#endif

	/* init here list, its cursor, the fileset to watch fd for
	 * changes and a queue to receive new registrations */
	loop->list    = axl_list_new (axl_list_always_return_1, __myqttd_loop_descriptor_free);
	loop->cursor  = axl_list_cursor_new (loop->list);

	/* force to use always default select(2) based implementation */
	loop->fileset  = __myqtt_io_waiting_default_create (myqttd_ctx_get_myqtt_ctx (loop->ctx), READ_OPERATIONS);
	
	/* now loop watching content from the list */
wait_for_first_item:
	if (! __myqttd_loop_read_first (loop))
		return NULL;
	
	while (axl_true) {
		/* build file set to watch */
		max_fds = __myqttd_loop_build_watch_set (loop);

		/* check if no descriptor must be watch */
		if (axl_list_length (loop->list) == 0) {
			msg ("no more loop descriptors found to be watched, putting thread to sleep");
			goto wait_for_first_item;
		} /* end if */
		
		/* perform IO wait operation */
		result = __myqtt_io_waiting_default_wait_on (loop->fileset, max_fds, READ_OPERATIONS);
		
		/* check for timeout and errors */
		if (result == -1 || result == -2) {
			if (errno == EBADF) {
				error ("error received from wait on operation, result=%d, errno=%d (discarding broken descriptors)", result, errno);
				__myqttd_loop_discard_broken (ctx, loop);
			} /* end if */

			goto process_pending;
		}
		if (result == -3) {
			error ("fatal error received from io-wait function, finishing myqttd loop manager..");
			return NULL;
		} /* end if */

		/* transfer content found */
		if (result > 0) {
			/* call handlers */
			myqttd_loop_handle_descriptors (loop);
		}

	process_pending:
		/* check for pending descriptors and stop the loop if
		 * found a signal for this */
		if (! __myqttd_loop_read_pending (loop))
			return NULL;
	} /* end if */
	

	return NULL;
}


/** 
 * @brief Creates a new loop instance (starting a new independent
 * thread) used to watch a list of file descriptors (usually sockets). 
 */
MyQttdLoop * myqttd_loop_create (MyQttdCtx * ctx)
{
	MyQttdLoop * loop;

	/* create loop instance */
	loop              = axl_new (MyQttdLoop, 1);
	loop->ctx         = ctx;
	loop->queue       = myqtt_async_queue_new ();

	/* crear manager */
	if (! myqtt_thread_create (&loop->thread, 
				    (MyQttThreadFunc) __myqttd_loop_run,
				    loop,
				    MYQTT_THREAD_CONF_END)) {
		axl_free (loop);
		error ("unable to start loop manager, checking clean start..");
		return NULL;
	} /* end if */

	/* return loop created */
	return loop;
}

/** 
 * @brief Allows to get the \ref MyQttdCtx associated to the provided \ref MyQttdLoop
 *
 * @param loop The loop where the context is being queried.
 *
 * @return A reference to the context or NULL if it fails.
 */
MyQttdCtx  * myqttd_loop_ctx    (MyQttdLoop * loop)
{
	if (loop == NULL)
		return NULL;

	/* report current ctx associated */
	return loop->ctx;
}

/** 
 * @brief Allows to configure the default read handler (\ref
 * MyQttdLoopOnRead) to be used by all notifications for each
 * descriptor not having its own particular on read handler.
 *
 * @param loop The loop to be configured with an on read handler.
 *
 * @param on_read The on read handler to be executed.
 *
 * @param ptr The user defined pointer to be passed to the on read handler.
 *
 * @param ptr2 Second user defined pointer to be passed to the on read handler.
 */
void             myqttd_loop_set_read_handler (MyQttdLoop        * loop,
						   MyQttdLoopOnRead    on_read,
						   axlPointer              ptr,
						   axlPointer              ptr2)
{
	v_return_if_fail (loop);

	loop->on_read = on_read;
	loop->ptr     = ptr;
	loop->ptr2    = ptr2;
	
	return;
}

/** 
 * @brief Allows to configure a descriptor to be watched, providing
 * optionally an on read handler (\ref MyQttdLoopOnRead) to be
 * used only to notify on read status on the provided descriptor. In
 * the case no on read handler is provided, the default handler
 * configured at \ref myqttd_loop_set_read_handler is used.
 *
 * @param loop The loop to be configured with an on read handler.
 *
 * @param descriptor The file descriptor to be watched.
 *
 * @param on_read The on read handler to be executed.
 *
 * @param ptr The user defined pointer to be passed to the on read handler.
 *
 * @param ptr2 Second user defined pointer to be passed to the on read handler.
 */
void             myqttd_loop_watch_descriptor (MyQttdLoop        * loop,
						   int                     descriptor,
						   MyQttdLoopOnRead    on_read,
						   axlPointer              ptr,
						   axlPointer              ptr2)
{
	MyQttdLoopDescriptor * loop_descriptor;

	v_return_if_fail (loop);

	/* build loop descriptor */
	loop_descriptor = axl_new (MyQttdLoopDescriptor, 1);
	
	/* configure internal data */
	loop_descriptor->descriptor = descriptor; 
	loop_descriptor->on_read    = on_read;
	loop_descriptor->ptr        = ptr;
	loop_descriptor->ptr2       = ptr2;

	/* notify loop_descriptor */
	myqtt_async_queue_push (loop->queue, loop_descriptor);

	return;
}

/** 
 * @brief Allows to unwatch the provided descriptor from the provided
 * loop.
 *
 * @param loop The loop where the descriptor will be unwatched (if found).
 *
 * @param descriptor The descriptor to be unwatched from the list.
 *
 * @param wait_until_unwatched If axl_true, the caller will be blocked
 * until the descriptor is removed from the waiting list, otherwise,
 * the unwatch operation will progress without blocking.
 *
 */
void             myqttd_loop_unwatch_descriptor (MyQttdLoop        * loop,
						     int                     descriptor,
						     axl_bool                wait_until_unwatched)
{
	MyQttdLoopDescriptor * loop_descriptor;
	MyQttAsyncQueue      * queue = NULL;

	v_return_if_fail (loop);

	/* build loop descriptor */
	loop_descriptor = axl_new (MyQttdLoopDescriptor, 1);
	
	/* configure descriptor and flag we want it removed */
	loop_descriptor->descriptor = descriptor; 
	loop_descriptor->remove     = axl_true;

	/* user requested to wait until removed descriptor, so create wait reply */
	if (wait_until_unwatched) {
		queue = myqtt_async_queue_new ();
		loop_descriptor->queue_reply = queue;
	} /* end if */

	/* notify loop_descriptor */
	myqtt_async_queue_push (loop->queue, loop_descriptor);

	if (queue && wait_until_unwatched) {
		/* wait for reply */
		myqtt_async_queue_pop (queue);
		myqtt_async_queue_unref (queue);
	} /* end if */

	return;
}

/** 
 * @brief Allows to get how many descriptors are being watched on the
 * provided loop.
 *
 * @param loop The loop that is queried about the number of descriptor watched.
 *
 * @return The number of descriptors watched. In case of failure 0 is returned.
 */
int              myqttd_loop_watching (MyQttdLoop * loop)
{
	if (loop == NULL)
		return 0;
	/* return the current count */
	return axl_list_length (loop->list);
}

/** 
 * @brief Finishes the provided \ref MyQttdLoop, releasing
 * resources and stopping its resources.
 *
 * @param loop The loop to finish.
 *
 * @param notify In the case the internal thread started by the loop
 * should be notified to be stopped.
 */
void             myqttd_loop_close (MyQttdLoop * loop, axl_bool notify)
{
	if (loop == NULL)
		return;

	/* now finish log manager */
	if (notify && loop->queue != NULL) {
		myqtt_async_queue_push (loop->queue, INT_TO_PTR (-4));
		myqtt_thread_destroy (&loop->thread, axl_false);
	} /* end if */	
	
	axl_list_free (loop->list);
	loop->list = NULL;

	axl_list_cursor_free (loop->cursor);
	loop->cursor = NULL;

	myqtt_async_queue_unref (loop->queue);
	loop->queue = NULL;

	__myqtt_io_waiting_default_destroy (loop->fileset);
	loop->fileset = NULL;

	axl_free (loop);

	return;
}


/** 
 * @}
 */
