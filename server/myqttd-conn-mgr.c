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


#include <myqttd.h>

/* local include */
#include <myqttd-ctx-private.h>

/** 
 * \defgroup myqttd_conn_mgr MyQttd Connection Manager: a module that controls all connections created under the myqttd execution
 */

/** 
 * \addtogroup myqttd_conn_mgr
 * @{
 */

/** 
 * @internal Handler called once the connection is about to be closed.
 * 
 * @param conn The connection to close.
 */
void myqttd_conn_mgr_on_close (MyQttConn * conn, 
				   axlPointer         user_data)
{
	/* get myqttd context */
	MyQttdCtx          * ctx = user_data;

	/* check if we are the last reference */
	if (vortex_connection_ref_count (conn) == 1)
		return;

	/* do not remove if hash is not defined */
	if (ctx->conn_mgr_hash == NULL)
		return;

	/* new connection created: configure it */
	vortex_mutex_lock (&ctx->conn_mgr_mutex);

	/* remove from the hash */
	axl_hash_remove (ctx->conn_mgr_hash, INT_TO_PTR (vortex_connection_get_id (conn)));

	/* unlock */
	vortex_mutex_unlock (&ctx->conn_mgr_mutex);


	return;
}

void __myqttd_conn_mgr_unref_show_errors (MyQttdCtx * ctx, MyQttConn *conn)
{
	int    code;
	char * message;
	while (vortex_connection_pop_channel_error (conn, &code, &message)) {
		switch (code) {
		case VortexOk:
		case MyQttConnCloseCalled:
		case VortexUnnotifiedConnectionClose:
			axl_free (message);
			break;
		default:
			error ("  error: %d, %s", code, message);
			axl_free (message);
			break;
		}
	}
	return;
}

void myqttd_conn_mgr_unref (axlPointer data)
{
	/* get a reference to the state */
	MyQttdConnMgrState * state = data;
	MyQttdCtx          * ctx   = state->ctx;

	/* check connection status */
	if (state->conn) {
		/* remove installed handlers */
		vortex_connection_remove_handler (state->conn, CONNECTION_CHANNEL_ADD_HANDLER, state->added_channel_id);
		vortex_connection_remove_handler (state->conn, CONNECTION_CHANNEL_REMOVE_HANDLER, state->removed_channel_id);

		/* uninstall on close full handler to avoid race conditions */
		vortex_connection_remove_on_close_full (state->conn, myqttd_conn_mgr_on_close, ctx);

		/* drop errors found on the connection */
		__myqttd_conn_mgr_unref_show_errors (ctx, state->conn);
		
		/* unref the connection */
		msg ("Unregistering connection: %d (%p, socket: %d)", 
		     vortex_connection_get_id ((MyQttConn*) state->conn), state->conn, vortex_connection_get_socket (state->conn));
		vortex_connection_unref ((MyQttConn*) state->conn, "myqttd-conn-mgr");
	} /* end if */

	/* finish profiles running hash */
	axl_hash_free (state->profiles_running);

	/* nullify and free */
	memset (state, 0, sizeof (MyQttdConnMgrState));
	axl_free (state);

	/* check if we have to initiate child process termination */
	if (ctx->child && ctx->started) {
		/* msg ("CHILD: Checking for process termination, current connections are: %d",
		   axl_hash_items (ctx->conn_mgr_hash)); */
		if (axl_hash_items (ctx->conn_mgr_hash) == 0) {
			wrn ("CHILD: Starting finishing process, current connections are: 0");
			myqttd_process_check_for_finish (ctx);
		}
	} /* end if */

	return;
}

void myqttd_conn_mgr_added_handler (VortexChannel * channel, axlPointer user_data)
{
	MyQttdConnMgrState * state;
	MyQttdCtx          * ctx             = user_data;
	MyQttConn       * conn;
	/* copy running_profile to avoid having a reference inside the
	 * hash to a pointer that may be lost when the channel is
	 * closed by still other channels with the same profile are
	 * running in the connection. */
	char                   * running_profile;
	int                      count;

	/* check if hash is finished */
	if (ctx->conn_mgr_hash == NULL) 
		return;


	/* get the lock */
	vortex_mutex_lock (&ctx->conn_mgr_mutex);

	/* get state */
	conn  = vortex_channel_get_connection (channel);
	state = axl_hash_get (ctx->conn_mgr_hash, INT_TO_PTR (vortex_connection_get_id (conn)));
	if (state == NULL) {
		/* get the lock */
		vortex_mutex_unlock (&ctx->conn_mgr_mutex);
		return;
	}
	
	/* make a copy of running profile */
	running_profile = axl_strdup (vortex_channel_get_profile (channel));

	/* get channel count for the profile */
	count = PTR_TO_INT (axl_hash_get (state->profiles_running, (axlPointer) running_profile));
	count++;

	axl_hash_insert_full (state->profiles_running, (axlPointer) running_profile, axl_free, INT_TO_PTR (count), NULL);

	/* configure here channel complete flag limit */
	vortex_channel_set_complete_frame_limit (channel, ctx->max_complete_flag_limit);

	/* release the lock */
	vortex_mutex_unlock (&ctx->conn_mgr_mutex);

	return;
}

