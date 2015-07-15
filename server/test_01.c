/* 
 *  MyQtt: A high performance open source MQTT implementation
 *  Copyright (C) 2015 Advanced Software Production Line, S.L.
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

#if defined(ENABLE_TLS_SUPPORT)
/* include tls support */
#include <myqtt-tls.h>
#endif

#if defined(ENABLE_WEBSOCKET_SUPPORT)
/* include web socket support */
#include <myqtt-web-socket.h>

/* default listener location */
const char * listener_ws_host = "localhost";
const char * listener_ws_port = "1181";
const char * listener_wss_port = "1191";

#endif

/* these includes are prohibited in a production ready code. They are
   just used to check various internal aspects of the MyQtt
   implementation. If you need your code to use them to work keep in
   mind you are referencing internal structures and definition that
   may change at any time */
#include <myqttd-ctx-private.h>
#include <myqtt-ctx-private.h>

#ifdef AXL_OS_UNIX
#include <signal.h>
#endif

/* local test header */
#include <test_01.h>

axl_bool test_common_enable_debug = axl_false;

/* default listener location */
const char * listener_host = "localhost";
const char * listener_port = "1883";

#if defined(ENABLE_TLS_SUPPORT)
const char * listener_tls_port = "8883";
#endif


MyQttCtx * common_init_ctx (void)
{
	MyQttCtx * ctx;

	/* call to init the base library and close it */
	ctx = myqtt_ctx_new ();

	/* enable log if requested by the user */
	if (test_common_enable_debug) {
		myqtt_log_enable (ctx, axl_true);
		myqtt_color_log_enable (ctx, axl_true);
		myqtt_log2_enable (ctx, axl_true);
	} /* end if */

	return ctx;
}

MyQttdCtx * common_init_ctxd (MyQttCtx * myqtt_ctx, const char * config)
{
	MyQttdCtx * ctx;

	/* call to init the base library and close it */
	ctx = myqttd_ctx_new ();

	/* enable log if requested by the user */
	if (test_common_enable_debug) {
		myqttd_log_enable (ctx, axl_true);
		myqttd_color_log_enable (ctx, axl_true);
		myqttd_log2_enable (ctx, axl_true);
	} /* end if */

	if (! myqttd_init (ctx, myqtt_ctx, config)) {
		printf ("Error: unable to initialize MyQtt library..\n");
		return NULL;
	} /* end if */

	/* enable log if requested by the user */
	if (test_common_enable_debug) {
		myqtt_log_enable (MYQTTD_MYQTT_CTX (ctx), axl_true);
		myqtt_color_log_enable (MYQTTD_MYQTT_CTX (ctx), axl_true);
		myqtt_log2_enable (MYQTTD_MYQTT_CTX (ctx), axl_true);
	} /* end if */

	/* now run config */
	if (! myqttd_run_config (ctx)) {
		printf ("Failed to run current config, finishing process: %d", getpid ());
		return NULL;
	} /* end if */

	return ctx;
}

axl_bool  test_00 (void) {

	MyQttdCtx * ctx;
	MyQttCtx  * myqtt_ctx;
	
	/* call to init the base library and close it */
	printf ("Test 00: init library and server engine..\n");
	myqtt_ctx = common_init_ctx ();
	ctx       = common_init_ctxd (myqtt_ctx, "myqtt.example.conf");
	if (ctx == NULL) {
		printf ("Test 00: failed to start library and server engine..\n");
		return axl_false;
	} /* end if */

	printf ("Test 00: library and server engine started.. ok\n");

	/* now close the library */
	myqttd_exit (ctx, axl_true, axl_true);
		
	return axl_true;
}

void common_queue_message_received (MyQttCtx * ctx, MyQttConn * conn, MyQttMsg * msg, axlPointer user_data)
{
	MyQttAsyncQueue * queue = user_data;

	/* push message received */
	printf ("Test --: queue received %p, msg=%p, msg-id=%d\n", queue, msg, myqtt_msg_get_id (msg));
	myqtt_msg_ref (msg);
	myqtt_async_queue_push (queue, msg);
	return;
} 

void common_queue_message_received_only_one (MyQttCtx * ctx, MyQttConn * conn, MyQttMsg * msg, axlPointer user_data)
{
	MyQttAsyncQueue * queue = user_data;

	/* push message received */
	if (! myqtt_msg_ref (msg)) 
		printf ("ERROR: failed to acquire msg reference, ref count is: %d (pointer %p)\n", 
			myqtt_msg_ref_count (msg), msg);
	myqtt_async_queue_push (queue, msg);
	return;
} 

axl_bool  test_01 (void) {
	MyQttdCtx       * ctx;
	MyQttConn       * conn;
	MyQttCtx        * myqtt_ctx;
	int               sub_result;
	MyQttAsyncQueue * queue;
	MyQttMsg        * msg;

	const char      * test_message = "This is test message (test-01)....";

	/* cleanup test_01 storage */
	if (system ("find reg-test-01/storage/test_01 -type f  -exec rm {} \\;") != 0) {
		printf ("ERROR: failed to initialize test..\n");
		return axl_false;
	} /* end if */
	
	/* call to init the base library and close it */
	printf ("Test 01: init library and server engine..\n");
	ctx       = common_init_ctxd (NULL, "test_01.conf");
	if (ctx == NULL) {
		printf ("Test 00: failed to start library and server engine..\n");
		return axl_false;
	} /* end if */

	printf ("Test 01: library and server engine started.. ok (ctxd = %p, ctx = %p\n", ctx, MYQTTD_MYQTT_CTX (ctx));

	/* create connection to local server and test domain support */
	myqtt_ctx = common_init_ctx ();
	if (! myqtt_init_ctx (myqtt_ctx)) {
		printf ("Error: unable to initialize MyQtt library..\n");
		return axl_false;
	} /* end if */

	printf ("Test 01: connecting to myqtt server (client ctx = %p)..\n", myqtt_ctx);
	conn = myqtt_conn_new (myqtt_ctx, "test_01", axl_true, 30, listener_host, listener_port, NULL, NULL, NULL);
	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: unable to connect to %s:%s..\n", listener_host, listener_port);
		return axl_false;
	} /* end if */

	/* subscribe */
	printf ("Test 01: subscribe to the topic myqtt/test..\n");
	if (! myqtt_conn_sub (conn, 10, "myqtt/test", 0, &sub_result)) {
		printf ("ERROR: unable to subscribe, myqtt_conn_sub () failed, sub_result=%d", sub_result);
		return axl_false;
	} /* end if */	

	printf ("Test 01: subscription completed with qos=%d\n", sub_result);
	
	/* register on message handler */
	queue = myqtt_async_queue_new ();
	myqtt_conn_set_on_msg (conn, common_queue_message_received, queue);

	/* push a message */
	printf ("Test 01: publishing to the topic myqtt/test..\n");
	if (! myqtt_conn_pub (conn, "myqtt/test", (axlPointer) test_message, strlen (test_message), MYQTT_QOS_0, axl_false, 0)) {
		printf ("ERROR: unable to publish message, myqtt_conn_pub() failed\n");
		return axl_false;
	} /* end if */

	/* receive it */
	printf ("Test 01: waiting for message myqtt/test (5 seconds)..\n");
	msg   = myqtt_async_queue_timedpop (queue, 5000000);
	myqtt_async_queue_unref (queue);
	if (msg == NULL) {
		printf ("ERROR: expected to find message from queue, but NULL was found..\n");
		return axl_false;
	} /* end if */

	/* check content */
	printf ("Test 01: received reply...checking data..\n");
	if (myqtt_msg_get_app_msg_size (msg) != strlen (test_message)) {
		printf ("ERROR: expected payload size of %d but found %d\n", 
			(int) strlen (test_message),
			(int) myqtt_msg_get_app_msg_size (msg));
		return axl_false;
	} /* end if */

	if (myqtt_msg_get_type (msg) != MYQTT_PUBLISH) {
		printf ("ERROR: expected to receive PUBLISH message but found: %s\n", myqtt_msg_get_type_str (msg));
		return axl_false;
	} /* end if */

	/* check content */
	if (! axl_cmp ((const char *) myqtt_msg_get_app_msg (msg), test_message)) {
		printf ("ERROR (test-01 dkfrf): expected to find different content. Received '%s', expected '%s'..\n",
			(const char *) myqtt_msg_get_app_msg (msg), test_message);
		return axl_false;
	} /* end if */

	printf ("Test 01: everything is ok, finishing context and connection..\n");
	myqtt_msg_unref (msg);

	/* close connection */
	myqtt_conn_close (conn);
	myqtt_exit_ctx (myqtt_ctx, axl_true);

	printf ("Test 01: finishing MyQttdCtx..\n");

	/* finish server */
	myqttd_exit (ctx, axl_true, axl_true);
		
	return axl_true;
}


