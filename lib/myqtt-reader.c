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
#include <myqtt.h>

/* local/private includes */
#include <myqtt_ctx_private.h>
#include <myqtt_connection_private.h>

#define LOG_DOMAIN "myqtt-reader"

/**
 * \defgroup myqtt_reader MyQtt Reader: The module that reads you frames. 
 */

/**
 * \addtogroup myqtt_reader
 * @{
 */

typedef enum {CONNECTION, 
	      LISTENER, 
	      TERMINATE, 
	      IO_WAIT_CHANGED,
	      IO_WAIT_READY,
	      FOREACH
} WatchType;

typedef struct _MyQttReaderData {
	WatchType            type;
	MyQttConnection   * connection;
	MyQttForeachFunc    func;
	axlPointer           user_data;
	/* queue used to notify that the foreach operation was
	 * finished: currently only used for type == FOREACH */
	MyQttAsyncQueue   * notify;
}MyQttReaderData;

/** 
 * @internal
 * 
 * The main purpose of this function is to dispatch received frames
 * into the appropriate channel. It also makes all checks to ensure the
 * frame receive have all indicators (seqno, channel, message number,
 * payload size correctness,..) to ensure the channel receive correct
 * frames and filter those ones which have something wrong.
 *
 * This function also manage frame fragment joining. There are two
 * levels of frame fragment managed by the myqtt reader.
 * 
 * We call the first level of fragment, the one described at RFC3080,
 * as the complete frame which belongs to a group of frames which
 * conform a message which was splitted due to channel window size
 * restrictions.
 *
 * The second level of fragment happens when the myqtt reader receive
 * a frame header which describes a frame size payload to receive but
 * not all payload was actually received. This can happen because
 * myqtt uses non-blocking socket configuration so it can avoid DOS
 * attack. But this behavior introduce the asynchronous problem of
 * reading at the moment where the whole frame was not received.  We
 * call to this internal frame fragmentation. It is also supported
 * without blocking to myqtt reader.
 *
 * While reading this function, you have to think about it as a
 * function which is executed for only one frame, received inside only
 * one channel for the given connection.
 *
 * @param connection the connection which have something to be read
 * 
 **/
void __myqtt_reader_process_socket (MyQttCtx        * ctx, 
				     MyQttConnection * connection)
{

	MyQttFrame      * frame;
	MyQttFrame      * previous;
	MyQttFrameType    type;
	MyQttChannel    * channel;
	axl_bool           more;
#if defined(ENABLE_MYQTT_LOG)
	char             * raw_frame;
	int                frame_id;
#endif

	myqtt_log (MYQTT_LEVEL_DEBUG, "something to read conn-id=%d", myqtt_connection_get_id (connection));

	/* check if there are pre read handler to be executed on this 
	   connection. */
	if (myqtt_connection_is_defined_preread_handler (connection)) {
		/* if defined preread handler invoke it and return. */
		myqtt_connection_invoke_preread_handler (connection);
		return;
	} /* end if */

	/* before doing anything, check if the connection is broken */
	if (! myqtt_connection_is_ok (connection, axl_false))
		return;

	/* check for unwatch requests */
	if (connection->reader_unwatch)
		return;

	/* read all frames received from remote site */
	frame   = myqtt_frame_get_next (connection);
	if (frame == NULL) 
		return;

	/* get frame type to avoid calling everytime to
	 * myqtt_frame_get_type */
	type    = myqtt_frame_get_type (frame);

	/* NOTE: After this point, frame received is
	 * complete. myqtt_frame_get_next function takes cares about
	 * joining frame fragments. */

	/* check for debug to throw some debug messages.*/
#if defined(ENABLE_MYQTT_LOG)
	/* get frame id */ 
	frame_id = myqtt_frame_get_id (frame);

	if (myqtt_log2_is_enabled (ctx)) {
		raw_frame = myqtt_frame_get_raw_frame (frame);
		myqtt_log (MYQTT_LEVEL_DEBUG, "frame id=%d received (before all filters)\n%s",
			    frame_id, raw_frame);
		axl_free (raw_frame);
	}
#endif
	
	/* if not, try to deliver to first level. If second level
	 * invocation was ok, the frame have been freed. If not, the
	 * first level will do this */
	if (myqtt_profiles_invoke_frame_received (myqtt_channel_get_profile    (channel),
						   myqtt_channel_get_number     (channel),
						   myqtt_channel_get_connection (channel),
						   frame)) {
		myqtt_log (MYQTT_LEVEL_DEBUG, "frame id=%d delivered on first (profile) level handler channel",
			    frame_id);
		return; /* frame was successfully delivered */
	}
	
	myqtt_log (MYQTT_LEVEL_WARNING, 
		    "unable to deliver incoming frame id=%d, no first or second level handler defined, dropping frame",
		    frame_id);

	/* unable to deliver the frame, free it */
	myqtt_frame_unref (frame);

	/* that's all I can do */
	return;
}

/** 
 * @internal 
 *
 * @brief Classify myqtt reader items to be managed, that is,
 * connections or listeners.
 * 
 * @param data The internal myqtt reader data to be managed.
 * 
 * @return axl_true if the item to be managed was clearly read or axl_false if
 * an error on registering the item was produced.
 */