void myqttd_conn_mgr_removed_handler (VortexChannel * channel, axlPointer user_data)
{
	MyQttdCtx          * ctx             = user_data;
	MyQttdConnMgrState * state;
	const char             * running_profile = vortex_channel_get_profile (channel);
	int                      count;
	MyQttConn       * conn;

	/* check if hash is finished */
	if (ctx->conn_mgr_hash == NULL) 
		return;

	/* get the lock */
	vortex_mutex_lock (&ctx->conn_mgr_mutex);

	/* get the state and check reference */
	conn  = vortex_channel_get_connection (channel);
	state = axl_hash_get (ctx->conn_mgr_hash, INT_TO_PTR (vortex_connection_get_id (conn)));
	if (state == NULL) {
		/* get the lock */
		vortex_mutex_unlock (&ctx->conn_mgr_mutex);
		return;
	}
	
	/* get channel count for the profile */
	count = PTR_TO_INT (axl_hash_get (state->profiles_running, (axlPointer) running_profile));
	count--;

	/* if reached 0 count, remove the key */
	if (count <= 0) {
		axl_hash_remove (state->profiles_running, (axlPointer) running_profile);

		/* release the lock */
		vortex_mutex_unlock (&ctx->conn_mgr_mutex);		
		return;
	} /* end if */

	/* copy running_profile to avoid having a reference inside the
	 * hash to a pointer that may be lost when the channel is
	 * closed by still other channels with the same profile are
	 * running in the connection. */
	running_profile = axl_strdup (running_profile);

	/* update count */
	axl_hash_insert_full (state->profiles_running, (axlPointer) running_profile, axl_free, INT_TO_PTR (count), NULL);

	/* release the lock */
	vortex_mutex_unlock (&ctx->conn_mgr_mutex);

	return;
}

/** 
 * @internal Function called foreach connection created within the
 * myqttd context.
 * 
 * @param conn The connection created.
 *
 * @param user_data User user defined data.
 */
int myqttd_conn_mgr_notify (VortexCtx               * vortex_ctx,
				MyQttConn        * conn, 
				MyQttConn       ** new_conn,
				MyQttConnStage     conn_state,
				axlPointer                user_data)
{
	/* get myqttd context */
	MyQttdConnMgrState * state;
	MyQttdCtx          * ctx   = user_data;
	/* the following reference is only defined when register
	 * process is done on a child process */
	MyQttdChild        * child = ctx->child;
	MyQttConn       * temp;

	/* skip connection that should be registered at conn mgr */
	if (vortex_connection_get_data (conn, "tbc:conn:mgr:!")) 
		return 1;

	/* NOTE REFERECE 002: check if we are a child process and the
	 * connection isn't the result of the temporal listener to
	 * create child control connection */
	if (ctx->child && (vortex_connection_get_role (child->conn_mgr) == VortexRoleMasterListener)) {
		msg ("CHILD: Checking if conn id=%d %p (role: %d) is because connection id=%d (role: %d)", 
		     vortex_connection_get_id (conn), conn, vortex_connection_get_role (conn), 
		     vortex_connection_get_id (child->conn_mgr), vortex_connection_get_role (child->conn_mgr));

		/* check listener */
		temp = vortex_connection_get_listener (conn);
		msg ("       master pointer is: %p, temp pointer is: %p", child->conn_mgr, temp);

		if (temp == child->conn_mgr) {
			
			/* replace reference to keep the accepted listener */
			temp = child->conn_mgr;

			/* set and update reference counting */
			child->conn_mgr = conn;
			vortex_connection_ref (conn, "conn mgr (child process)");

			msg ("CHILD: not registering connection id=%d (socket %d, refs: %d) because it is conn mgr (shutting down temporal listener id: %d, refs: %d)", 
			     vortex_connection_get_id (conn), vortex_connection_get_socket (conn), vortex_connection_ref_count (conn),
			     vortex_connection_get_id (temp), vortex_connection_ref_count (temp));

			/* shutdown and close */
			vortex_connection_shutdown (temp);
			vortex_connection_close (temp);

			return 1;
		} /* end if */
	} /* end if */

	/* create state */
	state       = axl_new (MyQttdConnMgrState, 1);
	if (state == NULL)
		return -1;
	if (! vortex_connection_ref (conn, "myqttd-conn-mgr")) {
		error ("Failed to acquire reference to connection during conn mgr notification, dropping");
		axl_free (state);
		return -1;
	} /* end if */
	state->conn = conn;
	state->ctx  = ctx;

	/* store in the hash */
	msg ("Registering connection: %d (%p, refs: %d, channels: %d, socket: %d)", 
	     vortex_connection_get_id (conn), conn, vortex_connection_ref_count (conn), 
	     vortex_connection_channels_count (conn), vortex_connection_get_socket (conn));

	/* new connection created: configure it */
	vortex_mutex_lock (&ctx->conn_mgr_mutex);

	axl_hash_insert_full (ctx->conn_mgr_hash, 
			      /* key to store */
			      INT_TO_PTR (vortex_connection_get_id (conn)), NULL,
			      /* data to store */
			      state, myqttd_conn_mgr_unref);

	/* configure on close */
	vortex_connection_set_on_close_full (conn, myqttd_conn_mgr_on_close, ctx);

	/* init profiles running hash */
	state->profiles_running   = axl_hash_new (axl_hash_string, axl_hash_equal_string);
	state->added_channel_id   = vortex_connection_set_channel_added_handler (conn, myqttd_conn_mgr_added_handler, ctx);
	state->removed_channel_id = vortex_connection_set_channel_removed_handler (conn, myqttd_conn_mgr_removed_handler, ctx);

	/* unlock */
	vortex_mutex_unlock (&ctx->conn_mgr_mutex);

	/* signal no error was found and the rest of handler can be
	 * executed */
	return 1;
}