axl_bool  test_02 (void) {
	MyQttdCtx       * ctx;
	MyQttCtx        * myqtt_ctx;
	int               sub_result;
	MyQttMsg        * msg;
	MyQttAsyncQueue * queue;
	MyQttConn       * conns[50];
	int               iterator;
	char            * client_id;

	/* cleanup test_01 storage */
	if (system ("find reg-test-01/storage/test_01 -type f  -exec rm {} \\;") != 0) {
		printf ("ERROR: failed to initialize test..\n");
		return axl_false;
	} /* end if */
	
	/* call to init the base library and close it */
	printf ("Test 02: init library and server engine..\n");
	ctx       = common_init_ctxd (NULL, "test_02.conf");
	if (ctx == NULL) {
		printf ("Test 00: failed to start library and server engine..\n");
		return axl_false;
	} /* end if */

	printf ("Test 02: library and server engine started.. ok (ctxd = %p, ctx = %p\n", ctx, MYQTTD_MYQTT_CTX (ctx));

	/* create connection to local server and test domain support */
	myqtt_ctx = common_init_ctx ();
	if (! myqtt_init_ctx (myqtt_ctx)) {
		printf ("Error: unable to initialize MyQtt library..\n");
		return axl_false;
	} /* end if */


	printf ("Test 02: connecting to myqtt server (client ctx = %p, 50 connections)..\n", myqtt_ctx);
	iterator = 0;
	while (iterator < 50) {
		client_id       = axl_strdup_printf ("test_02_%d", iterator);
		conns[iterator] = myqtt_conn_new (myqtt_ctx, client_id, axl_true, 30, listener_host, listener_port, NULL, NULL, NULL);
		if (! myqtt_conn_is_ok (conns[iterator], axl_false)) {
			printf ("ERROR: unable to connect to %s:%s..\n", listener_host, listener_port);
			return axl_false;
		} /* end if */
		axl_free (client_id);

		/* next iterator */
		iterator++;
	} /* end while */

	/* register on message handler */
	queue = myqtt_async_queue_new ();

	/* subscribe */
	printf ("Test 02: subscribe to the topic myqtt/test (in 50 connections)..\n");
	iterator = 0;
	while (iterator < 50) {
		if (! myqtt_conn_sub (conns[iterator], 10, "myqtt/test", 0, &sub_result)) {
			printf ("ERROR: unable to subscribe, myqtt_conn_sub () failed, sub_result=%d", sub_result);
			return axl_false;
		} /* end if */	

		myqtt_conn_set_on_msg (conns[iterator], common_queue_message_received_only_one, queue);

		/* next iterator */
		iterator++;

	} /* end while */


	printf ("Test 02: publishing to the topic myqtt/test (on 50 connections)..\n");
	iterator = 0;
	while (iterator < 50) {

		/* push a message */
		if (! myqtt_conn_pub (conns[iterator], "myqtt/test", "This is test message (test-02)....", 34, MYQTT_QOS_0, axl_false, 0)) {
			printf ("ERROR: unable to publish message, myqtt_conn_pub() failed\n");
			return axl_false;
		} /* end if */

		/* next iterator */
		iterator++;
	}

	printf ("Test 02: wait for 2500 messages (50 messages for each connection)\n");
	iterator = 0;
	while (iterator < 2500) {

		/* receive it */
		msg = myqtt_async_queue_timedpop (queue, 5000000);
		if (msg == NULL) {
			printf ("ERROR: expected to find message from queue, but NULL was found..\n");
			return axl_false;
		} /* end if */
		
		/* check content */
		if (myqtt_msg_get_app_msg_size (msg) != 34) {
			printf ("ERROR: expected payload size of 34 but found %d\n", myqtt_msg_get_app_msg_size (msg));
			return axl_false;
		} /* end if */
		
		if (myqtt_msg_get_type (msg) != MYQTT_PUBLISH) {
			printf ("ERROR: expected to receive PUBLISH message but found: %s\n", myqtt_msg_get_type_str (msg));
			return axl_false;
		} /* end if */
		
		/* check content */
		if (! axl_cmp ((const char *) myqtt_msg_get_app_msg (msg), "This is test message (test-02)....")) {
			printf ("ERROR: expected to find different content..\n");
			return axl_false;
		} /* end if */

		myqtt_msg_unref (msg);

		/* next iterator */
		iterator++;
	}

	printf ("Test 02: everything is ok, finishing context and connection (iterator=%d)..\n", iterator);

	iterator = 0;
	while (iterator < 50) {

		/* close connection */
		myqtt_conn_close (conns[iterator]);

		/* next iterator */
		iterator++;
	}

	/* receive it */
	printf ("Test 02: releasing queue %p\n", queue);
	myqtt_async_queue_unref (queue);

	myqtt_exit_ctx (myqtt_ctx, axl_true);

	printf ("Test 02: finishing MyQttdCtx..\n");

	/* finish server */
	myqttd_exit (ctx, axl_true, axl_true);
		
	return axl_true;
}

axl_bool common_connect_send_and_check (MyQttCtx   * myqtt_ctx, 
					const char * client_id, const char * user, const char * password,
					const char * topic,     const char * message, 
					const char * check_reply,
					MyQttQos qos, 
					axl_bool skip_error_reporting)
{

	MyQttAsyncQueue * queue;
	MyQttConn       * conn;
	MyQttMsg        * msg;
	int               sub_result;
	MyQttConnOpts   * opts = NULL;

	/* create connection to local server and test domain support */
	if (myqtt_ctx == NULL) {
		myqtt_ctx = common_init_ctx ();
		if (! myqtt_init_ctx (myqtt_ctx)) {
			if (! skip_error_reporting)
				printf ("Error: unable to initialize MyQtt library..\n");
			return axl_false;
		} /* end if */
	} /* end if */

	printf ("Test --: connecting to myqtt server (client ctx = %p)..\n", myqtt_ctx);

	/* configure user and password */
	if (user && password) {
		opts = myqtt_conn_opts_new ();
		myqtt_conn_opts_set_auth (opts, user, password);
	} /* end if */
	
	conn = myqtt_conn_new (myqtt_ctx, client_id, axl_true, 30, listener_host, listener_port, opts, NULL, NULL);
	if (! myqtt_conn_is_ok (conn, axl_false)) {
		if (! skip_error_reporting)
			printf ("ERROR: unable to connect to %s:%s..\n", listener_host, listener_port);

		myqtt_conn_close (conn);
		myqtt_exit_ctx (myqtt_ctx, axl_true);

		return axl_false;
	} /* end if */

	/* subscribe */
	printf ("Test --: subscribe to the topic %s..\n", topic);
	if (! myqtt_conn_sub (conn, 10, topic, 0, &sub_result)) {
		if (! skip_error_reporting)
			printf ("ERROR: unable to subscribe, myqtt_conn_sub () failed, sub_result=%d", sub_result);
		/* close connection */
		myqtt_conn_close (conn);
		myqtt_exit_ctx (myqtt_ctx, axl_true);

		return axl_false;
	} /* end if */	

	printf ("Test --: subscription completed with qos=%d (requested=%d)\n", sub_result, qos);
	
	/* register on message handler */
	queue = myqtt_async_queue_new ();
	myqtt_conn_set_on_msg (conn, common_queue_message_received, queue);

	/* push a message */
	printf ("Test --: publishing to the topic %s..\n", topic);
	if (! myqtt_conn_pub (conn, topic, (const axlPointer) message, strlen (message), qos, axl_false, 0)) {
		if (! skip_error_reporting)
			printf ("ERROR: unable to publish message, myqtt_conn_pub() failed\n");

		/* close connection */
		myqtt_conn_close (conn);
		myqtt_exit_ctx (myqtt_ctx, axl_true);

		return axl_false;
	} /* end if */

	/* receive it */
	printf ("Test --: waiting for message %s (5 seconds)..\n", topic);
	msg   = myqtt_async_queue_timedpop (queue, 5000000);
	myqtt_async_queue_unref (queue);
	if (msg == NULL) {
		if (! skip_error_reporting)
			printf ("ERROR: expected to find message from queue, but NULL was found..\n");

		/* close connection */
		myqtt_conn_close (conn);
		myqtt_exit_ctx (myqtt_ctx, axl_true);

		return axl_false;
	} /* end if */

	/* check content */
	printf ("Test --: received reply...checking data..\n");
	if (myqtt_msg_get_app_msg_size (msg) != strlen (check_reply ? check_reply : message)) {
		if (! skip_error_reporting)
			printf ("ERROR: expected payload size of %d but found %d\nContent found: %s\n", 
				(int) strlen (check_reply ? check_reply : message),
				myqtt_msg_get_app_msg_size (msg),
				(const char *) myqtt_msg_get_app_msg (msg));

		/* close connection */
		myqtt_conn_close (conn);
		myqtt_exit_ctx (myqtt_ctx, axl_true);

		return axl_false;
	} /* end if */

	if (myqtt_msg_get_type (msg) != MYQTT_PUBLISH) {
		if (! skip_error_reporting)
			printf ("ERROR: expected to receive PUBLISH message but found: %s\n", myqtt_msg_get_type_str (msg));

		/* close connection */
		myqtt_conn_close (conn);
		myqtt_exit_ctx (myqtt_ctx, axl_true);

		return axl_false;
	} /* end if */

	/* check content */
	if (! axl_cmp ((const char *) myqtt_msg_get_app_msg (msg), check_reply ? check_reply : message)) {
		if (! skip_error_reporting)
			printf ("ERROR: expected to find different content..\n");

		/* close connection */
		myqtt_conn_close (conn);
		myqtt_exit_ctx (myqtt_ctx, axl_true);

		return axl_false;
	} /* end if */

	printf ("Test --: everything is ok, finishing context and connection..\n");
	myqtt_msg_unref (msg);

	/* close connection */
	myqtt_conn_close (conn);
	myqtt_exit_ctx (myqtt_ctx, axl_true);

	return axl_true;
	
}

MyQttConn * common_connect_and_subscribe (MyQttCtx * myqtt_ctx, const char * client_id, 
					  const char * topic, 
					  MyQttQos qos, axl_bool skip_error_reporting)
{
	MyQttConn       * conn;
	int               sub_result;

	/* create connection to local server and test domain support */
	if (myqtt_ctx == NULL) {
		myqtt_ctx = common_init_ctx ();
		if (! myqtt_init_ctx (myqtt_ctx)) {
			if (! skip_error_reporting)
				printf ("Error: unable to initialize MyQtt library..\n");
			return axl_false;
		} /* end if */
	} /* end if */

	printf ("Test --: connecting to myqtt server (client ctx = %p, client_id = '%s', %s:%s)..\n", 
		myqtt_ctx, client_id, listener_host, listener_port);
	
	conn = myqtt_conn_new (myqtt_ctx, client_id, axl_true, 30, listener_host, listener_port, NULL, NULL, NULL);
	if (! myqtt_conn_is_ok (conn, axl_false)) {
		if (! skip_error_reporting)
			printf ("ERROR: unable to connect to %s:%s..\n", listener_host, listener_port);
		return NULL;
	} /* end if */

	printf ("Test --: connection OK to %s:%s (client ctx = %p)..\n", listener_host, listener_port, myqtt_ctx);

	/* subscribe */
	printf ("Test --: subscribe to the topic %s..\n", topic);
	if (! myqtt_conn_sub (conn, 10, topic, 0, &sub_result)) {
		if (! skip_error_reporting)
			printf ("ERROR: unable to subscribe, myqtt_conn_sub () failed, sub_result=%d", sub_result);
		return NULL;
	} /* end if */	

	printf ("Test --: subscription completed with qos=%d (requested=%d)\n", sub_result, qos);
	return conn;
}

void common_close_conn_and_ctx (MyQttConn * conn)
{
	MyQttCtx * ctx = myqtt_conn_get_ctx (conn);
	
	printf ("Test --: releasing connection (%p)\n", conn);
	myqtt_conn_close (conn);

	/* now close the library */
	printf ("Test --: releasing context (%p)\n", ctx);
	myqtt_exit_ctx (ctx, axl_true);

	return;
}

axl_bool common_send_msg (MyQttConn * conn, const char * topic, const char * message, MyQttQos qos) {
	/* publish message */
	return myqtt_conn_pub (conn, topic, (const axlPointer) message, strlen (message), qos, axl_false, 10);
}

void common_configure_reception_queue_received (MyQttCtx * ctx, MyQttConn * conn, MyQttMsg * msg, axlPointer user_data)
{
	MyQttAsyncQueue * queue = user_data;

	/* push message received */
	myqtt_msg_ref (msg);
	myqtt_async_queue_push (queue, msg);
	return;
}

MyQttAsyncQueue * common_configure_reception (MyQttConn * conn) {
	MyQttAsyncQueue * queue = myqtt_async_queue_new ();

	/* configure reception on queue  */
	myqtt_conn_set_on_msg (conn, common_configure_reception_queue_received, queue);

	return queue;
}

