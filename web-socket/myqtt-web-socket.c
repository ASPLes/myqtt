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
#include <myqtt-web-socket.h>
#include <myqtt-conn-private.h>
#include <myqtt-listener-private.h>
#include <myqtt-ctx-private.h>

void __myqtt_web_socket_ctx_unref (axlPointer nopoll_ctx)
{
	nopoll_ctx_unref (nopoll_ctx);
	return;
}

void __myqtt_web_socket_associate_ctx (MyQttCtx * ctx, noPollCtx * nopoll_ctx)
{
	/* check if this context has already a context */
	if (myqtt_ctx_get_data (ctx, "__my:ws:ctx")) {
		return;
	}

	myqtt_mutex_lock (&ctx->ref_mutex);
	if (myqtt_ctx_get_data (ctx, "__my:ws:ctx")) {
		myqtt_mutex_unlock (&ctx->ref_mutex);
		return;
	} /* end if */

	/* associate context */
	myqtt_ctx_set_data_full (ctx, "__my:ws:ctx", nopoll_ctx, 
				 NULL, (axlDestroyFunc) __myqtt_web_socket_ctx_unref);

	myqtt_mutex_unlock (&ctx->ref_mutex);
	return;
}

/* @internal Function used to read content from the provided
   connection.
 */
int __myqtt_web_socket_receive (MyQttConn     * conn,
				unsigned char * buffer,
				int             buffer_len)
{
	noPollConn * _conn;
	int          result;
	MyQttCtx   * ctx;
	MyQttMutex * mutex;

	/* check if the connection has the greetings completed and it
	 * is initiator role */
	_conn = myqtt_conn_get_data (conn, "__my:ws:conn");

	ctx = conn->ctx;

	/* get mutex */
	mutex = myqtt_conn_get_data (conn, "ws:mutex");
	if (mutex == NULL) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "unable to find mutex to protect ssl object to read data");
		return -1;
	} /* end if */

	/* call to acquire mutex, read and release */
	myqtt_mutex_lock (mutex);
	result = nopoll_conn_read (_conn, (char *) buffer, buffer_len, nopoll_false, 0);
	myqtt_mutex_unlock (mutex);

	if (result == -1) {

		/* check connection status to notify that no data was
		 * available  */
		if (nopoll_conn_is_ok (_conn)) 
			return -2; 

		myqtt_log (MYQTT_LEVEL_CRITICAL, "Found noPollConn-id=%d (%p) read error myqtt conn-id=%d (session: %d), errno=%d (shutting down)",
			   nopoll_conn_get_id (_conn), _conn, myqtt_conn_get_id (conn), myqtt_conn_get_socket (conn), errno);

		/* shutdown connection */
		nopoll_conn_set_socket (_conn, -1);
		myqtt_conn_shutdown (conn);
	} /* end if */

	return result;
}

/** 
 * @internal Function used to send content from the associated
 * websocket connection
 */
int __myqtt_web_socket_send (MyQttConn            * conn,
			     const unsigned char  * buffer,
			     int                    buffer_len)
{
	noPollConn  * _conn = myqtt_conn_get_data (conn, "__my:ws:conn");
	MyQttCtx    * ctx;
	MyQttMutex  * mutex;
	int           result;

	/* get a reference to the context */
	ctx = conn->ctx;

	/* get mutex */
	mutex = myqtt_conn_get_data (conn, "ws:mutex");
	if (mutex == NULL) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "unable to find mutex to protect noPoll WebSocket object to read data");
		return -1;
	} /* end if */

	/* acquire lock, operate and release */
	myqtt_mutex_lock (mutex);
	result = nopoll_conn_send_text (_conn, (const char *) buffer, buffer_len);

	/* ensure we have written all bytes but limit operation to 2 seconds */
	result = nopoll_conn_flush_writes (_conn, 2000000, result);

	myqtt_mutex_unlock (mutex);

	return result;
}