axlDoc * myqttd_conn_mgr_show_connections (const char * line,
					       axlPointer   user_data, 
					       axl_bool   * status)
{
	MyQttdCtx          * ctx = (MyQttdCtx *) user_data;
	axlDoc                 * doc;
	axlHashCursor          * cursor;
	MyQttdConnMgrState * state;
	axlNode                * parent;
	axlNode                * node;

	/* signal command completed ok */
	(* status) = axl_true;

	/* build document */
	doc = axl_doc_parse_strings (NULL, 
				     "<table>",
				     "  <title>BEEP peers connected</title>",
				     "  <description>The following is a list of peers connected</description>",
				     "  <content></content>",
				     "</table>");
	/* get parent node */
	parent = axl_doc_get (doc, "/table/content");

	/* lock connections */
	vortex_mutex_lock (&ctx->conn_mgr_mutex);
	
	/* create cursor */
	cursor = axl_hash_cursor_new (ctx->conn_mgr_hash);

	while (axl_hash_cursor_has_item (cursor)) {
		/* get connection */
		state = axl_hash_cursor_get_value (cursor);

		/* build connection status information */
		node  = axl_node_parse (NULL, "<row id='%d' host='%s' port='%s' local-addr='%s' local-port='%s' />",
					vortex_connection_get_id (state->conn),
					vortex_connection_get_host (state->conn),
					vortex_connection_get_port (state->conn),
					vortex_connection_get_local_addr (state->conn),
					vortex_connection_get_local_port (state->conn));

		/* set node to result document */
		axl_node_set_child (parent, node);

		/* next cursor */
		axl_hash_cursor_next (cursor);
	} /* end while */
	
	/* free cursor */
	axl_hash_cursor_free (cursor);

	/* unlock connections */
	vortex_mutex_unlock (&ctx->conn_mgr_mutex);
	
	return doc;
}

/** 
 * @internal Function used to catch modules registered. In the case
 * radmin module is found, publish a list of commands to manage
 * connections.
 */
void myqttd_conn_mgr_module_registered (MyQttdMediatorObject * object)
{
/*	MyQttdCtx * ctx  = myqttd_mediator_object_get (object, MYQTTD_MEDIATOR_ATTR_CTX);
	const char    * name = myqttd_mediator_object_get (object, MYQTTD_MEDIATOR_ATTR_EVENT_DATA); */

	return;
}

/** 
 * @internal Module init.
 */
void myqttd_conn_mgr_init (MyQttdCtx * ctx, axl_bool reinit)
{
	VortexCtx  * vortex_ctx = myqttd_ctx_get_vortex_ctx (ctx);

	/* init mutex */
	vortex_mutex_create (&ctx->conn_mgr_mutex);

	/* check for reinit operation */
	if (reinit) {
		/* lock during update */
		vortex_mutex_lock (&ctx->conn_mgr_mutex);

		axl_hash_free (ctx->conn_mgr_hash);
		ctx->conn_mgr_hash = axl_hash_new (axl_hash_int, axl_hash_equal_int);

		/* release */
		vortex_mutex_unlock (&ctx->conn_mgr_mutex);
		return;
	}

	/* init connection list hash */
	if (ctx->conn_mgr_hash == NULL) {
		
		ctx->conn_mgr_hash = axl_hash_new (axl_hash_int, axl_hash_equal_int);

		/* configure notification handlers */
		vortex_connection_set_connection_actions (vortex_ctx,
							  CONNECTION_STAGE_POST_CREATED,
							  myqttd_conn_mgr_notify, ctx);
	} /* end if */

	/* register on load module to install radmin connection
	   commands */
	myqttd_mediator_subscribe (ctx, "myqttd", "module-registered", 
				       myqttd_conn_mgr_module_registered, NULL);

	return;
}

/** 
 * @internal Function used to manually register connections on
 * myqttd connection manager.
 */