axl_bool common_receive_and_check (MyQttAsyncQueue * queue, const char * topic, const char * message, MyQttQos qos, axl_bool skip_fail_if_null)
{
	MyQttMsg * msg;

	/* get message */
	msg   = myqtt_async_queue_timedpop (queue, 3000000);
	if (msg == NULL) {
		if (skip_fail_if_null)
			return axl_true;
		printf ("ERROR: NULL message received...when expected to receive something..\n");
		return axl_false;
	} /* end if */

	if (! axl_cmp (message, (const char *) myqtt_msg_get_app_msg (msg))) {
		printf ("Test --: message content mismatch received (expected '%s' but received '%s'\n",
			message, (const char *) myqtt_msg_get_app_msg (msg));
		myqtt_msg_unref (msg);
		return axl_false;
	}

	if (! axl_cmp (topic, (const char *) myqtt_msg_get_topic (msg))) {
		printf ("Test --: message topic mismatch received..\n");
		myqtt_msg_unref (msg);
		return axl_false;
	}

	if (qos != myqtt_msg_get_qos (msg)) {
		printf ("Test --: message qos mismatch received..\n");
		myqtt_msg_unref (msg);
		return axl_false;
	}

	
	myqtt_msg_unref (msg);
	return axl_true;
	
}

axl_bool  test_03 (void) {
	MyQttdCtx       * ctx;

	MyQttConn       * conn;
	MyQttConn       * conn2;

	MyQttAsyncQueue * queue;
	MyQttAsyncQueue * queue2;
	
	/* call to init the base library and close it */
	printf ("Test 03: init library and server engine (using test_02.conf)..\n");
	ctx       = common_init_ctxd (NULL, "test_02.conf");
	if (ctx == NULL) {
		printf ("Test 00: failed to start library and server engine..\n");
		return axl_false;
	} /* end if */

	printf ("Test 03: library and server engine started.. ok (ctxd = %p, ctx = %p\n", ctx, MYQTTD_MYQTT_CTX (ctx));

	if (myqttd_domain_count_enabled (ctx) != 0) {
		printf ("Test 03: (1) expected to find 0 domains enabled but found %d\n", myqttd_domain_count_enabled (ctx));
		return axl_false;
	} /* end if */

	/* connect and send message */
	if (! common_connect_send_and_check (NULL, "test_02", NULL, NULL, "myqtt/test", "This is test message (test-03)....", NULL, MYQTT_QOS_0, axl_false)) {
		printf ("Test 03: unable to connect and send message...\n");
		return axl_false;
	} /* end if */

	if (myqttd_domain_count_enabled (ctx) != 1) {
		printf ("Test 03: (2) expected to find 1 domains enabled but found %d\n", myqttd_domain_count_enabled (ctx));
		return axl_false;
	} /* end if */

	/* connect and send message */
	if (! common_connect_send_and_check (NULL, "test_04", NULL, NULL, "myqtt/test", "This is test message (test-03)....", NULL, MYQTT_QOS_0, axl_false)) {
		printf ("Test 03: unable to connect and send message...\n");
		return axl_false;
	} /* end if */

	if (myqttd_domain_count_enabled (ctx) != 2) {
		printf ("Test 03: (3) expected to find 2 domains enabled but found %d\n", myqttd_domain_count_enabled (ctx));
		return axl_false;
	} /* end if */


	printf ("Test 03: checked domain activation for both services...Ok\n");

	/* connect and send message */
	if (common_connect_send_and_check (NULL, "test_05_wrong", NULL, NULL, "myqtt/test", "This is test message (test-03)....", NULL, MYQTT_QOS_0, axl_true)) {
		printf ("Test 03: it should fail to connect but it connected (using test_05_wrong)..\n");
		return axl_false;
	} /* end if */

	/* ensure connections in domain */
	if (myqttd_domain_conn_count (myqttd_domain_find_by_name (ctx, "test_01.context")) != 0) {
		printf ("Test 03: (4) expected to find 0 connection but found something different..\n");
		return axl_false;
	}

	/* connect and report connection */
	printf ("Test 03: ------------------\n");
	printf ("Test 03: connecting with client_identifier='test_02', and sub='myqtt/test'\n");
	conn = common_connect_and_subscribe (NULL, "test_02", "myqtt/test", MYQTT_QOS_0, axl_false);
	if (conn == NULL) {
		printf ("Test 03: unable to connect to the domain..\n");
		return axl_false;
	}

	/* ensure connections in domain */
	if (myqttd_domain_conn_count (myqttd_domain_find_by_name (ctx, "test_01.context")) != 1) {
		printf ("Test 03: (5) expected to find 1 connection but found something different: %d..\n", 
			myqttd_domain_conn_count (myqttd_domain_find_by_name (ctx, "test_01.context")));
		return axl_false;
	}

	/* ensure connections in domain */
	if (myqttd_domain_conn_count (myqttd_domain_find_by_name (ctx, "test_02.context")) != 0) {
		printf ("Test 03: (6) expected to find 0 connection but found something different: %d..\n",
			myqttd_domain_conn_count (myqttd_domain_find_by_name (ctx, "test_02.context")));
		return axl_false;
	}

	/* connect and report connection */
	printf ("Test 03: ------------------\n");
	printf ("Test 03: connecting with client_identifier='test_04', and sub='myqtt/test'\n");
	conn2 = common_connect_and_subscribe (NULL, "test_04", "myqtt/test", MYQTT_QOS_0, axl_false);
	if (conn2 == NULL) {
		printf ("Test 03: unable to connect to the domain..\n");
		return axl_false;
	}

	/* ensure connections in domain */
	printf ("Test 03: checking client_identifier='test_04' is connected to context='test_02.context'\n");
	if (myqttd_domain_conn_count (myqttd_domain_find_by_name (ctx, "test_02.context")) != 1) {
		printf ("Test 03: (7) expected to find 1 connection but found something different: %d..\n",
			myqttd_domain_conn_count (myqttd_domain_find_by_name (ctx, "test_02.context")));
		return axl_false;
	}
	
	/* configure message reception */
	queue  = common_configure_reception (conn);
	queue2 = common_configure_reception (conn2);

	/* send message for conn and check for reply */
	if (! common_send_msg (conn, "myqtt/test", "This is an application message", MYQTT_QOS_0)) {
		printf ("Test 03: unable to send message\n");
		return axl_false;
	}
	if (! common_send_msg (conn2, "myqtt/test", "This is an application message (2)", MYQTT_QOS_0)) {
		printf ("Test 03: unable to send message\n");
		return axl_false;
	}

	/* now receive message */
	printf ("Test 03: checking for messages received..\n");
	if (! common_receive_and_check (queue, "myqtt/test", "This is an application message", MYQTT_QOS_0, axl_false)) {
		printf ("Test 03: expected to receive different message..\n");
		return axl_false;
	}

	if (! common_receive_and_check (queue2, "myqtt/test", "This is an application message (2)", MYQTT_QOS_0, axl_false)) {
		printf ("Test 03: expected to receive different message..\n");
		return axl_false;
	}

	printf ("Test 03: now check if we don't receive messages from different domains (4 seconds waiting)..\n");
	if (myqtt_async_queue_timedpop (queue, 2000000)) {
		printf ("Test 03: expected to not receive any message over this connection..\n");
		return axl_false;
	}
	if (myqtt_async_queue_timedpop (queue2, 2000000)) {
		printf ("Test 03: expected to not receive any message over this connection..\n");
		return axl_false;
	}

	/* release queue */
	myqtt_async_queue_unref (queue);
	myqtt_async_queue_unref (queue2);


	/* close connection and context */
	common_close_conn_and_ctx (conn);
	common_close_conn_and_ctx (conn2);


	printf ("Test --: finishing MyQttdCtx..\n");
	

	/* finish server */
	myqttd_exit (ctx, axl_true, axl_true);
		
	return axl_true;
}

MyQttPublishCodes test_04_handle_publish (MyQttdCtx * ctx,       MyQttdDomain * domain,  
					  MyQttCtx  * myqtt_ctx, MyQttConn    * conn, 
					  MyQttMsg  * msg,       axlPointer user_data)
{
	printf ("Test --: received message on server (topic: %s)\n", myqtt_msg_get_topic (msg));
	if (axl_cmp ("get-context", myqtt_msg_get_topic (msg))) {
		printf ("Test --: publish received on domain: %s\n", domain->name);

		if (! myqtt_conn_pub (conn, "get-context", domain->name, (int) strlen (domain->name), MYQTT_QOS_0, axl_false, 0))
			printf ("ERROR: failed to publish message in reply..\n");
		return MYQTT_PUBLISH_DISCARD;
	}

	return MYQTT_PUBLISH_OK;
}

axl_bool  test_04 (void) {

	MyQttdCtx       * ctx;

	/* call to init the base library and close it */
	printf ("Test 04: init library and server engine (using test_02.conf)..\n");
	ctx       = common_init_ctxd (NULL, "test_02.conf");
	if (ctx == NULL) {
		printf ("Test 00: failed to start library and server engine..\n");
		return axl_false;
	} /* end if */

	printf ("Test 04: library and server engine started.. ok (ctxd = %p, ctx = %p\n", ctx, MYQTTD_MYQTT_CTX (ctx));

	/* configure the on publish */
	myqttd_ctx_add_on_publish (ctx, test_04_handle_publish, NULL);

	/* connect and send message */
	printf ("Test 04: checking context activation based or user/password/clientid (test_01.context)\n");
	if (! common_connect_send_and_check (NULL, 
				      /* client id, user and password */
				      "test_02", "user-test-02", "test1234", 
				      "get-context", "get-context", "test_01.context", MYQTT_QOS_0, axl_false)) {
		printf ("Test --: unable to connect and send message...\n");
		return axl_false;
	} /* end if */

	/* connect and send message */
	printf ("Test 04: checking context activation based or user/password/clientid (test_02.context)\n");
	if (! common_connect_send_and_check (NULL, 
				      /* client id, user and password */
				      "test_02", "user-test-02", "differentpass", 
				      "get-context", "get-context", "test_02.context", MYQTT_QOS_0, axl_false)) {
		printf ("Test --: unable to connect and send message...\n");
		return axl_false;
	} /* end if */
	
	printf ("Test --: finishing MyQttdCtx..\n");
	

	/* finish server */
	myqttd_exit (ctx, axl_true, axl_true);
		
	return axl_true;
}


axl_bool  test_05 (void) {

	MyQttdCtx       * ctx;

	/* call to init the base library and close it */
	printf ("Test 05: init library and server engine (using test_02.conf)..\n");
	ctx       = common_init_ctxd (NULL, "test_02.conf");
	if (ctx == NULL) {
		printf ("Test 00: failed to start library and server engine..\n");
		return axl_false;
	} /* end if */

	printf ("Test 05: library and server engine started.. ok (ctxd = %p, ctx = %p\n", ctx, MYQTTD_MYQTT_CTX (ctx));

	/* connect and send message */
	printf ("Test 05: checking context is not activated based or user/password/clientid (test_01.context)\n");
	if (common_connect_send_and_check (NULL, 
				      /* client id, user and password */
				      "test_02", "user-test-02", "test1234-5", 
				      "get-context", "get-context", "test_01.context", MYQTT_QOS_0, axl_true)) {
		printf ("ERROR: it shouldn't work it does...\n");
		return axl_false;
	} /* end if */

	/* connect and send message */
	printf ("Test 05: checking context is not activated based or user/password/clientid (test_02.context)\n");
	if (common_connect_send_and_check (NULL, 
				      /* client id, user and password */
				      "test_02", "user-test-02", "differentpass-5", 
				      "get-context", "get-context", "test_02.context", MYQTT_QOS_0, axl_true)) {
		printf ("ERROR: it shouldn't work it does...\n");
		return axl_false;
	} /* end if */
	
	printf ("Test --: finishing MyQttdCtx..\n");
	
	if (myqttd_domain_count_enabled (ctx) != 0) {
		printf ("Test --: expected to find 0 domains enabled but found %d\n", myqttd_domain_count_enabled (ctx));
		return axl_false;
	} /* end if */

	/* finish server */
	myqttd_exit (ctx, axl_true, axl_true);
		
	return axl_true;
}