void __myqtt_web_socket_nopoll_on_close (noPollCtx * _ctx, noPollConn * conn, noPollPtr ptr)
{
	/* remove reference to avoid future close of a descriptor with
	   the same value */
	nopoll_conn_set_socket (conn, -1);
	
	return;
}

/** 
 * @internal Internal function to release and free the mutex memory
 * allocated.
 * 
 * @param mutex The mutex to destroy.
 */
void __myqtt_web_socket_free_mutex (MyQttMutex * mutex)
{
	/* free mutex */
	myqtt_mutex_destroy (mutex);
	axl_free (mutex);
	return;
}

void __myqtt_web_socket_close_conn (noPollConn * conn)
{
	nopoll_conn_shutdown (conn);
	nopoll_conn_close (conn);
	return;
}

void __myqtt_web_socket_common_association (MyQttConn * conn, noPollConn * nopoll_conn)
{
	MyQttMutex * mutex;

	/* associate connection */
	myqtt_conn_set_data_full (conn, "__my:ws:conn", nopoll_conn, 
				  NULL, (axlDestroyFunc) __myqtt_web_socket_close_conn);

	/* configure default handlers */
	conn->receive = __myqtt_web_socket_receive;
	conn->send    = __myqtt_web_socket_send;

	/* setup I/O handlers */
	mutex = axl_new (MyQttMutex, 1);
	myqtt_mutex_create (mutex);
	myqtt_conn_set_data_full (conn, "ws:mutex", mutex,
				  NULL, (axlDestroyFunc) __myqtt_web_socket_free_mutex);

	/* setup on close handler to control sockets */
	nopoll_conn_set_on_close (nopoll_conn, __myqtt_web_socket_nopoll_on_close, conn);

	return;
}

axl_bool __myqtt_web_socket_session_setup (MyQttCtx * ctx, MyQttConn * conn, MyQttConnOpts * options, axlPointer user_data)
{
	noPollConn  * nopoll_conn = user_data;

	myqtt_log (MYQTT_LEVEL_DEBUG, "Creating new MQTT over WebSocket connection to %s:%s (socket: %d)",
		   nopoll_conn_host (nopoll_conn), nopoll_conn_port (nopoll_conn), nopoll_conn_socket (nopoll_conn));

	/* set socket on the connection */
	conn->session = nopoll_conn_socket (nopoll_conn);

	/* associate the connection to its websocket transport */
	__myqtt_web_socket_common_association (conn, nopoll_conn);

	/* configure this connection to have no preread */
	conn->preread_handler   = NULL;
	conn->preread_user_data = NULL;

	return axl_true;
}


/** 
 * @brief Allows to create a new MQTT connection to a MQTT
 * broker/server running MQTT over WebSocket.
 *
 * @param The context where the operation will take place.
 *
 * @param client_identifier The client identifier that uniquely
 * identifies this MQTT client from others.  It can be NULL to let
 * MQTT 3.1.1 servers to assign a default client identifier BUT
 * clean_session must be set to axl_true. This is done automatically
 * by the library (setting clean_session to axl_true when NULL is
 * provided).
 *
 * @param clean_session Flag to clean client session or to reuse the
 * existing one. If set to axl_false, you must provide a valid
 * client_identifier (otherwise the function will fail).
 *
 * @param keep_alive Keep alive configuration in seconds after which
 * the server/broker will close the connection if it doesn't detect
 * any activity. Setting 0 will disable keep alive mechanism.
 *
 * @param conn A reference to an established noPollConn connection
 * that is connecting to the MQTT over WebSocket server we want to
 * connect to. 
 *
 * @param opts Optional connection options. See \ref myqtt_conn_opts_new
 *
 * @param on_connected Async notification handler that will be called
 * once the connection fails or it is completed. In the case this
 * handler is configured the caller will not be blocked. In the case
 * this parameter is NULL, the caller will be blocked until the
 * connection completes or fails.
 *
 * @param user_data User defined pointer that will be passed to the on_connected handler (in case it is defined).
 *
 * @return A reference to the newli created connection or NULL if
 * on_connected handler is provided. In both cases, the reference
 * returned (or received at the on_connected handler) must be checked
 * with \ref myqtt_conn_is_ok. 
 *
 * <b>About pending messages / queued messages </b>
 *
 * After successful connection with clean_session set to axl_false and
 * defined client identifier, the library will resend any queued or in
 * flight QoS1/QoS2 messages (as well as QoS0 if they were
 * stored). This is done in background without intefering the caller.
 *
 * If you need to get the number of queued messages that are going to
 * be sent use \ref myqtt_storage_queued_messages_offline. In the case
 * you need the number remaining during the process use \ref
 * myqtt_storage_queued_messages.
 *
 * See \ref myqtt_conn_new for more information.
 *
 * <b>About reconnecting</b>
 *
 * If you enable automatic reconnect support after connection close
 * (\ref myqtt_conn_opts_set_reconnect), remember to also configure
 * the recover handler by using \ref
 * myqtt_conn_opts_set_recover_session_setup_ptr. That function should
 * implement a complete reconnect and return a noPollConn reference
 * used by the internal session setup. If you don't configure this,
 * the function will disable reconnect support even if you enable it.
 *
 */