void myqttd_conn_mgr_register (MyQttdCtx * ctx, MyQttConn * conn)
{
	/* simulate event */
	msg ("Registering connection at child process (%d) conn id: %d (status: %d), refs: %d", 
	     getpid (), vortex_connection_get_id (conn), vortex_connection_is_ok (conn, axl_false), vortex_connection_ref_count (conn));
	myqttd_conn_mgr_notify (TBC_VORTEX_CTX (ctx), conn, NULL, CONNECTION_STAGE_POST_CREATED, ctx);
	msg ("After register, connections are: %d", axl_hash_items (ctx->conn_mgr_hash));
	return;
}


/** 
 * @internal Function used to unregister a particular connection from
 * the connection manager.
 */
void myqttd_conn_mgr_unregister    (MyQttdCtx    * ctx, 
					MyQttConn * conn)
{
	/* new connection created: configure it */
	vortex_mutex_lock (&ctx->conn_mgr_mutex);

	/* remove from the hash */
	axl_hash_remove (ctx->conn_mgr_hash, INT_TO_PTR (vortex_connection_get_id (conn)));

	/* unlock */
	vortex_mutex_unlock (&ctx->conn_mgr_mutex);

	return;
}

typedef struct _MyQttdBroadCastMsg {
	const void      * message;
	int               message_size;
	const char      * profile;
	MyQttdCtx   * ctx;
} MyQttdBroadCastMsg;

int  _myqttd_conn_mgr_broadcast_msg_foreach (axlPointer key, axlPointer data, axlPointer user_data)
{
	VortexChannel          * channel   = data;
	MyQttdBroadCastMsg * broadcast = user_data;
#if ! defined(SHOW_FORMAT_BUGS)
	MyQttdCtx          * ctx       = broadcast->ctx; 
#endif

	/* check the channel profile */
	if (! axl_cmp (vortex_channel_get_profile (channel), broadcast->profile)) 
		return axl_false;

	/* channel found send the message */
	msg2 ("sending notification on channel=%d, conn=%d running profile: %s", 
	      vortex_channel_get_number (channel), vortex_connection_get_id (vortex_channel_get_connection (channel)),
	      broadcast->profile); 
	vortex_channel_send_msg (channel, broadcast->message, broadcast->message_size, NULL);

	/* always return axl_true to make the process to continue */
	return axl_false;
}

/** 
 * @brief General purpose function that allows to broadcast the
 * provided message (and size), over all channel, running the profile
 * provided, on all connections registered. 
 *
 * The function is really useful if it is required to perform a
 * general broadcast to all BEEP peers connected running a particular
 * profile. The function produces a MSG frame with the content
 * provided.
 *
 * The function also allows to configure the connections that aren't
 * broadcasted. To do so, pass the function (filter_conn) that
 * configures which connections receives the notification and which
 * not.
 * 
 * @param ctx MyQttd context where the operation will take place.
 * 
 * @param message The message that is being broadcasted. This
 * parameter is not optional. For empty messages use "" instead of
 * NULL.
 *
 * @param message_size The message size to broadcast. This parameter
 * is not optional. For empty messages use 0.
 *
 * @param profile The profile to search for in all connections
 * registered. If a channel is found running this profile, then a
 * message is sent. This attribute is not optional.
 *
 * @param filter_conn Connection filtering function. If it returns
 * axl_true, the connection is filter. Optional parameter.
 *
 * @param filter_data User defined data provided to the filter
 * function. Optional parameter.
 * 
 * @return axl_true if the broadcast message was sent to all
 * connections. The function could return axl_false but it has no support
 * to notify which was the connection(s) or channel(s) that failed.
 */
axl_bool  myqttd_conn_mgr_broadcast_msg (MyQttdCtx            * ctx,
					     const void               * message,
					     int                        message_size,
					     const char               * profile,
					     MyQttdConnMgrFilter    filter_conn,
					     axlPointer                 filter_data)
{

	/* get myqttd context */
	axlHashCursor          * cursor;
	MyQttConn       * conn;
	MyQttdBroadCastMsg * broadcast;
	MyQttdConnMgrState * state;
	axl_bool                 should_filter;

	v_return_val_if_fail (message, axl_false);
	v_return_val_if_fail (message_size >= 0, axl_false);
	v_return_val_if_fail (profile, axl_false);

	/* lock and send */
	vortex_mutex_lock (&ctx->conn_mgr_mutex);
	
	/* create the cursor */
	cursor = axl_hash_cursor_new (ctx->conn_mgr_hash);

	/* create the broadcast data */
	broadcast               = axl_new (MyQttdBroadCastMsg, 1);
	broadcast->message      = message;
	broadcast->message_size = message_size;
	broadcast->profile      = profile;
	broadcast->ctx          = ctx;

	while (axl_hash_cursor_has_item (cursor)) {
		
		/* get data */
		state   = axl_hash_cursor_get_value (cursor);
		conn    = state->conn;

		/* check if connection is nullified */
		if (conn == NULL) {
			/* connection filtered */
			axl_hash_cursor_next (cursor);
			continue;
		} /* end if */

		/* check filter function */

		if (filter_conn) {
			/* unlock during the filter call to allow
			 * conn-mgr reentrancy from inside filter
			 * handler */
			vortex_mutex_unlock (&ctx->conn_mgr_mutex); 

			/* get filtering result */
			should_filter = filter_conn (conn, filter_data);

			/* lock */
			vortex_mutex_lock (&ctx->conn_mgr_mutex); 

			if (should_filter) {
				/* connection filtered */
				axl_hash_cursor_next (cursor);
				continue;
			} /* end if */
		} /* end if */

		/* search for channels running the profile provided */
		if (! vortex_connection_foreach_channel (conn, _myqttd_conn_mgr_broadcast_msg_foreach, broadcast))
			error ("failed to broacast message over connection id=%d", vortex_connection_get_id (conn));

		/* next cursor */
		axl_hash_cursor_next (cursor);
	}

	/* unlock */
	vortex_mutex_unlock (&ctx->conn_mgr_mutex);

	/* free cursor hash and broadcast message */
	axl_hash_cursor_free (cursor);
	axl_free (broadcast);

	return axl_true;
}