axl_bool  test_06 (void) {

	MyQttdCtx           * ctx;
	MyQttdDomainSetting * setting;

	/* call to init the base library and close it */
	printf ("Test 06: init library and server engine (using test_02.conf)..\n");
	ctx       = common_init_ctxd (NULL, "test_02.conf");
	if (ctx == NULL) {
		printf ("Test --: failed to start library and server engine..\n");
		return axl_false;
	} /* end if */

	printf ("Test 06: checking loaded settings..\n");
	if (ctx->default_setting == NULL) {
		printf ("Test --: default settings wheren't loaded..\n");
		return axl_false;
	} /* end if */

	if (! ctx->default_setting->require_auth) {
		printf ("Test --: default settings wheren't loaded for require auth..\n");
		return axl_false;
	} /* end if */

	if (! ctx->default_setting->restrict_ids) {
		printf ("Test --: default settings wheren't loaded for restrict ids..\n");
		return axl_false;
	} /* end if */

	/* now get settings by name */
	setting = myqtt_hash_lookup (ctx->domain_settings, "basic");
	if (setting == NULL) {
		printf ("ERROR: we should've found setting but NULL was found..\n"); 
		return axl_false;
	}

	printf ("Test --: check basic settings\n");
	if (setting->conn_limit != 50 ||
	    setting->message_size_limit != 256 ||
	    setting->storage_messages_limit != 10000 ||
	    setting->storage_quota_limit != 102400) {
		printf ("ERROR: expected different values (basic)...\n");
		return axl_false;
	}

	/* now get settings by name */
	setting = myqtt_hash_lookup (ctx->domain_settings, "standard");
	if (setting == NULL) {
		printf ("ERROR: we should've found setting but NULL was found..\n"); 
		return axl_false;
	}

	printf ("Test --: check standard settings\n");
	if (setting->conn_limit != 10 ||
	    setting->message_size_limit != 65536 ||
	    setting->storage_messages_limit != 20000 ||
	    setting->storage_quota_limit != 204800) {
		printf ("ERROR: expected different values (standard)...\n");
		return axl_false;
	} /* end if */

	printf ("Test --: settings ok\n");

	/* finish server */
	myqttd_exit (ctx, axl_true, axl_true);

	return axl_true;
}

MyQttPublishCodes test_07_handle_publish (MyQttdCtx * ctx,       MyQttdDomain * domain,  
					  MyQttCtx  * myqtt_ctx, MyQttConn    * conn, 
					  MyQttMsg  * msg,       axlPointer user_data)
{
	char * result;

	printf ("Test --: received message on server (topic: %s)\n", myqtt_msg_get_topic (msg));
	if (axl_cmp ("get-connections", myqtt_msg_get_topic (msg))) {
		printf ("Test --: publish received on domain: %s\n", domain->name);

		/* get current connections */
		result = axl_strdup_printf ("%d", axl_list_length (myqtt_ctx->conn_list) + axl_list_length (myqtt_ctx->srv_list));

		printf ("Test --: current connections are: %s\n", result);

		if (! myqtt_conn_pub (conn, "get-connections", result, (int) strlen (result), MYQTT_QOS_0, axl_false, 0))
			printf ("ERROR: failed to publish message in reply..\n");
		axl_free (result);
		return MYQTT_PUBLISH_DISCARD;
	}

	return MYQTT_PUBLISH_OK;
}


axl_bool  test_07 (void) {
	
	MyQttdCtx       * ctx;
	MyQttCtx        * myqtt_ctx;
	MyQttConn       * conns[60];
	int               iterator;
	char            * client_id;

	/* call to init the base library and close it */
	printf ("Test 07: init library and server engine (using test_02.conf)..\n");
	ctx       = common_init_ctxd (NULL, "test_02.conf");
	if (ctx == NULL) {
		printf ("Test 00: failed to start library and server engine..\n");
		return axl_false;
	} /* end if */

	myqtt_ctx = common_init_ctx ();
	if (! myqtt_init_ctx (myqtt_ctx)) {
		printf ("Error: unable to initialize MyQtt library..\n");
		return axl_false;
	} /* end if */
	
	/* connect to test_01.context and create more than 5 connections */
	printf ("Test 07: creating 50 connections..\n");
	iterator = 0;
	while (iterator < 50) {
		/* call to get client id */
		client_id       = axl_strdup_printf ("test_02_%d", iterator);
		conns[iterator] = myqtt_conn_new (myqtt_ctx, client_id, axl_true, 30, listener_host, listener_port, NULL, NULL, NULL);
		axl_free (client_id);

		if (! myqtt_conn_is_ok (conns[iterator], axl_false)) {
			printf ("ERROR: unable to connect to %s:%s..\n", listener_host, listener_port);
			return axl_false;
		} /* end if */

		/* next iterator */
		iterator++;
	} /* end while */

	/* check connections */
	myqttd_ctx_add_on_publish (ctx, test_07_handle_publish, NULL);

	/* connect and send message */
	printf ("Test 07: requesting number of connections remotely..\n");
	if (! common_connect_send_and_check (NULL, 
				      /* client id, user and password */
				      "test_02", "user-test-02", "test1234", 
				      /* we've got 6 connections because we have 50 plus the
					 connection that is being requesting the get-connections */
				      "get-connections", "", "51", MYQTT_QOS_0, axl_false)) {
		printf ("Test --: unable to connect and send message...\n");
		return axl_false;
	} /* end if */
	
	/* check limits */
	conns[iterator] = myqtt_conn_new (myqtt_ctx, "test_02", axl_true, 30, listener_host, listener_port, NULL, NULL, NULL);
	if (myqtt_conn_is_ok (conns[iterator], axl_false)) {
		printf ("ERROR: it worked and it shouldn't...connect to %s:%s..\n", listener_host, listener_port);
		return axl_false;
	} /* end if */
	myqtt_conn_close (conns[iterator]);

	iterator = 0;
	while (iterator < 50) {
		/* close connection */
		myqtt_conn_close (conns[iterator]);
		/* next iterator */
		iterator++;
	} /* end while */

	/* connect to test_01.context and create more than 5 connections */
	printf ("Test 07: creating 10 connections (standard sensting)..\n");
	iterator = 0;
	while (iterator < 10) {
		/* create connection */
		client_id = axl_strdup_printf ("test_04_%d", iterator);
		conns[iterator] = myqtt_conn_new (myqtt_ctx, client_id, axl_true, 30, listener_host, listener_port, NULL, NULL, NULL);
		axl_free (client_id);

		if (! myqtt_conn_is_ok (conns[iterator], axl_false)) {
			printf ("ERROR: unable to connect to %s:%s..\n", listener_host, listener_port);
			return axl_false;
		} /* end if */

		/* next iterator */
		iterator++;
	} /* end while */

	/* connect and send message */
	printf ("Test 07: requesting number of connections remotely..\n");
	if (! common_connect_send_and_check (NULL, 
				      /* client id, user and password */
				      "test_04", NULL, NULL,
				      /* we've got 6 connections because we have 5 plus the
					 connection that is being requesting the get-connections */
				      "get-connections", "", "11", MYQTT_QOS_0, axl_false)) {
		printf ("Test --: unable to connect and send message...\n");
		return axl_false;
	} /* end if */

	/* check limits */
	conns[iterator] = myqtt_conn_new (myqtt_ctx, "test_04", axl_true, 30, listener_host, listener_port, NULL, NULL, NULL);
	if (myqtt_conn_is_ok (conns[iterator], axl_false)) {
		printf ("ERROR: it worked and it shouldn't...connect to %s:%s..\n", listener_host, listener_port);
		return axl_false;
	} /* end if */
	myqtt_conn_close (conns[iterator]);

	iterator = 0;
	while (iterator < 10) {
		/* close connection */
		myqtt_conn_close (conns[iterator]);
		/* next iterator */
		iterator++;
	} /* end while */

	myqtt_exit_ctx (myqtt_ctx, axl_true);
	printf ("Test 07: finishing MyQttdCtx..\n");
	/* finish server */
	myqttd_exit (ctx, axl_true, axl_true);
	
	
	return axl_true;
}

axl_bool  test_08 (void) {
	
	MyQttdCtx       * ctx;
	MyQttCtx        * myqtt_ctx;
	MyQttConn       * conn;
	MyQttAsyncQueue * queue;
	const char      * message;
	int               sub_result;

	/* call to init the base library and close it */
	printf ("Test 08: init library and server engine (using test_02.conf)..\n");
	ctx       = common_init_ctxd (NULL, "test_02.conf");
	if (ctx == NULL) {
		printf ("Test 00: failed to start library and server engine..\n");
		return axl_false;
	} /* end if */

	myqtt_ctx = common_init_ctx ();
	if (! myqtt_init_ctx (myqtt_ctx)) {
		printf ("Error: unable to initialize MyQtt library..\n");
		return axl_false;
	} /* end if */
	
	/* connect to test_01.context and create more than 5 connections */
	printf ("Test 08: attempting to send a message bigger than configured allowed sizes\n");
	conn = myqtt_conn_new (myqtt_ctx, "test_02", axl_true, 30, listener_host, listener_port, NULL, NULL, NULL);
	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: unable to connect to %s:%s..\n", listener_host, listener_port);
		return axl_false;
	} /* end if */

	/* subscribe to the topic to get notifications */
	if (! myqtt_conn_sub (conn, 10, "myqtt/test", 0, &sub_result)) {
		printf ("ERROR: unable to subscribe, myqtt_conn_sub () failed, sub_result=%d", sub_result);
		return axl_false;
	} /* end if */	

	/* configure asyncqueue reception */
	queue  = common_configure_reception (conn);

	message = "ksljg0823io2j135lknmwegoij2346oi24jtoi4mg4ylk34jyksljg0823io2j135lknmwegoij2346oi24jtoi4mg4ylk34jyksljg0823io2j135lknmwegoij2346oi24jtoi4mg4ylk34jyksljg0823io2j135lknmwegoij2346oi24jtoi4mg4ylk34jyksljg0823io2j135lknmwegoij2346oi24jtoi4mg4ylk34jyksljg0823io2j135lknmwegoij2346oi24jtoi4mg4ylk34jyksljg0823io2j135lknmwegoij2346oi24jtoi4mg4ylk34jyksljg0823io2j135lknmwegoij2346oi24jtoi4mg4ylk34jyksljg0823io2j135lknmwegoij2346oi24jtoi4mg4ylk34jyksljg0823io2j135lknmwegoij2346oi24jtoi4mg4ylk34jyksljg0823io2j135lknmwegoij2346oi24jtoi4mg4ylk34jyksljg0823io2j135lknmwegoij2346oi24jtoi4mg4ylk34jyksljg0823io2j135lknmwegoij2346oi24jtoi4mg4ylk34jyksljg0823io2j135lknmwegoij2346oi24jtoi4mg4ylk34jyksljg0823io2j135lknmwegoij2346oi24jtoi4mg4ylk34jy";

	if (! myqtt_conn_pub (conn, "myqtt/test", (axlPointer) message, (int) strlen (message), MYQTT_QOS_0, axl_false, 0))
		printf ("ERROR: failed to publish message in reply..\n");

	printf ("Test --: we shouldn't receive a totification for this message (waiting 3 seconds)\n");
	if (myqtt_async_queue_timedpop (queue, 3000000)) {
		printf ("Test --: expected to not receive any message over this connection..\n");
		return axl_false;
	}

	/* close connection */
	myqtt_conn_close (conn);

	myqtt_async_queue_unref (queue);

	myqtt_exit_ctx (myqtt_ctx, axl_true);
	printf ("Test 08: finishing MyQttdCtx..\n");
	/* finish server */
	myqttd_exit (ctx, axl_true, axl_true);
	
	return axl_true;
}

