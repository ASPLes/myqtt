/* 
 *  MyQtt: A high performance open source MQTT implementation
 *  Copyright (C) 2015 Advanced Software Production Line, S.L.
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
/* base myqtt */
#include <myqtt.h>

#if defined(ENABLE_TLS_SUPPORT)
/* include myqtt tls module */
#include <myqtt-tls.h>
#endif

#if defined(ENABLE_WEBSOCKET_SUPPORT)
#include <myqtt-web-socket.h>
#endif

#ifdef AXL_OS_UNIX
#include <signal.h>
#endif


#if defined(ENABLE_TLS_SUPPORT)
char * __certificate_handler (MyQttCtx * ctx,
			      MyQttConn * conn,
			      const char * serverName,
			      axlPointer   user_data)
{
	if (axl_cmp (serverName, "localhost"))
		return axl_strdup ("localhost-server.crt");

	if (axl_cmp (serverName, "test19a.localhost"))
		return axl_strdup ("test19a-localhost-server.crt");


	return NULL;
}

char * __private_handler  (MyQttCtx * ctx,
			   MyQttConn * conn,
			   const char * serverName,
			   axlPointer   user_data)
{
	if (axl_cmp (serverName, "localhost"))
		return axl_strdup ("localhost-server.key");
	if (axl_cmp (serverName, "test19a.localhost"))
		return axl_strdup ("test19a-localhost-server.key");

	return NULL;
}
char * __chain_handler  (MyQttCtx * ctx,
			 MyQttConn * conn,
			 const char * serverName,
			 axlPointer   user_data)
{
	return NULL;
}
#endif

/** 
 * IMPORTANT NOTE: do not include this header and any other header
 * that includes "private". They are used for internal definitions and
 * may change at any time without any notification. This header is
 * included on this file with the sole purpose of testing. 
 */
#include <myqtt-conn-private.h>
#include <myqtt-ctx-private.h>

/** global context used by the regression test */
MyQttCtx  * ctx;

#ifdef AXL_OS_UNIX
void __block_test (int value) 
{
	MyQttAsyncQueue * queue;

	printf ("******\n");
	printf ("****** Received a signal (the regression test is failing): pid %d..locking..!!!\n", myqtt_getpid ());
	printf ("******\n");

	/* block the caller */
	queue = myqtt_async_queue_new ();
	myqtt_async_queue_pop (queue);

	return;
}
#endif

axl_bool __doing_exit = axl_false;

void __terminate_myqtt_listener (int value)
{
	
	if (__doing_exit) 
		return;

	/* printf ("Terminating myqtt regression listener..\n");*/
	__doing_exit = axl_true;

	/* unlocking listener */
	/* printf ("Calling to unlock listener due to signal received: MyqttCtx %p", ctx); */
	myqtt_listener_unlock (ctx);

	return;
}

axl_bool test_common_enable_debug        = axl_false;
axl_bool test_common_enable_nopoll_debug = axl_false;

/* default listener location */
const char * listener_host     = "localhost";
const char * listener_port     = "1909";
const char * listener_tls_port = "1910";
const char * listener_websocket_port = "1918";
const char * listener_websocket_s_port = "1919";

MyQttCtx * init_ctx (void)
{
	MyQttCtx * ctx;

	/* call to init the base library and close it */
	ctx = myqtt_ctx_new ();

	if (! myqtt_init_ctx (ctx)) {
		printf ("Error: unable to initialize MyQtt library..\n");
		return NULL;
	} /* end if */

	/* enable log if requested by the user */
	if (test_common_enable_debug) {
		myqtt_log_enable (ctx, axl_true);
		myqtt_color_log_enable (ctx, axl_true);
		myqtt_log2_enable (ctx, axl_true);
	} /* end if */
		

	return ctx;
}

void __listener_sleep (long microseconds)
{
	MyQttAsyncQueue * queue;	
	queue = myqtt_async_queue_new ();
	myqtt_async_queue_timedpop (queue, microseconds);
	myqtt_async_queue_unref (queue);
	return;
}