MyQttConn        * myqtt_web_socket_conn_new            (MyQttCtx        * ctx,
							 const char      * client_identifier,
							 axl_bool          clean_session,
							 int               keep_alive,
							 noPollConn      * conn,
							 MyQttConnOpts   * opts,
							 MyQttConnNew      on_connected, 
							 axlPointer        user_data)
{

	/* check if the conn reference is not defined. In that case,
	   try to craete it with the init session setup ptr */
	if (conn == NULL && opts && opts->init_session_setup_ptr)
		conn = opts->init_session_setup_ptr (ctx, NULL, opts->init_user_data, opts->init_user_data2, opts->init_user_data3);

	/* report what we are doing */
	myqtt_log (MYQTT_LEVEL_DEBUG, "Creating new MQTT over WebSocket connection to %s:%s (is ready:%d)",
		   nopoll_conn_host (conn), nopoll_conn_port (conn), nopoll_conn_is_ready (conn));

	/* check and disable reconnect if it is not configured the
	   recover handler */
	if (opts && opts->reconnect) {
		if (opts->init_session_setup_ptr == NULL) {
			myqtt_log (MYQTT_LEVEL_CRITICAL, "Disable reconnect flag because user didn't provide a recover handler (myqtt_conn_opts_set_recover_session_setup_ptr)");
			opts->reconnect = axl_false; /* disable it */
		} /* end if */
	} /* end opts */

	/* associate context */
	__myqtt_web_socket_associate_ctx (ctx, nopoll_conn_ctx (conn));

	/* call to create the connection */
	return myqtt_conn_new_full_common (ctx, client_identifier, clean_session, keep_alive, 
					   /* destination host and port but only as a matter of reporting 
					      because we are handling everything through the setup handler */
					   nopoll_conn_host (conn), nopoll_conn_port (conn), 
					   /* this is the handler that will establish the connection on top
					      of the provided noPoll connection */
					   __myqtt_web_socket_session_setup, conn, 
					   /* additional user handlers */
					   on_connected, -1, opts, user_data);
}

nopoll_bool __myqtt_web_socket_listener_on_ready (noPollCtx * ctx, noPollConn * nopoll_conn, noPollPtr user_data)
{
	MyQttConn * conn = user_data;
	
	/* setup websocket header */
	myqtt_conn_set_server_name (conn, nopoll_conn_get_host_header (nopoll_conn));

	/* check if we have TLS running to set that flag in the
	   expected place */
	if (nopoll_conn_is_tls_on (nopoll_conn))
		conn->tls_on = axl_true; /* signal it */

	return nopoll_true; /* accept connection */
}