axl_bool   myqtt_reader_register_watch (MyQttReaderData * data, axlList * con_list, axlList * srv_list)
{
	MyQttConnection * connection;
#if defined(ENABLE_MYQTT_LOG)
	MyQttCtx        * ctx;
#endif

	/* get a reference to the connection (no matter if it is not
	 * defined) */
	connection = data->connection;
#if defined(ENABLE_MYQTT_LOG)
	ctx        = myqtt_connection_get_ctx (connection);
#endif

	switch (data->type) {
	case CONNECTION:
		/* check the connection */
		if (!myqtt_connection_is_ok (connection, axl_false)) {
			/* check if we can free this connection */
			myqtt_connection_unref (connection, "myqtt reader (watch)");
			myqtt_log (MYQTT_LEVEL_DEBUG, "received a non-valid connection, ignoring it");

			/* release data */
			axl_free (data);
			return axl_false;
		}
			
		/* now we have a first connection, we can start to wait */
		myqtt_log (MYQTT_LEVEL_DEBUG, "new connection (conn-id=%d) to be watched (%d)", 
			    myqtt_connection_get_id (connection), myqtt_connection_get_socket (connection));
		axl_list_append (con_list, connection);

		break;
	case LISTENER:
		myqtt_log (MYQTT_LEVEL_DEBUG, "new listener connection to be watched (socket: %d --> %s:%s, conn-id: %d)",
			    myqtt_connection_get_socket (connection), 
			    myqtt_connection_get_host (connection), 
			    myqtt_connection_get_port (connection),
			    myqtt_connection_get_id (connection));
		axl_list_append (srv_list, connection);
		break;
	case TERMINATE:
	case IO_WAIT_CHANGED:
	case IO_WAIT_READY:
	case FOREACH:
		/* just unref myqtt reader data */
		break;
	} /* end switch */
	
	axl_free (data);
	return axl_true;
}

/** 
 * @internal MyQtt function to implement myqtt reader I/O change.
 */
MyQttReaderData * __myqtt_reader_change_io_mech (MyQttCtx        * ctx,
						   axlPointer       * on_reading, 
						   axlList          * con_list, 
						   axlList          * srv_list, 
						   MyQttReaderData * data)
{
	/* get current context */
	MyQttReaderData * result;

	myqtt_log (MYQTT_LEVEL_DEBUG, "found I/O notification change");
	
	/* unref IO waiting object */
	myqtt_io_waiting_invoke_destroy_fd_group (ctx, *on_reading); 
	*on_reading = NULL;
	
	/* notify preparation done and lock until new
	 * I/O is installed */
	myqtt_log (MYQTT_LEVEL_DEBUG, "notify myqtt reader preparation done");
	myqtt_async_queue_push (ctx->reader_stopped, INT_TO_PTR(1));
	
	/* free data use the function that includes that knoledge */
	myqtt_reader_register_watch (data, con_list, srv_list);
	
	/* lock */
	myqtt_log (MYQTT_LEVEL_DEBUG, "lock until new API is installed");
	result = myqtt_async_queue_pop (ctx->reader_queue);

	/* initialize the read set */
	myqtt_log (MYQTT_LEVEL_DEBUG, "unlocked, creating new I/O mechanism used current API");
	*on_reading = myqtt_io_waiting_invoke_create_fd_group (ctx, READ_OPERATIONS);

	return result;
}


/* do a foreach operation */
void myqtt_reader_foreach_impl (MyQttCtx        * ctx, 
				 axlList          * con_list, 
				 axlList          * srv_list, 
				 MyQttReaderData * data)
{
	axlListCursor * cursor;

	myqtt_log (MYQTT_LEVEL_DEBUG, "doing myqtt reader foreach notification..");

	/* check for null function */
	if (data->func == NULL) 
		goto foreach_impl_notify;

	/* foreach the connection list */
	cursor = axl_list_cursor_new (con_list);
	while (axl_list_cursor_has_item (cursor)) {

		/* notify, if the connection is ok */
		if (myqtt_connection_is_ok (axl_list_cursor_get (cursor), axl_false)) {
			data->func (axl_list_cursor_get (cursor), data->user_data);
		} /* end if */

		/* next cursor */
		axl_list_cursor_next (cursor);
	} /* end while */
	
	/* free cursor */
	axl_list_cursor_free (cursor);

	/* foreach the connection list */
	cursor = axl_list_cursor_new (srv_list);
	while (axl_list_cursor_has_item (cursor)) {
		/* notify, if the connection is ok */
		if (myqtt_connection_is_ok (axl_list_cursor_get (cursor), axl_false)) {
			data->func (axl_list_cursor_get (cursor), data->user_data);
		} /* end if */

		/* next cursor */
		axl_list_cursor_next (cursor);
	} /* end while */

	/* free cursor */
	axl_list_cursor_free (cursor);

	/* notify that the foreach operation was completed */
 foreach_impl_notify:
	myqtt_async_queue_push (data->notify, INT_TO_PTR (1));

	return;
}

/**
 * @internal Function that checks if there are a global frame received
 * handler configured on the context provided and, in the case it is
 * defined, the frame is delivered to that handler.
 *
 * @param ctx The context to check for a global frame received.
 *
 * @param connection The connection where the frame was received.
 *
 * @param channel The channel where the frame was received.
 *
 * @param frame The frame that was received.
 *
 * @return axl_true in the case the global frame received handler is
 * defined and the frame was delivered on it.
 */
axl_bool  myqtt_reader_invoke_frame_received       (MyQttCtx        * ctx,
						    MyQttConnection * connection,
						    MyQttChannel    * channel,
						    MyQttFrame      * frame)
{
	/* check the reference and the handler */
	if (ctx == NULL || ctx->global_frame_received == NULL)
		return axl_false;
	
	/* no thread activation */
	ctx->global_frame_received (channel, connection, frame, ctx->global_frame_received_data);
	
	/* unref the frame here */
	myqtt_frame_unref (frame);

	/* return delivered */
	return axl_true;
}