MyQttPublishCodes on_publish (MyQttCtx * ctx, MyQttConn * conn, MyQttMsg * msg, axlPointer user_data)
{
	const char      * client_id;
	char            * reply_msg = NULL;
	char            * aux = NULL;
	char            * aux_value = NULL;
	char            * temp;
	axlHashCursor   * cursor = NULL;
	int               length;
	int               iterator;

	/* get current client identifier */
	if (axl_cmp ("get-subscriptions", myqtt_msg_get_topic (msg)) || axl_cmp ("get-subscriptions-ctx", myqtt_msg_get_topic (msg))) {

		if (axl_cmp ("get-subscriptions", myqtt_msg_get_topic (msg)))
			cursor = axl_hash_cursor_new (conn->subs);
		else if (axl_cmp ("get-subscriptions-ctx", myqtt_msg_get_topic (msg)))
			cursor = axl_hash_cursor_new (ctx->subs);

		/* iterate over all subscriptions */
		iterator = 0;
		while (iterator < 2) {
			
			while (axl_hash_cursor_has_item (cursor)) {
				if (axl_cmp ("get-subscriptions", myqtt_msg_get_topic (msg)))
					aux = axl_strdup_printf ("%s.%d", axl_hash_cursor_get_key (cursor), axl_hash_cursor_get_value (cursor));
				else if (axl_cmp ("get-subscriptions-ctx", myqtt_msg_get_topic (msg)))
					aux = axl_strdup_printf ("%s.num-conns=%d", axl_hash_cursor_get_key (cursor),
								 axl_hash_items (axl_hash_cursor_get_value (cursor)));
				
				if (aux_value) {
					temp = aux_value;
					aux_value = axl_strdup_printf ("%s,%s", temp, aux);
					axl_free (temp);
					axl_free (aux);
				} else {
					aux_value = aux;
				} /* end if */
				
				/* next cursor */
				axl_hash_cursor_next (cursor);
			}
			axl_hash_cursor_free (cursor);

			iterator++;

			/* now get wild card subscriptions */
			if (iterator == 1) {
				if (axl_cmp ("get-subscriptions", myqtt_msg_get_topic (msg)))
					cursor = axl_hash_cursor_new (conn->wild_subs);
				else if (axl_cmp ("get-subscriptions-ctx", myqtt_msg_get_topic (msg)))
					cursor = axl_hash_cursor_new (ctx->wild_subs);
			} /* end if */

			/* next iteration */
		} /* end while */
			
		printf ("Test --: subscriptions=%s\n", aux_value);
		length = 0;
		if (aux_value)
			length = strlen (aux_value);

		if (! myqtt_conn_pub (conn, myqtt_msg_get_topic (msg),
				      /* content */
				      aux_value ? (axlPointer) aux_value : "", 
				      /* content length */
				      length,
				      /* options */
				      MYQTT_QOS_0, axl_false, 0)) 
			printf ("ERROR: failed to publish get-server-name..\n");
		axl_free (aux_value);
		return MYQTT_PUBLISH_DISCARD; /* report received PUBLISH should be discarded */
	} /* end if */

	/* get current client identifier */
	if (axl_cmp ("myqtt/admin/get-server-name", myqtt_msg_get_topic (msg))) {
		/* send back serverName indicated */
		aux = (char *) myqtt_conn_get_server_name (conn);
		if (aux == NULL)
			aux = "";
		if (! myqtt_conn_pub (conn, "myqtt/admin/get-server-name", (axlPointer) aux, strlen (aux), MYQTT_QOS_0, axl_false, 0)) 
			printf ("ERROR: failed to publish get-server-name..\n");
		return MYQTT_PUBLISH_DISCARD; /* report received PUBLISH should be discarded */
	} /* end if */

	/* get current client identifier */
	if (axl_cmp ("myqtt/admin/get-tls-status", myqtt_msg_get_topic (msg))) {
		aux = myqtt_tls_is_on (conn) ? "tls-on-bro!" : "tls is not enabled brother";
		if (! myqtt_conn_pub (conn, "myqtt/admin/get-tls-status", aux, strlen (aux), MYQTT_QOS_0, axl_false, 0)) 
			printf ("ERROR: failed to publish get-tls-status..\n");
		return MYQTT_PUBLISH_DISCARD; /* report received PUBLISH should be discarded */
	} /* end if */

	/* get current client identifier */
	if (axl_cmp ("myqtt/admin/get-client-identifier", myqtt_msg_get_topic (msg))) {
		/* implement a wait so the caller can catch with myqtt_conn_get_nex () */
		__listener_sleep (1000000); /* 1 second */

		/* found administrative request, report this information and block publication */
		client_id = myqtt_conn_get_client_id (conn);
		if (! myqtt_conn_pub (conn, "myqtt/admin/get-client-identifier", (axlPointer) client_id, strlen (client_id), MYQTT_QOS_0, axl_false, 0)) 
			printf ("ERROR: failed to publish get-client-identifier..\n");
		return MYQTT_PUBLISH_DISCARD; /* report received PUBLISH should be discarded */
	} /* end if */

	/* get current user con */
	if (axl_cmp ("myqtt/admin/get-conn-user", myqtt_msg_get_topic (msg))) {
		/* implement a wait so the caller can catch with myqtt_conn_get_nex () */
		__listener_sleep (1000000); /* 1 second */

		if (! myqtt_conn_pub (conn, "myqtt/admin/get-conn-user", (axlPointer) myqtt_conn_get_username (conn), strlen (myqtt_conn_get_username (conn)), MYQTT_QOS_0, axl_false, 0))
			printf ("ERROR: failed to publish get-conn-user..\n");
		return MYQTT_PUBLISH_DISCARD; /* report received PUBLISH should be discarded */
	} /* end if */

	/* get current user con */
	if (axl_cmp ("myqtt/admin/get-conn-subs", myqtt_msg_get_topic (msg))) {
		/* implement a wait so the caller can catch with myqtt_conn_get_next () */
		__listener_sleep (1000000); /* 1 second */
		
		cursor = axl_hash_cursor_new (conn->subs);
		while (axl_hash_cursor_has_item (cursor)) {
			/* build message */
			if (reply_msg == NULL)
				reply_msg = axl_strdup_printf ("%d: %s\n", axl_hash_cursor_get_value (cursor), axl_hash_cursor_get_key (cursor));
			else {
				aux       = reply_msg;
				reply_msg = axl_strdup_printf ("%s%d: %s\n", reply_msg, axl_hash_cursor_get_value (cursor), axl_hash_cursor_get_key (cursor));
				axl_free (aux);
			}
				
			/* next position */
			axl_hash_cursor_next (cursor);
		} /* end while */

		axl_hash_cursor_free (cursor);

		printf ("replying: %s\n", reply_msg);

		if (! myqtt_conn_pub (conn, "myqtt/admin/get-conn-subs", reply_msg, strlen (reply_msg), MYQTT_QOS_0, axl_false, 0))
			printf ("ERROR: failed to publish get-conn-user..\n");
		axl_free (reply_msg);
		
		return MYQTT_PUBLISH_DISCARD; /* report received PUBLISH should be discarded */
	} /* end if */

	/* get current user con */
	if (axl_cmp ("myqtt/admin/get-queued-msgs", myqtt_msg_get_topic (msg))) {
		/* implement a wait so the caller can catch with myqtt_conn_get_nex () */
		__listener_sleep (1000000); /* 1 second */

		aux = axl_strdup_printf ("%d", myqtt_storage_queued_messages_offline (ctx, myqtt_msg_get_app_msg (msg)));
		printf ("Sending queued messages for %s: %s\n", (const char *) myqtt_msg_get_app_msg (msg), aux);
		if (! myqtt_conn_pub (conn, "myqtt/admin/get-queued-msgs", aux, strlen (aux), MYQTT_QOS_0, axl_false, 0))
			printf ("ERROR: failed to publish get-queued-messages..\n");
		axl_free (aux);
		return MYQTT_PUBLISH_DISCARD; /* report received PUBLISH should be discarded */
	} /* end if */

	/* get current user con */
	if (axl_cmp ("close/conn", myqtt_msg_get_topic (msg))) {
		printf ("Closing connection received as requested by the caller..\n");
		myqtt_conn_shutdown (conn);
		return MYQTT_PUBLISH_DISCARD; /* report received PUBLISH should be discarded */
	} /* end if */

	/* by default allow all publish operations */
	return MYQTT_PUBLISH_OK;
}