void __myqtt_web_socket_accept_connection (MyQttCtx * ctx, MyQttConn * listener, MyQttConn * conn, MyQttConnOpts * opts, axlPointer user_data)
{
	noPollConn * nopoll_listener;
	noPollConn * nopoll_conn;
	noPollCtx  * nopoll_ctx;

	/* ok, setup noPoll connection here and configure default read
	   and write operations over this new connection */
	
	/* get the listener associated to this MyQTT listener */
	nopoll_listener = myqtt_conn_get_data (listener, "__my:ws:lstnr");
	if (nopoll_listener == NULL) {
		myqtt_log (MYQTT_LEVEL_WARNING, "Listener is still not ready, rejecting conection");
		myqtt_conn_shutdown (conn);
		return;
	} /* end if */

	/* get a reference to the context */
	nopoll_ctx  = nopoll_conn_ctx (nopoll_listener);
	if (nopoll_ctx == NULL) {
		myqtt_log (MYQTT_LEVEL_WARNING, "Failed to accept connection, unable to find nopPollCtx from given Listener, rejecting conection");
		myqtt_conn_shutdown (conn);
		return;
	} /* end if */

	/* now accept new connection using provided socket on conn
	   (which has been accepted by the MyQTT engine) */
	nopoll_conn = nopoll_conn_accept_socket (nopoll_ctx, nopoll_listener, myqtt_conn_get_socket (conn));
	if (nopoll_conn == NULL) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Received null pointer from nopoll_conn_accept, rejecting connection");
		myqtt_conn_shutdown (conn);
		return;
	} /* end if */

	/* set on ready on this connection to setup received host header */
	nopoll_conn_set_on_ready (nopoll_conn, __myqtt_web_socket_listener_on_ready, conn);

	/* associate the connection to its websocket transport */
	__myqtt_web_socket_common_association (conn, nopoll_conn);

	/* configure this connection to have no preread */
	conn->preread_handler   = NULL;
	conn->preread_user_data = NULL;

	return;
}

typedef struct _MyQttWebSocketReady {
	axlPointer           user_data;
	noPollConn         * listener;
	MyQttListenerReady   user_on_ready;
} MyQttWebSocketReady;

void __myqtt_web_socket_listener_ready (const char   * host, 
					int            port, 
					MyQttStatus    status, 
					char         * message, 
					MyQttConn    * listener,
					axlPointer     _user_data)
{
	MyQttWebSocketReady * websocket_ready = _user_data;
	MyQttCtx            * ctx;

	/* configure listener with the right listener reference */
	ctx = listener->ctx;
	myqtt_log (MYQTT_LEVEL_DEBUG, "Websocket listener started at: %s:%s", 
		   myqtt_conn_get_host (listener), myqtt_conn_get_port (listener));
	myqtt_conn_set_data_full (listener, "__my:ws:lstnr", websocket_ready->listener,
				  NULL, (axlDestroyFunc) __myqtt_web_socket_close_conn);

	/* call user defined on ready function if defined */
	if (websocket_ready->user_on_ready)
		websocket_ready->user_on_ready (host, port, status, message, listener, websocket_ready->user_data);

	/* release data */
	axl_free (websocket_ready);

	return;
}


/** 
 * @brief Allows to start a MQTT server on the provided local host
 * address and port running MQTT over WebSocket protocol.
 *
 * <b>Important note:</b> you must call to \ref myqtt_storage_set_path
 * to define the path first before creating any listener. This is
 * because creating a listener activates all server side code which
 * among other things includes the storage loading (client
 * subscriptions, offline publishing, etc). In the case direction,
 * once the storage path is loaded it cannot be changed after
 * restarting the particular context used in this operation (\ref
 * MyQttCtx).
 *
 * @param ctx The context where the operation takes place.
 *
 * @param host The local host address to list for incoming connections. 
 *
 * @param port The local port to listen on.
 *
 * @param opts Optional connection options to modify default behaviour.
 *
 * @param on_ready Optional on ready notification handler that gets
 * called when the listener is created or a failure was
 * found. Providing this handler makes this function to not block the
 * caller.
 *
 * @param user_data Optional user defined pointer that is passed into
 * the on_ready function (in the case the former is defined too).
 *
 * See \ref myqtt_listener_new for more information.
 */