/** 
 * @internal
 * @brief Read the next item on the myqtt reader to be processed
 * 
 * Once an item is read, it is check if something went wrong, in such
 * case the loop keeps on going.
 * 
 * The function also checks for terminating myqtt reader loop by
 * looking for TERMINATE value into the data->type. In such case axl_false
 * is returned meaning that no further loop should be done by the
 * myqtt reader.
 *
 * @return axl_true to keep myqtt reader working, axl_false if myqtt reader
 * should stop.
 */
axl_bool      myqtt_reader_read_queue (MyQttCtx  * ctx,
					axlList    * con_list, 
					axlList    * srv_list, 
					axlPointer * on_reading)
{
	/* get current context */
	MyQttReaderData * data;
	int                should_continue;

	do {
		data            = myqtt_async_queue_pop (ctx->reader_queue);

		/* check if we have to continue working */
		should_continue = (data->type != TERMINATE);

		/* check if the io/wait mech have changed */
		if (data->type == IO_WAIT_CHANGED) {
			/* change io mechanism */
			data = __myqtt_reader_change_io_mech (ctx,
							       on_reading, 
							       con_list, 
							       srv_list, 
							       data);
		} else if (data->type == FOREACH) {
			/* do a foreach operation */
			myqtt_reader_foreach_impl (ctx, con_list, srv_list, data);

		} /* end if */

	}while (!myqtt_reader_register_watch (data, con_list, srv_list));

	return should_continue;
}

/** 
 * @internal Function used by the myqtt reader main loop to check for
 * more connections to watch, to check if it has to terminate or to
 * check at run time the I/O waiting mechanism used.
 * 
 * @param con_list The set of connections already watched.
 *
 * @param srv_list The set of listener connections already watched.
 *
 * @param on_reading A reference to the I/O waiting object, in the
 * case the I/O waiting mechanism is changed.
 * 
 * @return axl_true to flag the process to continue working to to stop.
 */
axl_bool      myqtt_reader_read_pending (MyQttCtx  * ctx,
					  axlList    * con_list, 
					  axlList    * srv_list, 
					  axlPointer * on_reading)
{
	/* get current context */
	MyQttReaderData * data;
	int                length;
	axl_bool           should_continue = axl_true;

	length = myqtt_async_queue_length (ctx->reader_queue);
	while (length > 0) {
		length--;
		data            = myqtt_async_queue_pop (ctx->reader_queue);

		myqtt_log (MYQTT_LEVEL_DEBUG, "read pending type=%d",
			    data->type);

		/* check if we have to continue working */
		should_continue = (data->type != TERMINATE);

		/* check if the io/wait mech have changed */
		if (data->type == IO_WAIT_CHANGED) {
			/* change io mechanism */
			data = __myqtt_reader_change_io_mech (ctx, on_reading, con_list, srv_list, data);

		} else if (data->type == FOREACH) {
			/* do a foreach operation */
			myqtt_reader_foreach_impl (ctx, con_list, srv_list, data);

		} /* end if */

		/* watch the request received, maybe a connection or a
		 * myqtt reader command to process  */
		myqtt_reader_register_watch (data, con_list, srv_list);
		
	} /* end while */

	return should_continue;
}

/** 
 * @internal Auxiliar function that populates the reading set of file
 * descriptors (on_reading), returning the max fds.
 */
MYQTT_SOCKET __myqtt_reader_build_set_to_watch_aux (MyQttCtx     * ctx,
						      axlPointer      on_reading, 
						      axlListCursor * cursor, 
						      MYQTT_SOCKET   current_max)
{
	MYQTT_SOCKET      max_fds     = current_max;
	MYQTT_SOCKET      fds         = 0;
	MyQttConnection * connection;
	long               time_stamp  = 0;

	/* get current time stamp if idle handler is defined */
	if (ctx->global_idle_handler)
		time_stamp = (long) time (NULL);
	
	axl_list_cursor_first (cursor);
	while (axl_list_cursor_has_item (cursor)) {

		/* get current connection */
		connection = axl_list_cursor_get (cursor);

		/* check for idle status */
		if (ctx->global_idle_handler)
			myqtt_connection_check_idle_status (connection, ctx, time_stamp);

		/* check ok status */
		if (! myqtt_connection_is_ok (connection, axl_false)) {

			/* FIRST: remove current cursor to ensure the
			 * connection is out of our handling before
			 * finishing the reference the reader owns */
			axl_list_cursor_unlink (cursor);

			/* connection isn't ok, unref it */
			myqtt_connection_unref (connection, "myqtt reader (build set)");

			continue;
		} /* end if */

		/* check if the connection must be unwatched */
		if (connection->reader_unwatch) {
			/* remove the unwatch flag from the connection */
			connection->reader_unwatch = axl_false;

			/* FIRST: remove current cursor to ensure the
			 * connection is out of our handling before
			 * finishing the reference the reader owns */
			axl_list_cursor_unlink (cursor);

			/* connection isn't ok, unref it */
			myqtt_connection_unref (connection, "myqtt reader (process: unwatch)");

			continue;
		} /* end if */

		/* check if the connection is blocked (no I/O read to
		 * perform on it) */
		if (myqtt_connection_is_blocked (connection)) {
			/* myqtt_log (MYQTT_LEVEL_DEBUG, "connection id=%d has I/O read blocked (myqtt_connection_block)", 
			   myqtt_connection_get_id (connection)); */
			/* get the next */
			axl_list_cursor_next (cursor);
			continue;
		} /* end if */

		/* get the socket to ge added and get its maximum
		 * value */
		fds        = myqtt_connection_get_socket (connection);
		max_fds    = fds > max_fds ? fds: max_fds;

		/* add the socket descriptor into the given on reading
		 * group */
		if (! myqtt_io_waiting_invoke_add_to_fd_group (ctx, fds, connection, on_reading)) {
			
			myqtt_log (MYQTT_LEVEL_WARNING, 
				    "unable to add the connection to the myqtt reader watching set. This could mean you did reach the I/O waiting mechanism limit.");

			/* FIRST: remove current cursor to ensure the
			 * connection is out of our handling before
			 * finishing the reference the reader owns */
			axl_list_cursor_unlink (cursor);

			/* set it as not connected */
			if (myqtt_connection_is_ok (connection, axl_false))
				__myqtt_connection_shutdown_and_record_error (connection, MyQttError, "myqtt reader (add fail)");
			myqtt_connection_unref (connection, "myqtt reader (add fail)");

			continue;
		} /* end if */

		/* get the next */
		axl_list_cursor_next (cursor);

	} /* end while */

	/* return maximum number for file descriptors */
	return max_fds;
	
} /* end __myqtt_reader_build_set_to_watch_aux */