void myqttd_conn_mgr_conn_list_free_item (axlPointer _conn)
{
	vortex_connection_unref ((MyQttConn *) _conn, "conn-mgr-list");
	return;
}

/** 
 * @brief Allows to get a list of connections registered on the
 * connection manager, matching the providing role and then filtered
 * by the provided filter string. 
 *
 * @param ctx The context where the operation will take place.
 *
 * @param role Connection role to select connections. Use -1 to select
 * all connections registered on the manager, no matter its role. 
 *
 * @param filter Optional filter expresion to resulting connection
 * list. It can be NULL.
 *
 * @return A newly allocated connection list having on each position a
 * reference to a MyQttConn object. The caller must finish the
 * list with axl_list_free to free resources. The function returns NULL if it fails.
 */
axlList *  myqttd_conn_mgr_conn_list   (MyQttdCtx            * ctx, 
					    VortexPeerRole             role,
					    const char               * filter)
{
	axlList                * result;
	MyQttConn       * conn;
	axlHashCursor          * cursor;
	MyQttdConnMgrState * state;

	v_return_val_if_fail (ctx, NULL);

	/* lock and send */
	vortex_mutex_lock (&ctx->conn_mgr_mutex);
	
	/* create the cursor */
	cursor = axl_hash_cursor_new (ctx->conn_mgr_hash);
	result = axl_list_new (axl_list_always_return_1, myqttd_conn_mgr_conn_list_free_item);

	msg ("connections registered: %d..", axl_hash_items (ctx->conn_mgr_hash));

	while (axl_hash_cursor_has_item (cursor)) {
		
		/* get data */
		state   = axl_hash_cursor_get_value (cursor);
		conn    = state->conn;

		/* check connection role to add it to the result
		   list */
		msg ("Checking connection role %d == %d", vortex_connection_get_role (conn), role);
		if ((role == -1) || vortex_connection_get_role (conn) == role) {
			/* update reference and add the connection */
			vortex_connection_ref (conn, "conn-mgr-list");
			axl_list_append (result, conn);
		} /* end if */
		
		/* next cursor */
		axl_hash_cursor_next (cursor);
	}

	/* unlock */
	vortex_mutex_unlock (&ctx->conn_mgr_mutex);

	/* free cursor */
	axl_hash_cursor_free (cursor);

	/* return list */
	return result;
}

/** 
 * @brief Allows to get number of connections currently handled by
 * this process.
 * @param ctx The context where the query will be served.
 * @return The number of connections or -1 if it fails.
 */
int        myqttd_conn_mgr_count       (MyQttdCtx            * ctx)
{
	int result;

	v_return_val_if_fail (ctx, -1);

	/* lock and send */
	vortex_mutex_lock (&ctx->conn_mgr_mutex);

	/* get hash list */
	result = axl_hash_items (ctx->conn_mgr_hash);

	/* unlock */
	vortex_mutex_unlock (&ctx->conn_mgr_mutex);	

	return result;
}

/** 
 * @brief Allows to check if the provided connection has to be proxied
 * on the parent master process in the case it is required to be sent
 * to a child process due to profile path configuration.
 *
 * To flag a connection in such way will create an especial
 * configuration at the parent process to read content over the
 * provided connection and send it to a "representation" running on
 * the child (as opposed to fully send the entire connection to be
 * handled by the child).
 *
 * This is done to support some especiall cases (especially those
 * where TLS is around) where it is not possible to store the state of a connection and resume it on a child process.
 *
 * NOTE: This function only flags! it does not actually send the
 * connection to a particular child (that only will happens when the
 * site administrator configures the profile path to make it).
 *
 * @param conn The connection that is flagged to be proxied on parent. 
 *
 * @return axl_true in the case the connection was flagged to be
 * proxied on on parent otherwise, axl_false is returned.
 *
 * NOTE TO DEVELOPERS: in the case you are creating a module that
 * needs to activate this feature, just set the value as follows:
 *
 * \code
 * // set up proxy configuration
 * vortex_connection_set_data (conn, "tbc:proxy:conn", INT_TO_PTR (axl_true));
 * \endcode
 *
 */