MyQttPublishCodes test_09_handle_publish (MyQttdCtx * ctx,       MyQttdDomain * domain,  
					  MyQttCtx  * myqtt_ctx, MyQttConn    * conn, 
					  MyQttMsg  * msg,       axlPointer user_data)
{
	axlHashCursor    * cursor;
	const char       * topic;
	char             * result = NULL;
	char             * aux;
	const char       * label;

	printf ("Test --: received message on server (topic: %s)\n", myqtt_msg_get_topic (msg));
	if (axl_cmp ("get-subscriptions", myqtt_msg_get_topic (msg)) || axl_cmp ("get-offline-subscriptions", myqtt_msg_get_topic (msg))) {
 		printf ("Test --: reported subscriptions in: %s\n", domain->name);
		
		if (axl_cmp ("get-subscriptions", myqtt_msg_get_topic (msg))) {
			cursor = axl_hash_cursor_new (myqtt_ctx->subs);
			label  = "ON-LINE";
		} else if (axl_cmp ("get-offline-subscriptions", myqtt_msg_get_topic (msg))) {
			cursor = axl_hash_cursor_new (myqtt_ctx->offline_subs);
			label  = "OFF-LINE";
		} else {
			printf ("Test --: ERROR: received unsupported request..\n");
			return MYQTT_PUBLISH_OK;
		}
		axl_hash_cursor_first (cursor);
		
		while (axl_hash_cursor_has_item (cursor)) {

			/* get topic */
			topic = axl_hash_cursor_get_key (cursor);
			if (result) {
				aux = result;
				result = axl_strdup_printf ("%s\n%s", result, topic);
				axl_free (aux);
			} else {
				result = axl_strdup (topic);
			} /* end if */

			/* next position */
			axl_hash_cursor_next (cursor);
		} /* end if */

		axl_hash_cursor_free (cursor);
		
		printf ("Test --: sending current subscriptions (%s): %s\n", label, result);

		if (! myqtt_conn_pub (conn, myqtt_msg_get_topic (msg), result, (int) strlen (result), MYQTT_QOS_0, axl_false, 0))
			printf ("ERROR: failed to publish message in reply..\n");

		/* free result */
		axl_free (result);
		return MYQTT_PUBLISH_DISCARD;
	}

	return MYQTT_PUBLISH_OK;
}


axl_bool  test_09 (void) {
	
	MyQttdCtx       * ctx;
	MyQttCtx        * myqtt_ctx;
	MyQttConn       * conn;
	const char      * message;
	int               sub_result;
	int               iterator;
	MyQttAsyncQueue * queue;
	MyQttdDomain    * domain;
	int               quota;
	int               value;

	/* do some cleanup */
	if (system ("find reg-test-03/storage/test_05 -type f -exec rm {} \\;") != 0) {
		printf ("ERROR: failed to initialize test..\n");
		return axl_false;
	}

	/* call to init the base library and close it */
	printf ("Test 09: init library and server engine (using test_02.conf)..\n");
	ctx       = common_init_ctxd (NULL, "test_02.conf");
	if (ctx == NULL) {
		printf ("Test 00: failed to start library and server engine..\n");
		return axl_false;
	} /* end if */

	myqtt_ctx = common_init_ctx ();
	if (! myqtt_init_ctx (myqtt_ctx)) {
		printf ("Error: unable to initialize MyQtt library..\n");
		return axl_false;
	} /* end if */
	
	/* connect and subscribe */
	conn = myqtt_conn_new (myqtt_ctx, "test_05", axl_false, 30, listener_host, listener_port, NULL, NULL, NULL);
	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: unable to connect to %s:%s..\n", listener_host, listener_port);
		return axl_false;
	} /* end if */

	/* subscribe to the topic to get notifications */
	if (! myqtt_conn_sub (conn, 10, "myqtt/test", 0, &sub_result)) {
		printf ("ERROR: unable to subscribe, myqtt_conn_sub () failed, sub_result=%d", sub_result);
		return axl_false;
	} /* end if */	

	/* checking that the subscription is recorded at the remote server */
	printf ("Test 09: checking subscription in domain selected..\n");
	myqttd_ctx_add_on_publish (ctx, test_09_handle_publish, NULL);

	/* configure reception */
	queue = common_configure_reception (conn);
	if (! common_send_msg (conn, "get-subscriptions", "---", MYQTT_QOS_0)) {
		printf ("ERROR: unable to send get-subscriptions message..\n");
		return axl_false;
	} /* end if */

	/* now get result */
	if (! common_receive_and_check (queue, "get-subscriptions", "myqtt/test", MYQTT_QOS_0, axl_false)) {
		printf ("ERROR: expected different content..\n");
		return axl_false;
	} /* end if */

	/* now close connection to overload quota */
	myqtt_async_queue_unref (queue);
	myqtt_conn_close (conn);

	/* checking that the subscription is recorded at the remote server at the offline storage */
	printf ("Test 09: checking subscription in domain selected (offline storage)..\n");
	if (! common_connect_send_and_check (NULL, "test_05", NULL, NULL, "get-offline-subscriptions", "--", "myqtt/test", MYQTT_QOS_0, axl_false)) {
		printf ("ERROR: expected to find different offline subscriptions..\n");
		return axl_false;
	} /* end if */

	/* now connect again to publish messages */
	conn = myqtt_conn_new (myqtt_ctx, "test_06", axl_false, 30, listener_host, listener_port, NULL, NULL, NULL);
	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: unable to connect to %s:%s..\n", listener_host, listener_port);
		return axl_false;
	} /* end if */

	domain = myqttd_domain_find_by_name (ctx, "test_03.context");
	if (domain == NULL || myqtt_storage_queued_messages_quota_offline (domain->myqtt_ctx, "test_05")) {
		printf ("ERROR: expected to find domain (test_03.context) or quota used by test_05 to be 0 but some of them have different values..\n");
		return axl_false;
	} /* end if */

	/* a 1K message (1024 bytes) */
	message = "ksljg0823io2j135lknmwegoij2346oi24jtoi4mg4ylk34jyksljg0823io2j135lknmwegoij2346oi24jtoi4mg4ylk34jyksljg0823io2j135lknmwegoij2346oi24jtoi4mg4ylk34jyksljg0823io2j135lknmwegoij2346oi24jtoi4mg4ylk34jyksljg0823io2j135lknmwegoij2346oi24jtoi4mg4ylk34jyksljg0823io2j135lknmwegoij2346oi24jtoi4mg4ylk34jyksljg0823io2j135lknmwegoij2346oi24jtoi4mg4ylk34jyksljg0823io2j135lknmwegoij2346oi24jtoi4mg4ylk34jyksljg0823io2j135lknmwegoij2346oi24jtoi4mg4ylk34jyksljg0823io2j135lknmwegoij2346oi24jtoi4mg4ylk34jyksljg0823io2j135lknmwegoij2346oi24jtoi4mg4ylk34jyksljg0823io2j135lknmwegoij2346oi24jtoi4mg4ylk34jyksljg0823io2j135lknmwegoij2346oi24jtoi4mg4ylk34jyksljg0823io2j135lknmwegoij2346oi24jtoi4mg4ylk34jyksljg0823io2j135lknmwegoij2346oi24jtoi4mg4ylk34jyj2346oi24jtoi4mg4ylk34jyksljg0823io2j135lknmwegoij2346oi24jtoi4mg4ylk34jyksljg0823io2j135lknmwegoij2346oi24jtoi4mg4ylk34jyk34jyksljg0823io2j135lknmwegoij2346oi24jtoi4mg4ylk34jyj2346oi24jtoi4mg4ylk34jyksljg0823io2j135lknmwegoij2346oi24jtoi4mg4ylk34jyksljg0823io2j135lknmwegoij2346oi24jtoi4m";
	iterator = 0;
	while (iterator < 10) {
		printf ("Test 09: sending message...(iterator=%d, message=%d)\n", iterator, (int) strlen (message));
		if (! myqtt_conn_pub (conn, "myqtt/test", (axlPointer) message, (int) strlen (message), MYQTT_QOS_2, axl_false, 10))
			printf ("ERROR: failed to publish message in reply..\n");

		/* next iterator */
		iterator++;
	} /* end while */

	/* check quotas here */
	quota = myqtt_storage_queued_messages_quota_offline (domain->myqtt_ctx, "test_05");
	printf ("Test 09: used quota so fat..by test_05 (%d)\n", quota);
	if (quota != 10390) {
		printf ("ERROR: expected to find quota=10390 but found %d\n", quota);
		return axl_false;
	} /* end if */

	printf ("Test 09: sending a bit more of messages..\n");
	iterator = 0;
	while (iterator < 8) {
		printf ("Test 09: sending message...(iterator=%d, message=%d)\n", iterator, (int) strlen (message));
		if (! myqtt_conn_pub (conn, "myqtt/test", (axlPointer) message, (int) strlen (message), MYQTT_QOS_2, axl_false, 10))
			printf ("ERROR: failed to publish message in reply..\n");

		/* next iterator */
		iterator++;
	} /* end while */

	/* check quotas here */
	quota = myqtt_storage_queued_messages_quota_offline (domain->myqtt_ctx, "test_05");
	printf ("Test 09: used quota so fat..by test_05 (%d)\n", quota);
	if (quota != 18702) {
		printf ("ERROR: expected to find quota=18702 but found %d\n", quota);
		return axl_false;
	} /* end if */

	printf ("Test 09: sending a bit more of messages (we should reach quota allowed know)..\n");
	iterator = 0;
	while (iterator < 10) {
		printf ("Test 09: sending message...(iterator=%d, message=%d)\n", iterator, (int) strlen (message));
		if (! myqtt_conn_pub (conn, "myqtt/test", (axlPointer) message, (int) strlen (message), MYQTT_QOS_2, axl_false, 10))
			printf ("ERROR: failed to publish message in reply..\n");

		/* next iterator */
		iterator++;
	} /* end while */

	/* check quotas here */
	quota = myqtt_storage_queued_messages_quota_offline (domain->myqtt_ctx, "test_05");
	printf ("Test 09: used quota so fat..by test_05 (%d)\n", quota);
	if (quota != 19741) {
		printf ("ERROR: expected to find quota=19741 but found %d\n", quota);
		return axl_false;
	} /* end if */

	value = PTR_TO_INT (myqtt_ctx_get_data (domain->myqtt_ctx, "test_05_str:qt"));
	printf ("Test 09: stored cached value for test_05 is: %d\n", value);

	/* close message */
	printf ("Test 09: closing connection..\n");
	myqtt_conn_close (conn);

	/* connect again with test_05 identifier */
	queue = myqtt_async_queue_new ();
	myqtt_ctx_set_on_msg (myqtt_ctx, common_queue_message_received, queue);
	conn = myqtt_conn_new (myqtt_ctx, "test_05", axl_false, 30, listener_host, listener_port, NULL, NULL, NULL);
	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: unable to connect to %s:%s..\n", listener_host, listener_port);
		return axl_false;
	} /* end if */

	/* check client id subscribed in main context and in the domain activated */
	iterator = 0;
	while (iterator < 19) {
		/* check and receive */
		if (! common_receive_and_check (queue, "myqtt/test", message, MYQTT_QOS_0, axl_false)) {
			printf ("ERROR: expected to receive a message but something failed..\n");
			return axl_false;
		} /* end if */

		iterator++;
	} /* end while */

	/* check quotas here */
	iterator = 0;
	while (iterator < 10) {
		iterator++;

		quota = myqtt_storage_queued_messages_quota_offline (domain->myqtt_ctx, "test_05");
		printf ("Test 09: used quota so fat..by test_05 (%d)\n", quota);
		if (quota != 0) {
			/* wait a bit..*/
			if (iterator < 10) {
				myqtt_async_queue_timedpop (queue, 100000);
		
				continue;
			} /* end if */

			printf ("ERROR: expected to find quota=0 but found %d\n", quota);
			return axl_false;
		} /* end if */


	}

	iterator = 0;
	while (iterator < 10) {
		iterator++;

		value = PTR_TO_INT (myqtt_ctx_get_data (domain->myqtt_ctx, (axlPointer) "test_05_str:qt"));
		printf ("Test 09: stored cached value for test_05 after removing is: %d\n", value);
		if (value != 0) {
			/* wait a bit..*/
			if (iterator < 10) {
				myqtt_async_queue_timedpop (queue, 100000);
		
				continue;
			} /* end if */

			printf ("ERROR: expected to find stored value of 0 after recovering all messages.. but found %d\n",
				value);
			return axl_false;
		} /* end if */
	} /* end if */


	myqtt_conn_close (conn);

	/* release queue */
	myqtt_async_queue_unref (queue);

	myqtt_exit_ctx (myqtt_ctx, axl_true);
	printf ("Test 09: finishing MyQttdCtx..\n");
	/* finish server */
	myqttd_exit (ctx, axl_true, axl_true);
	
	return axl_true;
}