MYQTT_SOCKET   __myqtt_reader_build_set_to_watch (MyQttCtx     * ctx,
						    axlPointer      on_reading, 
						    axlListCursor * conn_cursor, 
						    axlListCursor * srv_cursor)
{

	MYQTT_SOCKET       max_fds     = 0;

	/* read server connections */
	max_fds = __myqtt_reader_build_set_to_watch_aux (ctx, on_reading, srv_cursor, max_fds);

	/* read client connection list */
	max_fds = __myqtt_reader_build_set_to_watch_aux (ctx, on_reading, conn_cursor, max_fds);

	/* return maximum number for file descriptors */
	return max_fds;
	
}

void __myqtt_reader_check_connection_list (MyQttCtx     * ctx,
					    axlPointer      on_reading, 
					    axlListCursor * conn_cursor, 
					    int             changed)
{

	MYQTT_SOCKET       fds        = 0;
	MyQttConnection  * connection = NULL;
	int                 checked    = 0;

	/* check all connections */
	axl_list_cursor_first (conn_cursor);
	while (axl_list_cursor_has_item (conn_cursor)) {

		/* check changed */
		if (changed == checked)
			return;

		/* check if we have to keep on listening on this
		 * connection */
		connection = axl_list_cursor_get (conn_cursor);
		if (!myqtt_connection_is_ok (connection, axl_false)) {
			/* FIRST: remove current cursor to ensure the
			 * connection is out of our handling before
			 * finishing the reference the reader owns */
			axl_list_cursor_unlink (conn_cursor);

			/* connection isn't ok, unref it */
			myqtt_connection_unref (connection, "myqtt reader (check list)");
			continue;
		}
		
		/* get the connection and socket. */
	        fds = myqtt_connection_get_socket (connection);
		
		/* ask if this socket have changed */
		if (myqtt_io_waiting_invoke_is_set_fd_group (ctx, fds, on_reading, ctx)) {

			/* call to process incoming data, activating
			 * all invocation code (first and second level
			 * handler) */
			__myqtt_reader_process_socket (ctx, connection);

			/* update number of sockets checked */
			checked++;
		}

		/* get the next */
		axl_list_cursor_next (conn_cursor);

	} /* end for */

	return;
}

int  __myqtt_reader_check_listener_list (MyQttCtx     * ctx, 
					  axlPointer      on_reading, 
					  axlListCursor * srv_cursor, 
					  int             changed)
{

	int                fds      = 0;
	int                checked  = 0;
	MyQttConnection * connection;

	/* check all listeners */
	axl_list_cursor_first (srv_cursor);
	while (axl_list_cursor_has_item (srv_cursor)) {

		/* get the connection */
		connection = axl_list_cursor_get (srv_cursor);

		if (!myqtt_connection_is_ok (connection, axl_false)) {
			myqtt_log (MYQTT_LEVEL_DEBUG, "myqtt reader found listener id=%d not operational, unreference",
				    myqtt_connection_get_id (connection));

			/* FIRST: remove current cursor to ensure the
			 * connection is out of our handling before
			 * finishing the reference the reader owns */
			axl_list_cursor_unlink (srv_cursor);

			/* connection isn't ok, unref it */
			myqtt_connection_unref (connection, "myqtt reader (process), listener closed");

			/* update checked connections */
			checked++;

			continue;
		} /* end if */
		
		/* get the connection and socket. */
		fds  = myqtt_connection_get_socket (connection);

		/* check if the socket is activated */
		if (myqtt_io_waiting_invoke_is_set_fd_group (ctx, fds, on_reading, ctx)) {
			/* init the listener incoming connection phase */
			myqtt_log (MYQTT_LEVEL_DEBUG, "listener (%d) have requests, processing..", fds);
			myqtt_listener_accept_connections (ctx, fds, connection);

			/* update checked connections */
			checked++;
		} /* end if */

		/* check to stop listener */
		if (checked == changed)
			return 0;

		/* get the next */
		axl_list_cursor_next (srv_cursor);
	}
	
	/* return remaining sockets active */
	return changed - checked;
}

/** 
 * @internal
 *
 * @brief Internal function called to stop myqtt reader and cleanup
 * memory used.
 * 
 */