axl_bool   myqttd_conn_mgr_proxy_on_parent (MyQttConn * conn)
{
	/* set up proxy configuration */
	return PTR_TO_INT (vortex_connection_get_data (conn, "tbc:proxy:conn"));
}

/** 
 * Function used to read content from the connection and write that
 * content into the child socket.
 *
 *           Read                    Write
 *  [ Proxied connection ] --> [ Child socket ]
 */
void __myqttd_conn_mgr_proxy_reads (MyQttConn * conn)
{
	char               buffer[4096];
	int                bytes_read;
#if ! defined(SHOW_FORMAT_BUGS)
	MyQttdCtx    * ctx = vortex_connection_get_data (conn, "tbc:ctx");  
#endif
	/* get socket associated */
	int                _socket = PTR_TO_INT (vortex_connection_get_data (conn, "tbc:proxy:fd"));

	/* check connection status */
	if (! vortex_connection_is_ok (conn, axl_false)) 
		return;

	/* check status and close the other connection if found that */
	bytes_read = vortex_frame_receive_raw (conn, buffer, 4096);

	/* msg ("PROXY-beep: Read %d bytes from conn-id=%d, sending them to child socket=%d (refs: %d, status: %d, errno=%d%s%s)",
	     bytes_read, vortex_connection_get_id (conn), _socket,
	     vortex_connection_ref_count (conn), vortex_connection_is_ok (conn, axl_false), errno,
	     errno != 0 ? " : " : "",
	     errno != 0 ? strerror (errno) : "");  */

	if (bytes_read > 0) {
		/* send content */
		if (send (_socket, buffer, bytes_read, 0) != bytes_read) {
			wrn ("PROXY-beep: closing conn-id=%d because socket=%d isn't working", 
			     vortex_connection_get_id (conn), _socket); 

			/* remove preread handler and shutdown */
			vortex_connection_shutdown (conn);
			return;
		} /* end if */

		/* buffer[bytes_read] = 0;
		   msg ("PROXY-beep: sent content (beep conn-id=%d -> socket=%d): %s", vortex_connection_get_id (conn), _socket, buffer); */
	} /* end if */

	return;
}

/** 
 * Function used to read content from the socket in the loop into the
 * connection proxied.
 *
 *       Read                   Write
 *  [ Loop socket ] --> [ Proxy connection ]
 */
axl_bool __myqttd_conn_proxy_reads_loop (MyQttdLoop * loop, 
					     MyQttdCtx  * ctx,
					     int              descriptor, 
					     axlPointer       ptr, 
					     axlPointer       ptr2)
{
	MyQttConn * conn = ptr;
	char buffer[4096];
	int  bytes_read;

	/* read content */
	bytes_read = recv (descriptor, buffer, 4096, 0);
	/* msg ("PROXY: reading from socket=%d (bytes read=%d, errno=%d)", descriptor, bytes_read, errno);  */

	/* send content */
	if (bytes_read <= 0 || 
	    ! vortex_connection_is_ok (conn, axl_false) || 
	    ! vortex_frame_send_raw (conn, buffer, bytes_read)) {

		/* close socket and unregister it and close associated conn */
		wrn ("PROXY-fd: connection-id=%d is falling (wasn't able to send %d bytes, is zero?), closing associated socket=%d", 
		     vortex_connection_get_id (conn), bytes_read, descriptor); 

		/* check connection status to finish it */
		if (vortex_connection_is_ok (conn, axl_false))
			vortex_connection_shutdown (conn);
		
		return axl_false;
	} /* end if */

	return axl_true; /* continue reading that socket */
}

axlPointer __myqttd_conn_mgr_release_proxy_conn (axlPointer conn)
{
	/* call to unref the connection */
	vortex_connection_unref ((MyQttConn *) conn, "proxy-on-parent");
	return NULL;
}

void __myqttd_conn_mgr_proxy_on_close (MyQttConn * conn, axlPointer _loop)
{
	MyQttdLoop  * loop = _loop;
	/* MyQttdCtx   * ctx  = myqttd_loop_ctx (loop); */

	/* get socket associated */
	int               _socket = PTR_TO_INT (vortex_connection_get_data (conn, "tbc:proxy:fd"));	

	/* msg ("PROXY: closing connection-id=%d, refs=%d, socket=%d", 
	   vortex_connection_get_id (conn), vortex_connection_ref_count (conn), _socket); */

	/* unregister socket from loop watcher */
	/* msg ("PROXY: calling to unwatch descriptor from loop _socket=%d (finished watching=%d)", _socket, myqttd_loop_watching (loop)); */
	myqttd_loop_unwatch_descriptor (loop, _socket, axl_true);
	/* msg ("PROXY: calling to unwatch descriptor from loop _socket=%d (finished watching=%d)", _socket, myqttd_loop_watching (loop)); */

	/* close socket */
	vortex_close_socket (_socket);
	
	/* release and shutdown */
	vortex_connection_set_preread_handler (conn, NULL);

	/* reduce reference counting but do it out side the close handler */
	vortex_thread_pool_new_task (CONN_CTX (conn), __myqttd_conn_mgr_release_proxy_conn, conn);

	return;
}