axl_bool  test_10 (void) {
	
	MyQttdCtx       * ctx;
	MyQttCtx        * myqtt_ctx;
	MyQttConn       * conn;
	MyQttConn       * conn2;
	MyQttdDomain    * domain;

	/* call to init the base library and close it */
	printf ("Test 10: init library and server engine (using test_02.conf)..\n");
	ctx       = common_init_ctxd (NULL, "test_10.conf");
	if (ctx == NULL) {
		printf ("Test 00: failed to start library and server engine..\n");
		return axl_false;
	} /* end if */

	myqtt_ctx = common_init_ctx ();
	if (! myqtt_init_ctx (myqtt_ctx)) {
		printf ("Error: unable to initialize MyQtt library..\n");
		return axl_false;
	} /* end if */
	
	/* connect and subscribe */
	conn = myqtt_conn_new (myqtt_ctx, "test_05", axl_false, 30, listener_host, listener_port, NULL, NULL, NULL);
	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: unable to connect to %s:%s..\n", listener_host, listener_port);
		return axl_false;
	} /* end if */

	conn2 = myqtt_conn_new (myqtt_ctx, "test_05", axl_false, 30, listener_host, listener_port, NULL, NULL, NULL);
	if (myqtt_conn_is_ok (conn2, axl_false)) {
		printf ("ERROR: it shouldn't have connected but it did connect to %s:%s..\n", listener_host, listener_port);
		return axl_false;
	} /* end if */
	myqtt_conn_close (conn2);

	if (! common_send_msg (conn, "this/is/a/test", "test", MYQTT_QOS_0)) {
		printf ("ERROR: expected to be able to send a message but it failed..\n");
		return axl_false;
	} /* end if */

	/* now get domain where this user was enabled */
	domain = myqttd_domain_find_by_name (ctx, "test_03.context");
	if (domain == NULL) {
		printf ("ERROR: unable to find test_03.context domain, mqyttd_domain_find_by_name ()..\n");
		return axl_false;
	} /* end if */

	/* change settings dinamically */
	printf ("Test 10: change settings to drop previous connections..\n");
	domain->settings->drop_conn_same_client_id = axl_true;

	conn2 = myqtt_conn_new (myqtt_ctx, "test_05", axl_false, 30, listener_host, listener_port, NULL, NULL, NULL);
	if (! myqtt_conn_is_ok (conn2, axl_false)) {
		printf ("ERROR: it SHOULD have connected but it didn't connect to %s:%s..\n", listener_host, listener_port);
		return axl_false;
	} /* end if */

	if (! common_send_msg (conn2, "this/is/a/test", "test", MYQTT_QOS_0)) {
		printf ("ERROR: expected to be able to send a message but it failed (conn2)..\n");
		return axl_false;
	} /* end if */

	if (common_send_msg (conn, "this/is/a/test", "test", MYQTT_QOS_2)) {
		printf ("ERROR: expected to NOT be able to send a message but it did (conn)..\n");
		return axl_false;
	} /* end if */

	myqtt_conn_close (conn);
	myqtt_conn_close (conn2);

	myqtt_exit_ctx (myqtt_ctx, axl_true);
	printf ("Test 10: finishing MyQttdCtx..\n");
	/* finish server */
	myqttd_exit (ctx, axl_true, axl_true);
	
	return axl_true;
}

#if defined(ENABLE_TLS_SUPPORT)
axl_bool  test_11 (void) {
	
	MyQttAsyncQueue * queue;
	MyQttAsyncQueue * queue2;
	MyQttdCtx       * ctx;
	MyQttCtx        * myqtt_ctx;
	MyQttConn       * conn;
	MyQttConn       * conn2;
	MyQttConnOpts   * opts;
	int               sub_result;
	/* MyQttdDomain    * domain;*/

	/* cleanup test_01 storage */
	if (system ("find reg-test-01/storage/test_01 -type f  -exec rm {} \\;") != 0) {
		/* printf ("ERROR: failed to initialize test..\n");
		   return axl_false; */
	} /* end if */

	/* cleanup test_01 storage */
	if (system ("find reg-test-02/storage/test_01 -type f  -exec rm {} \\;") != 0) {
		/* printf ("ERROR: failed to initialize test..\n");
		   return axl_false; */
	} /* end if */

	/* call to init the base library and close it */
	printf ("Test 11: init library and server engine (using test_02.conf)..\n");
	ctx       = common_init_ctxd (NULL, "test_10.conf");
	if (ctx == NULL) {
		printf ("Test 00: failed to start library and server engine..\n");
		return axl_false;
	} /* end if */

	myqtt_ctx = common_init_ctx ();
	if (! myqtt_init_ctx (myqtt_ctx)) {
		printf ("Error: unable to initialize MyQtt library..\n");
		return axl_false;
	} /* end if */
	
	/* disable verification */
	opts = myqtt_conn_opts_new ();
	myqtt_tls_opts_ssl_peer_verify (opts, axl_false);

	/* configure the announced SNI */
	myqtt_tls_opts_set_server_name (opts, "test_01.context");

	/* do a simple connection */
	conn = myqtt_tls_conn_new (myqtt_ctx, "test_01", axl_false, 30, listener_host, listener_tls_port, opts, NULL, NULL);
	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: expected being able to connect to %s:%s..\n", listener_host, listener_tls_port);
		return axl_false;
	} /* end if */

	/* create queue */
	queue  = common_configure_reception (conn);

	/* subscribe to the topic to get notifications */
	if (! myqtt_conn_sub (conn, 10, "myqtt/test", 0, &sub_result)) {
		printf ("ERROR: unable to subscribe, myqtt_conn_sub () failed, sub_result=%d", sub_result);
		return axl_false;
	} /* end if */	

	if (! common_send_msg (conn, "myqtt/test", "test", MYQTT_QOS_0)) {
		printf ("ERROR: expected to be able to send a message but it failed..\n");
		return axl_false;
	} /* end if */

	/* check message */
	if (! common_receive_and_check (queue, "myqtt/test", "test", MYQTT_QOS_0, axl_false)) {
		printf ("ERROR: expected to receive different message..\n");
		return axl_false;
	}

	printf ("Test 11: checking context activated.. (it should be test_01.context)\n");
	/* configure the on publish */
	myqttd_ctx_add_on_publish (ctx, test_04_handle_publish, NULL);
	if (! common_send_msg (conn, "get-context", "test", MYQTT_QOS_0)) {
		printf ("ERROR: expected to be able to send a message but it failed..\n");
		return axl_false;
	} /* end if */

	/* check message */
	if (! common_receive_and_check (queue, "get-context", "test_01.context", MYQTT_QOS_0, axl_false)) {
		printf ("ERROR: expected to receive different message (test_01.context)..\n");
		return axl_false;
	}

	/* disable verification */
	opts = myqtt_conn_opts_new ();
	myqtt_tls_opts_ssl_peer_verify (opts, axl_false);

	/* configure the announced SNI */
	myqtt_tls_opts_set_server_name (opts, "test_02.context");

	/* do a simple connection */
	conn2 = myqtt_tls_conn_new (myqtt_ctx, "test_01", axl_false, 30, listener_host, listener_tls_port, opts, NULL, NULL);
	if (! myqtt_conn_is_ok (conn2, axl_false)) {
		printf ("ERROR: expected being able to connect to %s:%s..\n", listener_host, listener_tls_port);
		return axl_false;
	} /* end if */

	/* create queue */
	queue2  = common_configure_reception (conn2);

	if (! common_send_msg (conn2, "get-context", "test", MYQTT_QOS_0)) {
		printf ("ERROR: expected to be able to send a message but it failed..\n");
		return axl_false;
	} /* end if */

	/* check message */
	if (! common_receive_and_check (queue2, "get-context", "test_02.context", MYQTT_QOS_0, axl_false)) {
		printf ("Test 03: expected to receive different message (test_01.context)..\n");
		return axl_false;
	}

	/* release queue */
	myqtt_async_queue_unref (queue);
	myqtt_async_queue_unref (queue2);

	/* close connection */
	myqtt_conn_close (conn);
	myqtt_conn_close (conn2);

	myqtt_exit_ctx (myqtt_ctx, axl_true);
	printf ("Test 11: finishing MyQttdCtx..\n");
	/* finish server */
	myqttd_exit (ctx, axl_true, axl_true);
	
	return axl_true;
}
#endif