void __myqtt_reader_stop_process (MyQttCtx     * ctx,
				   axlPointer      on_reading, 
				   axlListCursor * conn_cursor, 
				   axlListCursor * srv_cursor)

{
	/* stop myqtt reader process unreferring already managed
	 * connections */

	myqtt_async_queue_unref (ctx->reader_queue);

	/* unref listener connections */
	myqtt_log (MYQTT_LEVEL_DEBUG, "cleaning pending %d listener connections..", axl_list_length (ctx->srv_list));
	ctx->srv_list = NULL;
	axl_list_free (axl_list_cursor_list (srv_cursor));
	axl_list_cursor_free (srv_cursor);

	/* unref initiators connections */
	myqtt_log (MYQTT_LEVEL_DEBUG, "cleaning pending %d peer connections..", axl_list_length (ctx->conn_list));
	ctx->conn_list = NULL;
	axl_list_free (axl_list_cursor_list (conn_cursor));
	axl_list_cursor_free (conn_cursor);

	/* unref IO waiting object */
	myqtt_io_waiting_invoke_destroy_fd_group (ctx, on_reading); 

	/* signal that the myqtt reader process is stopped */
	QUEUE_PUSH (ctx->reader_stopped, INT_TO_PTR (1));

	return;
}

void __myqtt_reader_close_connection (axlPointer pointer)
{
	MyQttConnection * conn = pointer;

	/* unref the connection */
	myqtt_connection_shutdown (conn);
	myqtt_connection_unref (conn, "myqtt reader");

	return;
}

/** 
 * @internal Dispatch function used to process all sockets that have
 * changed.
 * 
 * @param fds The socket that have changed.
 * @param wait_to The purpose that was configured for the file set.
 * @param connection The connection that is notified for changes.
 */
void __myqtt_reader_dispatch_connection (int                  fds,
					  MyQttIoWaitingFor   wait_to,
					  MyQttConnection   * connection,
					  axlPointer           user_data)
{
	/* cast the reference */
	MyQttCtx * ctx = user_data;

	switch (myqtt_connection_get_role (connection)) {
	case MyQttRoleMasterListener:
		/* check if there are pre read handler to be executed on this 
		   connection. */
		if (myqtt_connection_is_defined_preread_handler (connection)) {
			/* if defined preread handler invoke it and return. */
			myqtt_connection_invoke_preread_handler (connection);
			return;
		} /* end if */

		/* listener connections */
		myqtt_listener_accept_connections (ctx, fds, connection);
		break;
	default:
		/* call to process incoming data, activating all
		 * invocation code (first and second level handler) */
		__myqtt_reader_process_socket (ctx, connection);
		break;
	} /* end if */
	return;
}

axl_bool __myqtt_reader_detect_and_cleanup_connection (axlListCursor * cursor) 
{
	MyQttConnection * conn;
	char               bytes[3];
	int                result;
	int                fds;
#if defined(ENABLE_MYQTT_LOG)
	MyQttCtx        * ctx;
#endif
	
	/* get connection from cursor */
	conn = axl_list_cursor_get (cursor);
	if (myqtt_connection_is_ok (conn, axl_false)) {

		/* get the connection and socket. */
		fds    = myqtt_connection_get_socket (conn);
#if defined(AXL_OS_UNIX)
		errno  = 0;
#endif
		result = recv (fds, bytes, 1, MSG_PEEK);
		if (result == -1 && errno == EBADF) {
			  
			/* get context */
#if defined(ENABLE_MYQTT_LOG)
			ctx = CONN_CTX (conn);
#endif
			myqtt_log (MYQTT_LEVEL_CRITICAL, "Found connection-id=%d, with session=%d not working (errno=%d), shutting down",
				    myqtt_connection_get_id (conn), fds, errno);
			/* close connection, but remove the socket reference to avoid closing some's socket */
			conn->session = -1;
			myqtt_connection_shutdown (conn);
			
			/* connection isn't ok, unref it */
			myqtt_connection_unref (conn, "myqtt reader (process), wrong socket");
			axl_list_cursor_unlink (cursor);
			return axl_false;
		} /* end if */
	} /* end if */
	
	return axl_true;
}

void __myqtt_reader_detect_and_cleanup_connections (MyQttCtx * ctx)
{
	/* check all listeners */
	axl_list_cursor_first (ctx->conn_cursor);
	while (axl_list_cursor_has_item (ctx->conn_cursor)) {

		/* get the connection */
		if (! __myqtt_reader_detect_and_cleanup_connection (ctx->conn_cursor))
			continue;

		/* get the next */
		axl_list_cursor_next (ctx->conn_cursor);
	} /* end while */

	/* check all listeners */
	axl_list_cursor_first (ctx->srv_cursor);
	while (axl_list_cursor_has_item (ctx->srv_cursor)) {

	  /* get the connection */
	  if (! __myqtt_reader_detect_and_cleanup_connection (ctx->srv_cursor))
		   continue; 

	    /* get the next */
	    axl_list_cursor_next (ctx->srv_cursor); 
	} /* end while */

	/* clear errno after cleaning descriptors */
#if defined(AXL_OS_UNIX)
	errno = 0;
#endif

	return; 
}