axl_bool reply_deferred (MyQttCtx * ctx, axlPointer _conn, axlPointer _user_data)
{
	MyQttConn * conn = _conn;

	/* send ok */
	printf ("Sending ok reply..\n");
	myqtt_conn_send_connect_reply (conn, MYQTT_CONNACK_ACCEPTED);

	myqtt_conn_unref (conn, "reply_deferred");

	return axl_true; /* finish this call */
}

/** 
 * @brief On connect function used to check and accept/reject
 * connections to this instance.
 */
MyQttConnAckTypes on_connect (MyQttCtx * ctx, MyQttConn * conn, axlPointer user_data)
{
	/* check support for auth operations */
	const char * username = myqtt_conn_get_username (conn);
	const char * password = myqtt_conn_get_password (conn);

	/* check for user and password (if provided).  */
	if (username && password) {
		if (! axl_cmp (username, "aspl") || !axl_cmp (password, "test")) {

			/* user and/or password is wrong */
			return MYQTT_CONNACK_BAD_USERNAME_OR_PASSWORD;
		}

	} /* end if */

	if (axl_cmp (myqtt_conn_get_client_id (conn), "test_23")) {
		printf ("Deferring connection accept for user aspl..\n");
		myqtt_conn_ref (conn, "reply_deferred");
		myqtt_thread_pool_new_event (ctx, 2000000, reply_deferred, conn, NULL);
		
		/* user and/or password is wrong */
		return MYQTT_CONNACK_DEFERRED;
	} /* end if */

	/* report connection accepted */
	return MYQTT_CONNACK_ACCEPTED;

} /* end on_connect */