#if defined(ENABLE_WEBSOCKET_SUPPORT)
axl_bool  test_12 (void) {
	
	MyQttAsyncQueue * queue;
	MyQttAsyncQueue * queue2;
	MyQttdCtx       * ctx;
	MyQttCtx        * myqtt_ctx;
	MyQttConn       * conn;
	MyQttConn       * conn2;
	noPollConn      * nopoll_conn;
	noPollCtx       * nopoll_ctx;
	/* MyQttdDomain    * domain;*/

	/* cleanup test_01 storage */
	if (system ("find reg-test-01/storage/test_01 -type f  -exec rm {} \\;") != 0) {
		printf ("ERROR: failed to initialize test..\n");
		return axl_false;
	} /* end if */

	/* cleanup test_01 storage */
	if (system ("find reg-test-02/storage/test_01 -type f  -exec rm {} \\;") != 0) {
		printf ("ERROR: failed to initialize test..\n");
		return axl_false;
	} /* end if */

	/* call to init the base library and close it */
	printf ("Test 12: init library and server engine (using test_02.conf)..\n");
	ctx       = common_init_ctxd (NULL, "test_12.conf");
	if (ctx == NULL) {
		printf ("Test 00: failed to start library and server engine..\n");
		return axl_false;
	} /* end if */

	myqtt_ctx = common_init_ctx ();
	if (! myqtt_init_ctx (myqtt_ctx)) {
		printf ("Error: unable to initialize MyQtt library..\n");
		return axl_false;
	} /* end if */
	
	/* create first a noPoll connection, for that we need to
	   create a context */
	nopoll_ctx   = nopoll_ctx_new ();
	nopoll_conn  = nopoll_conn_new (nopoll_ctx, listener_host, listener_ws_port, "test_01.context", NULL, NULL, NULL);
	if (! nopoll_conn_is_ok (nopoll_conn)) {
		printf ("ERROR: failed to connect remote host through WebSocket..\n");
		return nopoll_false;
	} /* end if */

	/* now create MQTT connection using already working noPoll connection */
	printf ("Test 12: creating WeBsocket connection ..\n");
	conn = myqtt_web_socket_conn_new (myqtt_ctx, "test_01", axl_false, 30, nopoll_conn, NULL, NULL, NULL);
	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: expected being able to connect to %s:%s..\n", listener_ws_host, listener_ws_port);
		return axl_false;
	} /* end if */

	printf ("Test 12: connected, now checking..\n");

	/* create queue */
	queue  = common_configure_reception (conn);

	printf ("Test 12: sending get-context message..\n");
	myqttd_ctx_add_on_publish (ctx, test_04_handle_publish, NULL);
	if (! common_send_msg (conn, "get-context", "test", MYQTT_QOS_0)) {
		printf ("ERROR: expected to be able to send a message but it failed..\n");
		return axl_false;
	} /* end if */

	/* check message */
	printf ("Test 12: waiting for message reception..\n");
	if (! common_receive_and_check (queue, "get-context", "test_01.context", MYQTT_QOS_0, axl_false)) {
		printf ("Test 03: expected to receive different message (test_01.context)..\n");
		return axl_false;
	}

	/////////// STEP 2 //////////////
	nopoll_conn  = nopoll_conn_new (nopoll_ctx, listener_host, listener_ws_port, "test_02.context", NULL, NULL, NULL);
	if (! nopoll_conn_is_ok (nopoll_conn)) {
		printf ("ERROR: failed to connect remote host through WebSocket..\n");
		return nopoll_false;
	} /* end if */

	/* now create MQTT connection using already working noPoll connection */
	printf ("Test 12: creating WeBsocket connection (for test_02.context) ..\n");
	conn2 = myqtt_web_socket_conn_new (myqtt_ctx, "test_01", axl_false, 30, nopoll_conn, NULL, NULL, NULL);
	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: expected being able to connect to %s:%s..\n", listener_ws_host, listener_ws_port);
		return axl_false;
	} /* end if */

	printf ("Test 12: connected, now checking..\n");

	/* create queue */
	queue2  = common_configure_reception (conn2);

	printf ("Test 12: sending get-context message (test_02.context)..\n");
	if (! common_send_msg (conn2, "get-context", "test", MYQTT_QOS_0)) {
		printf ("ERROR: expected to be able to send a message but it failed..\n");
		return axl_false;
	} /* end if */

	/* check message */
	printf ("Test 12: waiting for message reception (test_02.context)..\n");
	if (! common_receive_and_check (queue2, "get-context", "test_02.context", MYQTT_QOS_0, axl_false)) {
		printf ("Test 03: expected to receive different message (test_01.context)..\n");
		return axl_false;
	}

	/* release queue */
	myqtt_async_queue_unref (queue);
	myqtt_async_queue_unref (queue2);

	myqtt_conn_close (conn);
	myqtt_conn_close (conn2);

	myqtt_exit_ctx (myqtt_ctx, axl_true);
	printf ("Test 12: finishing MyQttdCtx..\n");
	/* finish server */
	myqttd_exit (ctx, axl_true, axl_true);
	
	return axl_true;
}

axl_bool  test_13 (void) {
	
	MyQttAsyncQueue * queue;
	MyQttAsyncQueue * queue2;
	MyQttdCtx       * ctx;
	MyQttCtx        * myqtt_ctx;
	MyQttConn       * conn;
	MyQttConn       * conn2;
	noPollConn      * nopoll_conn;
	noPollCtx       * nopoll_ctx;
	noPollConnOpts  * opts;

	/* MyQttdDomain    * domain;*/

	/* cleanup test_01 storage */
	if (system ("find reg-test-01/storage/test_01 -type f  -exec rm {} \\;") != 0) {
		printf ("ERROR: failed to initialize test..\n");
		return axl_false;
	} /* end if */

	/* cleanup test_01 storage */
	if (system ("find reg-test-02/storage/test_01 -type f  -exec rm {} \\;") != 0) {
		printf ("ERROR: failed to initialize test..\n");
		return axl_false;
	} /* end if */

	/* call to init the base library and close it */
	printf ("Test 13: init library and server engine (using test_02.conf)..\n");
	ctx       = common_init_ctxd (NULL, "test_12.conf");
	if (ctx == NULL) {
		printf ("Test 00: failed to start library and server engine..\n");
		return axl_false;
	} /* end if */

	myqtt_ctx = common_init_ctx ();
	if (! myqtt_init_ctx (myqtt_ctx)) {
		printf ("Error: unable to initialize MyQtt library..\n");
		return axl_false;
	} /* end if */
	
	/* create first a noPoll connection, for that we need to
	   create a context */
	nopoll_ctx   = nopoll_ctx_new ();

	/* disable verification */
	opts = nopoll_conn_opts_new ();
	nopoll_conn_opts_ssl_peer_verify (opts, nopoll_false);

	nopoll_conn  = nopoll_conn_tls_new (nopoll_ctx, opts, listener_host, listener_wss_port, "test_01.context", NULL, NULL, NULL);
	if (! nopoll_conn_is_ok (nopoll_conn)) {
		printf ("ERROR: failed to connect remote host through WebSocket..\n");
		return nopoll_false;
	} /* end if */

	printf ("Test 13: waiting TLS connection to work (WebSocket)..waiting 4 seconds\n");
	if (! nopoll_conn_wait_until_connection_ready (nopoll_conn, 4)) {
		printf ("ERROR: TLS WebSocket session failed..\n");
		return nopoll_false;
	}

	if (! nopoll_conn_is_ready (nopoll_conn)) {
		printf ("ERROR: expected to find ready connection...\n");
		return nopoll_false;
	} /* end if */

	/* now create MQTT connection using already working noPoll connection */
	printf ("Test 13: creating WebSocket connection (with working nopoll conn %s:%s) ..\n", listener_host, listener_wss_port);
	conn = myqtt_web_socket_conn_new (myqtt_ctx, "test_01", axl_false, 30, nopoll_conn, NULL, NULL, NULL);
	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: expected being able to connect to %s:%s..\n", listener_ws_host, listener_ws_port);
		return axl_false;
	} /* end if */

	printf ("Test 13: connected, now checking..\n");

	/* create queue */
	queue  = common_configure_reception (conn);

	printf ("Test 13: sending get-context message..\n");
	myqttd_ctx_add_on_publish (ctx, test_04_handle_publish, NULL);
	if (! common_send_msg (conn, "get-context", "test", MYQTT_QOS_0)) {
		printf ("ERROR: expected to be able to send a message but it failed..\n");
		return axl_false;
	} /* end if */

	/* check message */
	printf ("Test 13: waiting for message reception..\n");
	if (! common_receive_and_check (queue, "get-context", "test_01.context", MYQTT_QOS_0, axl_false)) {
		printf ("Test 03: expected to receive different message (test_01.context)..\n");
		return axl_false;
	}

	/////////// STEP 2 //////////////
	/* disable verification */
	opts = nopoll_conn_opts_new ();
	nopoll_conn_opts_ssl_peer_verify (opts, nopoll_false);

	nopoll_conn  = nopoll_conn_tls_new (nopoll_ctx, opts, listener_host, listener_wss_port, "test_02.context", NULL, NULL, NULL);
	if (! nopoll_conn_is_ok (nopoll_conn)) {
		printf ("ERROR: failed to connect remote host through WebSocket..\n");
		return nopoll_false;
	} /* end if */

	/* now create MQTT connection using already working noPoll connection */
	printf ("Test 13: creating WeBsocket connection (for test_02.context) ..\n");
	conn2 = myqtt_web_socket_conn_new (myqtt_ctx, "test_01", axl_false, 30, nopoll_conn, NULL, NULL, NULL);
	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: expected being able to connect to %s:%s..\n", listener_ws_host, listener_ws_port);
		return axl_false;
	} /* end if */

	printf ("Test 13: connected, now checking..\n");

	/* create queue */
	queue2  = common_configure_reception (conn2);

	printf ("Test 13: sending get-context message (test_02.context)..\n");
	if (! common_send_msg (conn2, "get-context", "test", MYQTT_QOS_0)) {
		printf ("ERROR: expected to be able to send a message but it failed..\n");
		return axl_false;
	} /* end if */

	/* check message */
	printf ("Test 13: waiting for message reception (test_02.context)..\n");
	if (! common_receive_and_check (queue2, "get-context", "test_02.context", MYQTT_QOS_0, axl_false)) {
		printf ("Test 03: expected to receive different message (test_01.context)..\n");
		return axl_false;
	}

	/* release queue */
	myqtt_async_queue_unref (queue);
	myqtt_async_queue_unref (queue2);

	myqtt_conn_close (conn);
	myqtt_conn_close (conn2);

	myqtt_exit_ctx (myqtt_ctx, axl_true);
	printf ("Test 13: finishing MyQttdCtx..\n");
	/* finish server */
	myqttd_exit (ctx, axl_true, axl_true);
	
	return axl_true;
}