axlPointer __myqtt_reader_run (MyQttCtx * ctx)
{
	MYQTT_SOCKET      max_fds     = 0;
	MYQTT_SOCKET      result;
	int                error_tries = 0;

	/* initialize the read set */
	if (ctx->on_reading != NULL)
		myqtt_io_waiting_invoke_destroy_fd_group (ctx, ctx->on_reading);
	ctx->on_reading  = myqtt_io_waiting_invoke_create_fd_group (ctx, READ_OPERATIONS);

	/* create lists */
	ctx->conn_list = axl_list_new (axl_list_always_return_1, __myqtt_reader_close_connection);
	ctx->srv_list = axl_list_new (axl_list_always_return_1, __myqtt_reader_close_connection);

	/* create cursors */
	ctx->conn_cursor = axl_list_cursor_new (ctx->conn_list);
	ctx->srv_cursor = axl_list_cursor_new (ctx->srv_list);

	/* first step. Waiting blocked for our first connection to
	 * listen */
 __myqtt_reader_run_first_connection:
	if (!myqtt_reader_read_queue (ctx, ctx->conn_list, ctx->srv_list, &(ctx->on_reading))) {
		/* seems that the myqtt reader main loop should
		 * stop */
		__myqtt_reader_stop_process (ctx, ctx->on_reading, ctx->conn_cursor, ctx->srv_cursor);
		return NULL;
	}

	while (axl_true) {
		/* reset descriptor set */
		myqtt_io_waiting_invoke_clear_fd_group (ctx, ctx->on_reading);

		if ((axl_list_length (ctx->conn_list) == 0) && (axl_list_length (ctx->srv_list) == 0)) {
			/* check if we have to terminate the process
			 * in the case no more connections are
			 * available: useful when the current instance
			 * is running in the context of turbulence */
			myqtt_ctx_check_on_finish (ctx);

			myqtt_log (MYQTT_LEVEL_DEBUG, "no more connection to watch for, putting thread to sleep");
			goto __myqtt_reader_run_first_connection;
		}

		/* build socket descriptor to be read */
		max_fds = __myqtt_reader_build_set_to_watch (ctx, ctx->on_reading, ctx->conn_cursor, ctx->srv_cursor);
		if (errno == EBADF) {
			myqtt_log (MYQTT_LEVEL_CRITICAL, "Found wrong file descriptor error...(max_fds=%d, errno=%d), cleaning", max_fds, errno);
			/* detect and cleanup wrong connections */
			__myqtt_reader_detect_and_cleanup_connections (ctx);
			continue;
		} /* end if */
		
		/* perform IO blocking wait for read operation */
		result = myqtt_io_waiting_invoke_wait (ctx, ctx->on_reading, max_fds, READ_OPERATIONS);

		/* do automatic thread pool resize here */
		__myqtt_thread_pool_automatic_resize (ctx);  

		/* check for timeout error */
		if (result == -1 || result == -2)
			goto process_pending;

		/* check errors */
		if ((result < 0) && (errno != 0)) {

			error_tries++;
			if (error_tries == 2) {
				myqtt_log (MYQTT_LEVEL_CRITICAL, 
					    "tries have been reached on reader, error was=(errno=%d): %s exiting..",
					    errno, myqtt_errno_get_last_error ());
				return NULL;
			} /* end if */
			continue;
		} /* end if */

		/* check for fatal error */
		if (result == -3) {
			myqtt_log (MYQTT_LEVEL_CRITICAL, "fatal error received from io-wait function, exiting from myqtt reader process..");
			__myqtt_reader_stop_process (ctx, ctx->on_reading, ctx->conn_cursor, ctx->srv_cursor);
			return NULL;
		}


		/* check for each listener */
		if (result > 0) {
			/* check if the mechanism have automatic
			 * dispatch */
			if (myqtt_io_waiting_invoke_have_dispatch (ctx, ctx->on_reading)) {
				/* perform automatic dispatch,
				 * providing the dispatch function and
				 * the number of sockets changed */
				myqtt_io_waiting_invoke_dispatch (ctx, ctx->on_reading, __myqtt_reader_dispatch_connection, result, ctx);

			} else {
				/* call to check listener connections */
				result = __myqtt_reader_check_listener_list (ctx, ctx->on_reading, ctx->srv_cursor, result);
			
				/* check for each connection to be watch is it have check */
				__myqtt_reader_check_connection_list (ctx, ctx->on_reading, ctx->conn_cursor, result);
			} /* end if */
		}

		/* we have finished the connection dispatching, so
		 * read the pending queue elements to be watched */
		
		/* reset error tries */
	process_pending:
		error_tries = 0;

		/* read new connections to be managed */
		if (!myqtt_reader_read_pending (ctx, ctx->conn_list, ctx->srv_list, &(ctx->on_reading))) {
			__myqtt_reader_stop_process (ctx, ctx->on_reading, ctx->conn_cursor, ctx->srv_cursor);
			return NULL;
		}
	}
	return NULL;
}

/** 
 * @brief Function that returns the number of connections that are
 * currently watched by the reader.
 * @param ctx The context where the reader loop is located.
 * @return Number of connections watched. 
 */
int  myqtt_reader_connections_watched         (MyQttCtx        * ctx)
{
	if (ctx == NULL || ctx->conn_list == NULL || ctx->srv_list == NULL)
		return 0;
	
	/* return list */
	return axl_list_length (ctx->conn_list) + axl_list_length (ctx->srv_list);
}

/** 
 * @internal Function used to unwatch the provided connection from the
 * myqtt reader loop.
 *
 * @param ctx The context where the unread operation will take place.
 * @param connection The connection where to be unwatched from the reader...
 */
void myqtt_reader_unwatch_connection          (MyQttCtx        * ctx,
						MyQttConnection * connection)
{
	v_return_if_fail (ctx && connection);
	/* flag connection myqtt reader unwatch */
	connection->reader_unwatch = axl_true;
	return;
}