/** 
 * @brief Setups the necessary configuration to start proxing content
 * of that connection passing all bytes into the returned socket.
 *
 * @param ctx The myqttd context where the operation will take place.
 *
 * @param conn The connection that will be proxied.
 *
 * @return The socket where all content will be proxied, remaining on
 * the parent process all code to reading and writing the provided
 * socket.
 */
int        myqttd_conn_mgr_setup_proxy_on_parent (MyQttdCtx * ctx, MyQttConn * conn)
{
	int                     descf[2];

	/* here you have the diagram about what is about to happen:
	 *
	 * +----------+                      +--------------+
	 * |  remote  |   <----[ conn ]----> |  MyQttd  |
	 * |          |                      |    loop      |
	 * |  client  |                      +--------------+
	 * +----------+                             ^
         *                                          |    
	 *                                       descf[1]
	 *                                          ^
         *                                          |
         *                                          |
	 *                                          v
	 *                                       descf[0]
	 *				     +-------------+
         *                                   |   child     |
         *                                   |  process    |
         *                                   +-------------+
	 *
	 * read on [conn]     -> write in descf[1]
	 * read on descf[1]   -> write in [conn]
	 */

	/* init the portable pipe associated to this connection */
	if (vortex_support_pipe (CONN_CTX (conn), descf) != 0) {
		error ("Failed to create pipe to proxy connection on the parent, check vortex log to get more details");
		return -1;
	} /* end if */

	/* acquire a connection reference to ensure the loop's got it */
	if (! vortex_connection_ref (conn, "proxy-on-parent")) {
		vortex_close_socket (descf[0]);
		vortex_close_socket (descf[1]);
		return -1;
	} /* end if */

	/* create the proxy loop watcher if it wasn't created yet */
	if (ctx->proxy_loop == NULL) 
		ctx->proxy_loop = myqttd_loop_create (ctx);

	/* watch the socket */
	myqttd_loop_watch_descriptor (ctx->proxy_loop, descf[1], __myqttd_conn_proxy_reads_loop, conn, NULL);

	/* configure links between both connections */
	vortex_connection_set_data (conn,       "tbc:proxy:fd", INT_TO_PTR (descf[1]));
	vortex_connection_set_data (conn,       "tbc:ctx", ctx);

	/* now configure preread handlers to pass data from both
	 * connections */
	vortex_connection_set_preread_handler (conn, __myqttd_conn_mgr_proxy_reads);

	/* setup connection close to cleanup */
	vortex_connection_set_on_close_full (conn, __myqttd_conn_mgr_proxy_on_close, ctx->proxy_loop);
	
	/* return the socket that will be using the child process */
	msg ("PROXY: Activated proxy on parent conn-id=%d (socket: %d), parent socket %d <--> child socket: %d", 
	     vortex_connection_get_id (conn), vortex_connection_get_socket (conn), descf[1], descf[0]);
	return descf[0];
}

/** 
 * @brief Allows to get a reference to the registered connection with
 * the provided id. The function will return a reference to a
 * MyQttConn owned by the myqttd connection
 * manager. 
 *
 * @param ctx The myqttd conext where the connection reference will be looked up.
 * @param conn_id The connection id to lookup.
 *
 * @return A MyQttConn reference or NULL value it if fails.
 */
MyQttConn * myqttd_conn_mgr_find_by_id (MyQttdCtx * ctx,
						   int             conn_id)
{
	MyQttConn       * conn = NULL;
	MyQttdConnMgrState * state;

	v_return_val_if_fail (ctx, NULL);

	/* lock and send */
	vortex_mutex_lock (&ctx->conn_mgr_mutex);
	
	/* get the connection */
	state = axl_hash_get (ctx->conn_mgr_hash, INT_TO_PTR (conn_id));
	
	/* set conection */
	/* msg ("Connection find_by_id for conn id=%d returned pointer %p (conn: %p)", conn_id, state, state ? state->conn : NULL); */
	if (state)
		conn = state->conn;

	/* unlock */
	vortex_mutex_unlock (&ctx->conn_mgr_mutex);

	/* return list */
	return conn;
}