MyQttConn       * myqtt_web_socket_listener_new         (MyQttCtx             * ctx,
							 noPollConn           * listener,
							 MyQttConnOpts        * opts,
							 MyQttListenerReady     on_ready, 
							 axlPointer             user_data)
{
	MyQttWebSocketReady * websocket_ready = NULL;
	MyQttConn           * myqtt_listener;

	if (! nopoll_conn_is_ok (listener)) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Unable to create listener, received reference is not working");
		return NULL;
	} /* end if */

	/* create object for the function proxy but only in the user
	   on_ready function is defined */
	if (on_ready) {
		websocket_ready = axl_new (MyQttWebSocketReady, 1);
		if (websocket_ready == NULL) {
			myqtt_log (MYQTT_LEVEL_CRITICAL, "Unable to allocate memory to start the listener..");
			return NULL;
		} /* end if */
		
		websocket_ready->user_data     = user_data;
		websocket_ready->listener      = listener;
		websocket_ready->user_on_ready = on_ready;
	} /* end if */

	/* associate context */
	__myqtt_web_socket_associate_ctx (ctx, nopoll_conn_ctx (listener));

	myqtt_listener = __myqtt_listener_new_common (ctx, nopoll_conn_host (listener), 
						      __myqtt_listener_get_port (nopoll_conn_port (listener)), 
						      /* register connection and socket */
						      axl_true, nopoll_conn_socket (listener), 
						      opts, on_ready, MYQTT_IPv6, __myqtt_web_socket_accept_connection, 
						      /* configure proxy on ready */
						      __myqtt_web_socket_listener_ready, websocket_ready);

	if (myqtt_listener) {
		/* configure here reference to the noPoll listener */
		myqtt_conn_set_data_full (myqtt_listener, "__my:ws:lstnr", listener,
					  NULL, (axlDestroyFunc) __myqtt_web_socket_close_conn);

		/* configure on ready */
	} /* end if */

	return myqtt_listener;
}

/** 
 * @brief Allows to get the associated noPoll (WebSocket) connection
 * from the provided MyQttConn object.
 *
 * @param conn The MyQtt connection for which we want access to the
 * noPollConn object.
 *
 * @return A reference to the associated noPollConn or NULL if it
 * fails.
 */
noPollConn      * myqtt_web_socket_get_conn             (MyQttConn * conn)
{
	return myqtt_conn_get_data (conn, "__my:ws:conn");
}

/** 
 * @brief Allows to return the noPollCtx object associated to the
 * provided connection.
 *
 * @param ctx The MyQttCtx for which we want access to the noPollCtx
 * object.
 *
 * @return A reference to the associated noPollCtx or NULL if it
 * fails.
 */
noPollCtx       * myqtt_web_socket_get_ctx              (MyQttCtx * ctx)
{
	noPollCtx * nopoll_ctx;

	if (ctx == NULL)
		return NULL;

	/* get current nopoll context configured */
	myqtt_mutex_lock (&ctx->ref_mutex);
	nopoll_ctx =  myqtt_ctx_get_data (ctx, "__my:ws:ctx");
	if (nopoll_ctx) {
		/* release lock */
		myqtt_mutex_unlock (&ctx->ref_mutex);

		return nopoll_ctx;
	} /* end if */

	/* reached this point, it looks like no context is configured,
	   create one and associate */
	nopoll_ctx = nopoll_ctx_new ();

	/* associate context */
	myqtt_ctx_set_data_full (ctx, "__my:ws:ctx", nopoll_ctx, 
				 NULL, (axlDestroyFunc) __myqtt_web_socket_ctx_unref);

	myqtt_mutex_unlock (&ctx->ref_mutex);

	return nopoll_ctx;
}

/* @} */