/** 
 * @internal
 * 
 * Adds a new connection to be watched on myqtt reader process. This
 * function is for internal myqtt library use.
 **/
void myqtt_reader_watch_connection (MyQttCtx        * ctx,
				     MyQttConnection * connection)
{
	/* get current context */
	MyQttReaderData * data;

	v_return_if_fail (myqtt_connection_is_ok (connection, axl_false));
	v_return_if_fail (ctx->reader_queue);

	if (!myqtt_connection_set_nonblocking_socket (connection)) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "unable to set non-blocking I/O operation, at connection registration, closing session");
 		return;
	}

	/* increase reference counting */
	if (! myqtt_connection_ref (connection, "myqtt reader (watch)")) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "unable to increase connection reference count, dropping connection");
		return;
	}

	myqtt_log (MYQTT_LEVEL_DEBUG, "Accepting conn-id=%d into reader queue %p, library status: %d", 
		    myqtt_connection_get_id (connection),
		    ctx->reader_queue,
		    myqtt_is_exiting (ctx));

	/* prepare data to be queued */
	data             = axl_new (MyQttReaderData, 1);
	data->type       = CONNECTION;
	data->connection = connection;

	/* push data */
	QUEUE_PUSH (ctx->reader_queue, data);

	return;
}

/** 
 * @internal
 *
 * Install a new listener to watch for new incoming connections.
 **/
void myqtt_reader_watch_listener   (MyQttCtx        * ctx,
				     MyQttConnection * listener)
{
	/* get current context */
	MyQttReaderData * data;
	v_return_if_fail (listener > 0);
	
	/* prepare data to be queued */
	data             = axl_new (MyQttReaderData, 1);
	data->type       = LISTENER;
	data->connection = listener;

	/* push data */
	QUEUE_PUSH (ctx->reader_queue, data);

	return;
}

/** 
 * @internal Function used to configure the connection so the next
 * call to terminate the list of connections will not close the
 * connection socket.
 */
axl_bool __myqtt_reader_configure_conn (axlPointer ptr, axlPointer data)
{
	/* set the connection socket to be not closed */
	myqtt_connection_set_close_socket ((MyQttConnection *) ptr, axl_false);
	return axl_false; /* not found so all items are iterated */
}

/** 
 * @internal
 * 
 * Creates the reader thread process. It will be waiting for any
 * connection that have changed to read its connect and send it
 * appropriate channel reader.
 * 
 * @return The function returns axl_true if the myqtt reader was started
 * properly, otherwise axl_false is returned.
 **/
axl_bool  myqtt_reader_run (MyQttCtx * ctx) 
{
	v_return_val_if_fail (ctx, axl_false);

	/* check connection list to be previously created to terminate
	   it without closing sockets associated to each connection */
	if (ctx->conn_list != NULL) {
		myqtt_log (MYQTT_LEVEL_DEBUG, "releasing previous client connections, installed: %d",
			    axl_list_length (ctx->conn_list));
		ctx->reader_cleanup = axl_true;
		axl_list_lookup (ctx->conn_list, __myqtt_reader_configure_conn, NULL);
		axl_list_cursor_free (ctx->conn_cursor);
		axl_list_free (ctx->conn_list);
		ctx->conn_list   = NULL;
		ctx->conn_cursor = NULL;
	} /* end if */
	if (ctx->srv_list != NULL) {
		myqtt_log (MYQTT_LEVEL_DEBUG, "releasing previous listener connections, installed: %d",
			    axl_list_length (ctx->srv_list));
		ctx->reader_cleanup = axl_true;
		axl_list_lookup (ctx->srv_list, __myqtt_reader_configure_conn, NULL);
		axl_list_cursor_free (ctx->srv_cursor);
		axl_list_free (ctx->srv_list);
		ctx->srv_list   = NULL;
		ctx->srv_cursor = NULL;
	} /* end if */

	/* clear reader cleanup flag */
	ctx->reader_cleanup = axl_false;

	/* reader_queue */
	if (ctx->reader_queue != NULL)
		myqtt_async_queue_release (ctx->reader_queue);
	ctx->reader_queue   = myqtt_async_queue_new ();

	/* reader stopped */
	if (ctx->reader_stopped != NULL) 
		myqtt_async_queue_release (ctx->reader_stopped);
	ctx->reader_stopped = myqtt_async_queue_new ();

	/* create the myqtt reader main thread */
	if (! myqtt_thread_create (&ctx->reader_thread, 
				    (MyQttThreadFunc) __myqtt_reader_run,
				    ctx,
				    MYQTT_THREAD_CONF_END)) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "unable to start myqtt reader loop");
		return axl_false;
	} /* end if */
	
	return axl_true;
}

/** 
 * @internal
 * @brief Cleanup myqtt reader process.
 */
void myqtt_reader_stop (MyQttCtx * ctx)
{
	/* get current context */
	MyQttReaderData * data;

	myqtt_log (MYQTT_LEVEL_DEBUG, "stopping myqtt reader ..");

	/* create a bacon to signal myqtt reader that it should stop
	 * and unref resources */
	data       = axl_new (MyQttReaderData, 1);
	data->type = TERMINATE;

	/* push data */
	myqtt_log (MYQTT_LEVEL_DEBUG, "pushing data stop signal..");
	QUEUE_PUSH (ctx->reader_queue, data);
	myqtt_log (MYQTT_LEVEL_DEBUG, "signal sent reader ..");

	/* waiting until the reader is stoped */
	myqtt_log (MYQTT_LEVEL_DEBUG, "waiting myqtt reader 60 seconds to stop");
	if (PTR_TO_INT (myqtt_async_queue_timedpop (ctx->reader_stopped, 60000000))) {
		myqtt_log (MYQTT_LEVEL_DEBUG, "myqtt reader properly stopped, cleaning thread..");
		/* terminate thread */
		myqtt_thread_destroy (&ctx->reader_thread, axl_false);

		/* clear queue */
		myqtt_async_queue_unref (ctx->reader_stopped);
	} else {
		myqtt_log (MYQTT_LEVEL_WARNING, "timeout while waiting myqtt reader thread to stop..");
	}

	return;
}