axl_bool count_channels (axlPointer key, axlPointer _value, axlPointer user_data, axlPointer _ctx)
{
	int           * count  = user_data;
	int             value  = PTR_TO_INT(_value);
#if ! defined(SHOW_FORMAT_BUGS)
	MyQttdCtx * ctx    = _ctx;
#endif
	
	/* count */
	msg2 ("Adding %d to current count %d (profile: %s)", *count, value, (const char *) key);
	(*count) = (*count) + value;

	return axl_false; /* iterate over all items found in the
			   * hash */
}

/** 
 * @internal Allows to get a newly created hash cursor pointing to opened
 * profiles on this connection (key), containing how many channels
 * runs that profile (value), stored in a hash.
 *
 * @param ctx The MyQttdCtx object where the operation will be applied.
 * @param conn The connection where it is required to get profile stats.
 *
 * @return A newly created cursor or NULL if it fails. The function
 * will only fail if ctx or conn are NULL or because not enough memory
 * to hold the cursor.
 */
axlHashCursor    * myqttd_conn_mgr_profiles_stats (MyQttdCtx    * ctx,
						       MyQttConn * conn)
{
	MyQttdConnMgrState * state;
	axlHashCursor          * cursor;
	int                      total_count;

	v_return_val_if_fail (ctx && conn, NULL);
	
	/* new connection created: configure it */
	vortex_mutex_lock (&ctx->conn_mgr_mutex);

	/* get state */
	state = axl_hash_get (ctx->conn_mgr_hash, INT_TO_PTR (vortex_connection_get_id (conn)));
	if (state == NULL) {
		if (vortex_connection_channels_count (conn) > 1) {
			error ("Failed to find connection manager internal state associated to connection id=%d but it has channels %d, failed to return stats..",
			       vortex_connection_channels_count (conn), vortex_connection_get_id (conn));
		} /* end if */
		/* unlock the mutex */
		vortex_mutex_unlock (&ctx->conn_mgr_mutex);
		return NULL;
	}

	/* ensure current hash info is consistent with the channel
	 * number running on the connection */
	total_count = 0;
	axl_hash_foreach2 (state->profiles_running, count_channels, &total_count, ctx);
	if (total_count != (vortex_connection_channels_count (conn) -1)) {
		/* release current mutex */
		vortex_mutex_unlock (&ctx->conn_mgr_mutex);
		msg2 ("  Found inconsistent channel count (%d != %d) for profile stats, unlock and wait", 
		      total_count, (vortex_connection_channels_count (conn) - 1));
		
		/* call to wait before calling again to get mgr
		 * stats (2ms) */
		myqttd_ctx_wait (ctx, 2000);
		
		/* call to get stats again */
		return myqttd_conn_mgr_profiles_stats (ctx, conn);
	}
	
	/* create the cursor */
	cursor = axl_hash_cursor_new (state->profiles_running);

	/* unlock the mutex */
	vortex_mutex_unlock (&ctx->conn_mgr_mutex);

	return cursor;
}

/** 
 * @internal Ensure we close all active connections before existing...
 */
axl_bool myqttd_conn_mgr_shutdown_connections (axlPointer key, axlPointer data, axlPointer user_data) 
{
	MyQttdConnMgrState * state = data;
	MyQttdCtx          * ctx   = state->ctx;
	MyQttConn       * conn  = state->conn;

	/* remove installed handlers */
	vortex_connection_remove_handler (state->conn, CONNECTION_CHANNEL_ADD_HANDLER, state->added_channel_id);
	vortex_connection_remove_handler (state->conn, CONNECTION_CHANNEL_REMOVE_HANDLER, state->removed_channel_id);

	/* nullify conn reference on state */
	state->conn = NULL;

	/* uninstall on close full handler to avoid race conditions */
	vortex_connection_remove_on_close_full (conn, myqttd_conn_mgr_on_close, ctx);

	msg ("shutting down connection id %d", vortex_connection_get_id (conn));
	vortex_connection_shutdown (conn);
	msg ("..socket status after shutdown: %d..", vortex_connection_get_socket (conn));

	/* now unref */
	vortex_connection_unref (conn, "myqttd-conn-mgr shutdown");

	/* keep on iterating over all connections */
	return axl_false;
}

/** 
 * @internal Module cleanup.
 */
void myqttd_conn_mgr_cleanup (MyQttdCtx * ctx)
{
	axlHash * conn_hash;

	/* shutdown all pending connections */
	msg ("calling to cleanup registered connections that are still opened: %d", axl_hash_items (ctx->conn_mgr_hash));

	/* nullify hash to be the only owner */
	vortex_mutex_lock (&ctx->conn_mgr_mutex);
	conn_hash          = ctx->conn_mgr_hash;
	ctx->conn_mgr_hash = NULL;
	vortex_mutex_unlock (&ctx->conn_mgr_mutex);

	/* finish the hash */
	axl_hash_foreach (conn_hash, myqttd_conn_mgr_shutdown_connections, NULL);

	/* destroy mutex */
	axl_hash_free (conn_hash);

	vortex_mutex_destroy (&ctx->conn_mgr_mutex);

	return;
}

/** 
 * @}
 */