/** 
 * @brief General regression test to check all features inside myqtt
 */
int main (int argc, char ** argv)
{
	MyQttConn     * listener;
	MyQttConnOpts * opts;
#if defined(ENABLE_WEBSOCKET_SUPPORT)
	noPollCtx     * nopoll_ctx;
	noPollConn    * nopoll_listener;
#endif

	printf ("** MyQtt: A high performance open source MQTT implementation\n");
	printf ("** Copyright (C) 2015 Advanced Software Production Line, S.L.\n**\n");
	printf ("** Regression test listener: %s \n",
		VERSION);

	/* install default handling to get notification about
	 * segmentation faults */
#ifdef AXL_OS_UNIX
	signal (SIGSEGV, __block_test);
	signal (SIGABRT, __block_test);
	signal (SIGTERM,  __terminate_myqtt_listener);
#endif

	/* uncomment the following four lines to get debug */
	while (argc > 0) {
		if (axl_cmp (argv[argc], "--help")) 
			exit (0);
		if (axl_cmp (argv[argc], "--debug")) 
			test_common_enable_debug = axl_true;
		if (axl_cmp (argv[argc], "--nopoll-debug")) 
			test_common_enable_nopoll_debug = axl_true;
		argc--;
	} /* end if */

	/* call to init the base library */
	ctx = init_ctx ();

	/* configure storage */
	printf ("Setting storage path on: .myqtt-listener\n");
	printf ("Cleaning storage... at .myqtt-listener .. wait a little bit please..\n");
	if (system ("find .myqtt-listener -type f -exec rm {} \\; > /dev/null 2>&1"))
		printf ("WARNING: clean subscriptions failed (reported non-zero value) (this is usual for the first time)\n");
	myqtt_storage_set_path (ctx, ".myqtt-listener", 4096);

	printf ("Path cleaning, starting listeners..\n");

	/* start a listener */
	listener = myqtt_listener_new (ctx, listener_host, listener_port, NULL, NULL, NULL);
	if (! myqtt_conn_is_ok (listener, axl_false)) {
		printf ("ERROR: failed to start listener at: %s:%s..\n", listener_host, listener_port);
		exit (-1);
	} /* end if */

#if defined(ENABLE_TLS_SUPPORT)
	/* start a TLS listener */
	listener = myqtt_tls_listener_new (ctx, listener_host, listener_tls_port, NULL, NULL, NULL);
	if (! myqtt_conn_is_ok (listener, axl_false)) {
		printf ("ERROR: failed to start TLS listener at: %s:%s..\n", listener_host, listener_tls_port);
		exit (-1);
	} /* end if */

	/* configure certificates to be used by this listener */
	if (! myqtt_tls_set_certificate (listener, "test-certificate.crt", "test-private.key", NULL)) {
		printf ("ERROR: unable to configure certificates for TLS myqtt..\n");
		exit (-1);
	}

	/* create connection options */
	opts     = myqtt_conn_opts_new ();

	/* configure server certificates (server.pem) signed by the
	 * provided ca (root.pem) also configured in the last
	 * parameter */
	if (! myqtt_tls_opts_set_ssl_certs (opts, 
					    "server.pem",
					    "server.pem",
					    NULL,
					    "root.pem")) {
		printf ("ERROR: unable to setup certificates...\n");
		return -1;
	}
	/* configure peer verification */
	myqtt_tls_opts_ssl_peer_verify (opts, axl_true);

	/* start a TLS listener */
	listener = myqtt_tls_listener_new (ctx, listener_host, "1911", opts, NULL, NULL);
	if (! myqtt_conn_is_ok (listener, axl_false)) {
		printf ("ERROR: failed to start TLS listener at: %s:%s..\n", listener_host, "1911");
		exit (-1);
	} /* end if */

	/* set certificate handlers */
	myqtt_tls_listener_set_certificate_handlers (ctx, 
						     __certificate_handler,
						     __private_handler,
						     __chain_handler,
						     NULL);

#endif

#if defined(ENABLE_WEBSOCKET_SUPPORT)
	printf ("INFO: starting MQTT over WebSocket at %s:%s\n", listener_host, listener_websocket_port);
	nopoll_ctx      = nopoll_ctx_new ();
	if (test_common_enable_nopoll_debug) {
		nopoll_log_enable (nopoll_ctx, axl_true);
		nopoll_log_color_enable (nopoll_ctx, axl_true);
	} /* end if */


	nopoll_listener = nopoll_listener_new (nopoll_ctx, listener_host, listener_websocket_port);
	if (! nopoll_conn_is_ok (nopoll_listener)) {
		printf ("ERROR: failed to start WebSocket listener at %s:%s..\n", listener_host, listener_websocket_port);
		return nopoll_false;
	} /* end if */

	/* now start listener */
	listener = myqtt_web_socket_listener_new (ctx, nopoll_listener, NULL, NULL, NULL);
	if (! myqtt_conn_is_ok (listener, axl_false)) {
		printf ("ERROR: failed to start WebSocket listener at: %s:%s..\n", listener_host, listener_websocket_port);
		exit (-1);
	} /* end if */

	/* create another listener */
	printf ("Starting WSS (MQTT over TLS WebSocket) %s:%s..\n", listener_host, listener_websocket_s_port);
	nopoll_listener = nopoll_listener_tls_new (nopoll_ctx, listener_host, listener_websocket_s_port);
	if (! nopoll_conn_is_ok (nopoll_listener)) {
		printf ("ERROR: failed to start WebSocket listener at %s:%s..\n", listener_host, listener_websocket_s_port);
		return nopoll_false;
	} /* end if */

	/* configure certificates.. */
	nopoll_listener_set_certificate (nopoll_listener, "test-certificate.crt", "test-private.key", NULL);

	/* now start listener */
	listener = myqtt_web_socket_listener_new (ctx, nopoll_listener, NULL, NULL, NULL);
	if (! myqtt_conn_is_ok (listener, axl_false)) {
		printf ("ERROR: failed to start WebSocket listener at: %s:%s..\n", listener_host, listener_websocket_s_port);
		exit (-1);
	} /* end if */

#endif

	/* install on publish handler */
	myqtt_ctx_set_on_publish (ctx, on_publish, NULL);
	myqtt_ctx_set_on_connect (ctx, on_connect, NULL);

	/* wait for listeners */
	printf ("Ready and accepting connections..OK\n");
	myqtt_listener_wait (ctx);

	/* now close the library */
	myqtt_exit_ctx (ctx, axl_true);

	/* terminate */
	return 0;
	
} /* end main */