/** 
 * @internal Allows to check notify myqtt reader to stop its
 * processing and to change its I/O processing model. 
 * 
 * @return The function returns axl_true to notfy that the reader was
 * notified and axl_false if not. In the later case it means that the
 * reader is not running.
 */
axl_bool  myqtt_reader_notify_change_io_api               (MyQttCtx * ctx)
{
	MyQttReaderData * data;

	/* check if the myqtt reader is running */
	if (ctx == NULL || ctx->reader_queue == NULL)
		return axl_false;

	myqtt_log (MYQTT_LEVEL_DEBUG, "stopping myqtt reader due to a request for a I/O notify change...");

	/* create a bacon to signal myqtt reader that it should stop
	 * and unref resources */
	data       = axl_new (MyQttReaderData, 1);
	data->type = IO_WAIT_CHANGED;

	/* push data */
	myqtt_log (MYQTT_LEVEL_DEBUG, "pushing signal to notify I/O change..");
	QUEUE_PUSH (ctx->reader_queue, data);

	/* waiting until the reader is stoped */
	myqtt_async_queue_pop (ctx->reader_stopped);

	myqtt_log (MYQTT_LEVEL_DEBUG, "done, now myqtt reader will wait until the new API is installed..");

	return axl_true;
}

/** 
 * @internal Allows to notify myqtt reader to continue with its
 * normal processing because the new I/O api have been installed.
 */
void myqtt_reader_notify_change_done_io_api   (MyQttCtx * ctx)
{
	MyQttReaderData * data;

	/* create a bacon to signal myqtt reader that it should stop
	 * and unref resources */
	data       = axl_new (MyQttReaderData, 1);
	data->type = IO_WAIT_READY;

	/* push data */
	myqtt_log (MYQTT_LEVEL_DEBUG, "pushing signal to notify I/O is ready..");
	QUEUE_PUSH (ctx->reader_queue, data);

	myqtt_log (MYQTT_LEVEL_DEBUG, "notification done..");

	return;
}

/** 
 * @internal Function that allows to preform a foreach operation over
 * all connections handled by the myqtt reader.
 * 
 * @param ctx The context where the operation will be implemented.
 *
 * @param func The function to execute on each connection. If the
 * function provided is NULL the call will produce to lock until the
 * reader tend the foreach, restarting the reader loop.
 *
 * @param user_data User data to be provided to the function.
 *
 * @return The function returns a reference to the queue that will be
 * used to notify the foreach operation finished.
 */
MyQttAsyncQueue * myqtt_reader_foreach                     (MyQttCtx            * ctx,
							      MyQttForeachFunc      func,
							      axlPointer             user_data)
{
	MyQttReaderData * data;
	MyQttAsyncQueue * queue;

	v_return_val_if_fail (ctx, NULL);

	/* queue an operation */
	data            = axl_new (MyQttReaderData, 1);
	data->type      = FOREACH;
	data->func      = func;
	data->user_data = user_data;
	queue           = myqtt_async_queue_new ();
	data->notify    = queue;
	
	/* queue the operation */
	myqtt_log (MYQTT_LEVEL_DEBUG, "notify foreach reader operation..");
	QUEUE_PUSH (ctx->reader_queue, data);

	/* notification done */
	myqtt_log (MYQTT_LEVEL_DEBUG, "finished foreach reader operation..");

	/* return a reference */
	return queue;
}

/** 
 * @internal Iterate over all connections currently stored on the
 * provided context associated to the myqtt reader. This function is
 * only usable when the context is stopped.
 */
void               myqtt_reader_foreach_offline (MyQttCtx           * ctx,
						  MyQttForeachFunc3    func,
						  axlPointer            user_data,
						  axlPointer            user_data2,
						  axlPointer            user_data3)
{
	/* first iterate over all client connextions */
	axl_list_cursor_first (ctx->conn_cursor);
	while (axl_list_cursor_has_item (ctx->conn_cursor)) {

		/* notify connection */
		func (axl_list_cursor_get (ctx->conn_cursor), user_data, user_data2, user_data3);

		/* next item */
		axl_list_cursor_next (ctx->conn_cursor);
	} /* end while */

	/* now iterate over all server connections */
	axl_list_cursor_first (ctx->srv_cursor);
	while (axl_list_cursor_has_item (ctx->srv_cursor)) {

		/* notify connection */
		func (axl_list_cursor_get (ctx->srv_cursor), user_data, user_data2, user_data3);

		/* next item */
		axl_list_cursor_next (ctx->srv_cursor);
	} /* end while */

	return;
}




/** 
 * @internal Allows to restart the myqtt reader module, locking the
 * caller until the reader restart its loop.
 */
void myqtt_reader_restart (MyQttCtx * ctx)
{
	MyQttAsyncQueue * queue;

	/* call to restart */
	queue = myqtt_reader_foreach (ctx, NULL, NULL);
	myqtt_async_queue_pop (queue);
	myqtt_async_queue_unref (queue);
	return;
}

/* @} */
