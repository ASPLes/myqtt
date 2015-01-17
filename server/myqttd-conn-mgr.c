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
#include <myqtt-conn-private.h>

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
	if (myqtt_conn_ref_count (conn) == 1)
		return;

	/* do not remove if hash is not defined */
	if (ctx->conn_mgr_hash == NULL)
		return;

	/* new connection created: configure it */
	myqtt_mutex_lock (&ctx->conn_mgr_mutex);

	/* remove from the hash */
	axl_hash_remove (ctx->conn_mgr_hash, INT_TO_PTR (myqtt_conn_get_id (conn)));

	/* unlock */
	myqtt_mutex_unlock (&ctx->conn_mgr_mutex);


	return;
}

void myqttd_conn_mgr_unref (axlPointer data)
{
	/* get a reference to the state */
	MyQttdConnMgrState * state = data;
	MyQttdCtx          * ctx   = state->ctx;

	/* check connection status */
	if (state->conn) {
		/* uninstall on close full handler to avoid race conditions */
		myqtt_conn_remove_on_close (state->conn, myqttd_conn_mgr_on_close, ctx);

		/* unref the connection */
		msg ("Unregistering connection: %d (%p, socket: %d)", 
		     myqtt_conn_get_id ((MyQttConn*) state->conn), state->conn, myqtt_conn_get_socket (state->conn));
		myqtt_conn_unref ((MyQttConn*) state->conn, "myqttd-conn-mgr");
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
	myqtt_mutex_lock (&ctx->conn_mgr_mutex);
	
	/* create cursor */
	cursor = axl_hash_cursor_new (ctx->conn_mgr_hash);

	while (axl_hash_cursor_has_item (cursor)) {
		/* get connection */
		state = axl_hash_cursor_get_value (cursor);

		/* build connection status information */
		node  = axl_node_parse (NULL, "<row id='%d' host='%s' port='%s' local-addr='%s' local-port='%s' />",
					myqtt_conn_get_id (state->conn),
					myqtt_conn_get_host (state->conn),
					myqtt_conn_get_port (state->conn),
					myqtt_conn_get_local_addr (state->conn),
					myqtt_conn_get_local_port (state->conn));

		/* set node to result document */
		axl_node_set_child (parent, node);

		/* next cursor */
		axl_hash_cursor_next (cursor);
	} /* end while */
	
	/* free cursor */
	axl_hash_cursor_free (cursor);

	/* unlock connections */
	myqtt_mutex_unlock (&ctx->conn_mgr_mutex);
	
	return doc;
}


/** 
 * @internal Module init.
 */
void myqttd_conn_mgr_init (MyQttdCtx * ctx)
{
	/* init mutex */
	myqtt_mutex_create (&ctx->conn_mgr_mutex);

	/* init connection list hash */
	if (ctx->conn_mgr_hash == NULL) {
		
		ctx->conn_mgr_hash = axl_hash_new (axl_hash_int, axl_hash_equal_int);
	
	} /* end if */

	return;
}

void myqttd_conn_mgr_conn_list_free_item (axlPointer _conn)
{
	myqtt_conn_unref ((MyQttConn *) _conn, "conn-mgr-list");
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
					MyQttPeerRole          role,
					const char           * filter)
{
	axlList                * result;
	MyQttConn       * conn;
	axlHashCursor          * cursor;
	MyQttdConnMgrState * state;

	v_return_val_if_fail (ctx, NULL);

	/* lock and send */
	myqtt_mutex_lock (&ctx->conn_mgr_mutex);
	
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
		msg ("Checking connection role %d == %d", myqtt_conn_get_role (conn), role);
		if ((role == -1) || myqtt_conn_get_role (conn) == role) {
			/* update reference and add the connection */
			myqtt_conn_ref (conn, "conn-mgr-list");
			axl_list_append (result, conn);
		} /* end if */
		
		/* next cursor */
		axl_hash_cursor_next (cursor);
	}

	/* unlock */
	myqtt_mutex_unlock (&ctx->conn_mgr_mutex);

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
	myqtt_mutex_lock (&ctx->conn_mgr_mutex);

	/* get hash list */
	result = axl_hash_items (ctx->conn_mgr_hash);

	/* unlock */
	myqtt_mutex_unlock (&ctx->conn_mgr_mutex);	

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
 * myqtt_conn_set_data (conn, "tbc:proxy:conn", INT_TO_PTR (axl_true));
 * \endcode
 *
 */
axl_bool   myqttd_conn_mgr_proxy_on_parent (MyQttConn * conn)
{
	/* set up proxy configuration */
	return PTR_TO_INT (myqtt_conn_get_data (conn, "tbc:proxy:conn"));
}

/** 
 * Function used to read content from the connection and write that
 * content into the child socket.
 *
 *           Read                    Write
 *  [ Proxied connection ] --> [ Child socket ]
 */
void __myqttd_conn_mgr_proxy_reads (MyQttCtx * myqtt_ctx, MyQttConn * listener, MyQttConn * conn, MyQttConnOpts * opts, axlPointer user_data)
{
	char               buffer[4096];
	int                bytes_read;
	/* get socket associated */
	int                _socket = PTR_TO_INT (myqtt_conn_get_data (conn, "tbc:proxy:fd"));
	MyQttdCtx        * ctx     = user_data;

	/* check connection status */
	if (! myqtt_conn_is_ok (conn, axl_false)) 
		return;

	/* check status and close the other connection if found that */
	bytes_read = myqtt_msg_receive_raw (conn, (unsigned char *) buffer, 4096);

	/* msg ("PROXY-beep: Read %d bytes from conn-id=%d, sending them to child socket=%d (refs: %d, status: %d, errno=%d%s%s)",
	     bytes_read, myqtt_conn_get_id (conn), _socket,
	     myqtt_conn_ref_count (conn), myqtt_conn_is_ok (conn, axl_false), errno,
	     errno != 0 ? " : " : "",
	     errno != 0 ? strerror (errno) : "");  */

	if (bytes_read > 0) {
		/* send content */
		if (send (_socket, buffer, bytes_read, 0) != bytes_read) {
			wrn ("PROXY-beep: closing conn-id=%d because socket=%d isn't working", 
			     myqtt_conn_get_id (conn), _socket); 

			/* remove preread handler and shutdown */
			myqtt_conn_shutdown (conn);
			return;
		} /* end if */

		/* buffer[bytes_read] = 0;
		   msg ("PROXY-beep: sent content (beep conn-id=%d -> socket=%d): %s", myqtt_conn_get_id (conn), _socket, buffer); */
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
	    ! myqtt_conn_is_ok (conn, axl_false) || 
	    ! myqtt_msg_send_raw (conn, (unsigned char *) buffer, bytes_read)) {

		/* close socket and unregister it and close associated conn */
		wrn ("PROXY-fd: connection-id=%d is falling (wasn't able to send %d bytes, is zero?), closing associated socket=%d", 
		     myqtt_conn_get_id (conn), bytes_read, descriptor); 

		/* check connection status to finish it */
		if (myqtt_conn_is_ok (conn, axl_false))
			myqtt_conn_shutdown (conn);
		
		return axl_false;
	} /* end if */

	return axl_true; /* continue reading that socket */
}

axlPointer __myqttd_conn_mgr_release_proxy_conn (axlPointer conn)
{
	/* call to unref the connection */
	myqtt_conn_unref ((MyQttConn *) conn, "proxy-on-parent");
	return NULL;
}

void __myqttd_conn_mgr_proxy_on_close (MyQttConn * conn, axlPointer _loop)
{
	MyQttdLoop  * loop = _loop;
	/* MyQttdCtx   * ctx  = myqttd_loop_ctx (loop); */

	/* get socket associated */
	int               _socket = PTR_TO_INT (myqtt_conn_get_data (conn, "tbc:proxy:fd"));	

	/* msg ("PROXY: closing connection-id=%d, refs=%d, socket=%d", 
	   myqtt_conn_get_id (conn), myqtt_conn_ref_count (conn), _socket); */

	/* unregister socket from loop watcher */
	/* msg ("PROXY: calling to unwatch descriptor from loop _socket=%d (finished watching=%d)", _socket, myqttd_loop_watching (loop)); */
	myqttd_loop_unwatch_descriptor (loop, _socket, axl_true);
	/* msg ("PROXY: calling to unwatch descriptor from loop _socket=%d (finished watching=%d)", _socket, myqttd_loop_watching (loop)); */

	/* close socket */
	myqtt_close_socket (_socket);
	
	/* release and shutdown */
	conn->preread_handler   = NULL;
	conn->preread_user_data = NULL;

	/* reduce reference counting but do it out side the close handler */
	myqtt_thread_pool_new_task (CONN_CTX (conn), __myqttd_conn_mgr_release_proxy_conn, conn);

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
	if (myqtt_support_pipe (CONN_CTX (conn), descf) != 0) {
		error ("Failed to create pipe to proxy connection on the parent, check myqtt log to get more details");
		return -1;
	} /* end if */

	/* acquire a connection reference to ensure the loop's got it */
	if (! myqtt_conn_ref (conn, "proxy-on-parent")) {
		myqtt_close_socket (descf[0]);
		myqtt_close_socket (descf[1]);
		return -1;
	} /* end if */

	/* create the proxy loop watcher if it wasn't created yet */
	if (ctx->proxy_loop == NULL) 
		ctx->proxy_loop = myqttd_loop_create (ctx);

	/* watch the socket */
	myqttd_loop_watch_descriptor (ctx->proxy_loop, descf[1], __myqttd_conn_proxy_reads_loop, conn, NULL);

	/* configure links between both connections */
	myqtt_conn_set_data (conn,       "tbc:proxy:fd", INT_TO_PTR (descf[1]));
	myqtt_conn_set_data (conn,       "tbc:ctx", ctx);

	/* now configure preread handlers to pass data from both
	 * connections */
	conn->preread_handler   = __myqttd_conn_mgr_proxy_reads;
	conn->preread_user_data = ctx;

	/* setup connection close to cleanup */
	myqtt_conn_set_on_close (conn, axl_true, __myqttd_conn_mgr_proxy_on_close, ctx->proxy_loop);
	
	/* return the socket that will be using the child process */
	msg ("PROXY: Activated proxy on parent conn-id=%d (socket: %d), parent socket %d <--> child socket: %d", 
	     myqtt_conn_get_id (conn), myqtt_conn_get_socket (conn), descf[1], descf[0]);
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
	myqtt_mutex_lock (&ctx->conn_mgr_mutex);
	
	/* get the connection */
	state = axl_hash_get (ctx->conn_mgr_hash, INT_TO_PTR (conn_id));
	
	/* set conection */
	/* msg ("Connection find_by_id for conn id=%d returned pointer %p (conn: %p)", conn_id, state, state ? state->conn : NULL); */
	if (state)
		conn = state->conn;

	/* unlock */
	myqtt_mutex_unlock (&ctx->conn_mgr_mutex);

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
 * @internal Ensure we close all active connections before existing...
 */
axl_bool myqttd_conn_mgr_shutdown_connections (axlPointer key, axlPointer data, axlPointer user_data) 
{
	MyQttdConnMgrState * state = data;
	MyQttdCtx          * ctx   = state->ctx;
	MyQttConn       * conn  = state->conn;

	/* nullify conn reference on state */
	state->conn = NULL;

	/* uninstall on close full handler to avoid race conditions */
	myqtt_conn_remove_on_close (conn, myqttd_conn_mgr_on_close, ctx);

	msg ("shutting down connection id %d", myqtt_conn_get_id (conn));
	myqtt_conn_shutdown (conn);
	msg ("..socket status after shutdown: %d..", myqtt_conn_get_socket (conn));

	/* now unref */
	myqtt_conn_unref (conn, "myqttd-conn-mgr shutdown");

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
	myqtt_mutex_lock (&ctx->conn_mgr_mutex);
	conn_hash          = ctx->conn_mgr_hash;
	ctx->conn_mgr_hash = NULL;
	myqtt_mutex_unlock (&ctx->conn_mgr_mutex);

	/* finish the hash */
	axl_hash_foreach (conn_hash, myqttd_conn_mgr_shutdown_connections, NULL);

	/* destroy mutex */
	axl_hash_free (conn_hash);

	myqtt_mutex_destroy (&ctx->conn_mgr_mutex);

	return;
}

/** 
 * @}
 */