axl_bool  test_14 (void) {
	
	MyQttdCtx       * ctx;
	MyQttCtx        * myqtt_ctx;
	MyQttConn       * conn;
	noPollConn      * nopoll_conn;
	noPollCtx       * nopoll_ctx;
	MyQttAsyncQueue * queue;

	/* cleanup test_01 storage */
	if (system ("find reg-test-01/storage/test_01 -type f  -exec rm {} \\;") != 0) {
		printf ("ERROR: failed to initialize test..\n");
		return axl_false;
	} /* end if */


	/* MyQttdDomain    * domain;*/

	/* call to init the base library and close it */
	printf ("Test 14: init library and server engine (using test_02.conf)..\n");
	ctx       = common_init_ctxd (NULL, "test_12.conf");
	if (ctx == NULL) {
		printf ("Test 00: failed to start library and server engine..\n");
		return axl_false;
	} /* end if */

	myqtt_ctx = common_init_ctx ();
	if (! myqtt_init_ctx (myqtt_ctx)) {
		printf ("Error: unable to initialize MyQtt library..\n");
		return axl_false;
	} /* end if */
	
	/* create first a noPoll connection, for that we need to
	   create a context */
	nopoll_ctx   = nopoll_ctx_new ();

	printf ("Test 14: connecting to %s:%s (WebSocket)..\n", listener_ws_host, listener_ws_port);
	nopoll_conn  = nopoll_conn_new (nopoll_ctx, listener_ws_host, listener_ws_port, "test_01.context", NULL, NULL, NULL);
	if (! nopoll_conn_is_ok (nopoll_conn)) {
		printf ("ERROR: failed to connect remote host through WebSocket..\n");
		return nopoll_false;
	} /* end if */

	/* now create MQTT connection using already working noPoll connection */
	printf ("Test 14: creating WebSocket connection (with working nopoll conn %s:%s) ..\n", listener_host, listener_wss_port);
	conn = myqtt_web_socket_conn_new (myqtt_ctx, NULL, axl_false, 30, nopoll_conn, NULL, NULL, NULL);
	if (myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: expected connection failure, but a proper connection was found.\n");
		return axl_false;
	} /* end if */

	printf ("Test 14: not connected, good, now checking..\n");

	myqtt_conn_close (conn);

	printf ("Test 14: connecting to %s:%s (WebSocket) but without serverName indication..\n", listener_ws_host, listener_ws_port);
	nopoll_conn  = nopoll_conn_new (nopoll_ctx, listener_ws_host, listener_ws_port, NULL, NULL, NULL, NULL);
	if (! nopoll_conn_is_ok (nopoll_conn)) {
		printf ("ERROR: failed to connect remote host through WebSocket..\n");
		return nopoll_false;
	} /* end if */

	/* now create MQTT connection using already working noPoll connection */
	printf ("Test 14: creating WebSocket connection (with working nopoll conn %s:%s) ..\n", listener_host, listener_wss_port);
	conn = myqtt_web_socket_conn_new (myqtt_ctx, "test_01", axl_false, 30, nopoll_conn, NULL, NULL, NULL);
	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: expected connection OK, but a connection failure was found.\n");
		return axl_false;
	} /* end if */

	/* create queue */
	queue  = common_configure_reception (conn);

	/* configure server handler */
	myqttd_ctx_add_on_publish (ctx, test_04_handle_publish, NULL);

	if (! common_send_msg (conn, "get-context", "test", MYQTT_QOS_0)) {
		printf ("ERROR: expected to be able to send a message but it failed..\n");
		return axl_false;
	} /* end if */
	
	printf ("Test 14: waiting for message reception (test_01.context)..\n");
	if (! common_receive_and_check (queue, "get-context", "test_01.context", MYQTT_QOS_0, axl_false)) {
		printf ("Test 03: expected to receive different message (test_01.context)..\n");
		return axl_false;
	}
	myqtt_async_queue_unref (queue);
	myqtt_conn_close (conn);


	myqtt_exit_ctx (myqtt_ctx, axl_true);
	printf ("Test 14: finishing MyQttdCtx..\n");
	/* finish server */
	myqttd_exit (ctx, axl_true, axl_true);
	
	return axl_true;
}

#endif /* defined(ENABLE_WEBSOCKET_SUPPORT) */

#define CHECK_TEST(name) if (run_test_name == NULL || axl_cmp (run_test_name, name))

typedef axl_bool (* MyQttTestHandler) (void);

/** 
 * @brief Helper handler that allows to execute the function provided
 * with the message associated.
 * @param function The handler to be called (test)
 * @param message The message value.
 */
int run_test (MyQttTestHandler function, const char * message) {

	printf ("--- --: starting %s\n", message);

	if (function ()) {
		printf ("%s [   OK   ]\n", message);
	} else {
		printf ("%s [ FAILED ]\n", message);
		exit (-1);
	}
	return 0;
}


#ifdef AXL_OS_UNIX
void __block_test (int _signal) 
{
	MyQttAsyncQueue * queue;
	char * cmd;
	int    _value; 

	signal (_signal, __block_test);

	printf ("******\n");
	printf ("****** Received a signal=%d (the regression test is failing): pid %d..locking..!!!\n", _signal, myqtt_getpid ());
	printf ("******\n");

	/* block the caller */
	queue = myqtt_async_queue_new ();
	myqtt_async_queue_pop (queue);
	
	
	cmd = axl_strdup_printf ("gdb -ex \"attach %d\"", (int) myqtt_getpid ());
	_value = system (cmd);
	printf ("Gdb finished with %d\n", _value);
	axl_free (cmd); 
	signal (_signal, __block_test);
	return;
}
#endif


/** 
 * @brief General regression test to check all features inside myqtt
 */
int main (int argc, char ** argv)
{
	char * run_test_name = NULL;

	/* install default handling to get notification about
	 * segmentation faults */
#ifdef AXL_OS_UNIX
	signal (SIGSEGV, __block_test);
	signal (SIGABRT, __block_test);
#endif

	printf ("** MyQtt: A high performance open source MQTT implementation\n");
	printf ("** Copyright (C) 2015 Advanced Software Production Line, S.L.\n**\n");
	printf ("** Regression tests: %s \n",
		VERSION);
	printf ("** To gather information about time performance you can use:\n**\n");
	printf ("**     time ./test_01 [--help] [--debug] [--no-unmap] [--run-test=NAME]\n**\n");
	printf ("** To gather information about memory consumed (and leaks) use:\n**\n");
	printf ("**     >> libtool --mode=execute valgrind --leak-check=yes --show-reachable=yes --error-limit=no ./test_01 [--debug]\n**\n");
	printf ("** Providing --run-test=NAME will run only the provided regression test.\n");
	printf ("** Available tests: test_00, test_01, test_02, test_03, test_04, test_05\n");
	printf ("**                  test_06, test_07, test_08, test_09, test_10, test_11\n");
	printf ("**                  test_12, test_13, test_14\n");
	printf ("**\n");
	printf ("** Report bugs to:\n**\n");
	printf ("**     <myqtt@lists.aspl.es> MyQtt Mailing list\n**\n");

	/* uncomment the following four lines to get debug */
	while (argc > 0) {
		if (axl_cmp (argv[argc], "--help")) 
			exit (0);
		if (axl_cmp (argv[argc], "--debug")) 
			test_common_enable_debug = axl_true;
	
		if (argv[argc] && axl_memcmp (argv[argc], "--run-test", 10)) {
			run_test_name = argv[argc] + 11;
			printf ("INFO: running test: %s\n", run_test_name);
		}
		argc--;
	} /* end if */

	/* run tests */
	CHECK_TEST("test_00")
	run_test (test_00, "Test 00: basic context initialization");

	CHECK_TEST("test_01")
	run_test (test_01, "Test 01: basic domain initialization (selecting one domain based on connection settings)");

	CHECK_TEST("test_02")
	run_test (test_02, "Test 02: sending 50 messages with 50 connections subscribed (2500 messages received)");

	CHECK_TEST("test_03")
	run_test (test_03, "Test 03: domain selection (working with two domains based on client_id selection)");

	CHECK_TEST("test_04")
	run_test (test_04, "Test 04: domain selection (working with two domains based on user/password selection)");

	CHECK_TEST("test_05")
	run_test (test_05, "Test 05: domain selection (testing right users with wrong passwords)");

	CHECK_TEST("test_06")
	run_test (test_06, "Test 06: check loading domain settings");

	CHECK_TEST("test_07")
	run_test (test_07, "Test 07: testing connection limits to a domain");

	CHECK_TEST("test_08")
	run_test (test_08, "Test 08: test message size limit to a domain");

	CHECK_TEST("test_09")
	run_test (test_09, "Test 09: check storage quota enforcement (amount of messages that can be saved on disk)");

	CHECK_TEST("test_10")
	run_test (test_10, "Test 10: checking client id (<drop-conn-same-client-id value=\"no\" />)");

#if defined(ENABLE_TLS_SUPPORT)
	CHECK_TEST("test_11")
	run_test (test_11, "Test 11: checking domain activation when connected with TLS (hostname == domain name)");
#endif

#if defined(ENABLE_WEBSOCKET_SUPPORT)
	CHECK_TEST("test_12")
	run_test (test_12, "Test 12: checking domain activation when connected with WebSocket (hostname == domain name)");

	CHECK_TEST("test_13")
	run_test (test_13, "Test 13: checking domain activation when connected with WebSocket TLS (hostname == domain name)");

	CHECK_TEST("test_14")
	run_test (test_14, "Test 14: checks that authentication fails when provided right hostname but wrong credentials");
#endif
	

	/* check support to limit amount of subscriptions a user can
	 * do */

	/* check client ids registered in all contexts */

	/* check connection has storage initialized after connecting
	   to its domain */

	/* test message global quota limit to a domain */

	/* test message limits to a domain (it must reset day by day,
	   month by month) */

	/* test message filtering (by topic and by message) (it contains, it is equal) */

	/* domain activation when connected with WebSocket/WebSocketTLS when
	   providing same hostname as domain name */

	/* test support for port sharing (running MQTT, MQTT-tls,
	   WebSocket-MQTT..etc over the same port) */

	/* test wildcard subscribe */

	/* reload running service when received SIGHUP without
	   dropping any connection */

	printf ("All tests passed OK!\n");

	/* terminate */
	return 0;
	
} /* end main */
