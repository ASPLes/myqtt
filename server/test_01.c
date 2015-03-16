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

axl_bool test_common_enable_debug = axl_false;

/* default listener location */
const char * listener_host = "localhost";
const char * listener_port = "1883";

MyQttCtx * init_ctx (void)
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

MyQttdCtx * init_ctxd (MyQttCtx * myqtt_ctx, const char * config)
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
	myqtt_ctx = init_ctx ();
	ctx       = init_ctxd (myqtt_ctx, "myqtt.example.conf");
	if (ctx == NULL) {
		printf ("Test 00: failed to start library and server engine..\n");
		return axl_false;
	} /* end if */

	printf ("Test 00: library and server engine started.. ok\n");

	/* now close the library */
	myqttd_exit (ctx, axl_true, axl_true);
		
	return axl_true;
}

void queue_message_received (MyQttConn * conn, MyQttMsg * msg, axlPointer user_data)
{
	MyQttAsyncQueue * queue = user_data;

	/* push message received */
	myqtt_msg_ref (msg);
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
	
	/* call to init the base library and close it */
	printf ("Test 01: init library and server engine..\n");
	ctx       = init_ctxd (NULL, "test_01.conf");
	if (ctx == NULL) {
		printf ("Test 00: failed to start library and server engine..\n");
		return axl_false;
	} /* end if */

	printf ("Test 01: library and server engine started.. ok\n");

	/* create connection to local server and test domain support */
	myqtt_ctx = init_ctx ();
	if (! myqtt_init_ctx (myqtt_ctx)) {
		printf ("Error: unable to initialize MyQtt library..\n");
		return axl_false;
	} /* end if */

	printf ("Test 01: connecting to myqtt server..\n");
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
	myqtt_conn_set_on_msg (conn, queue_message_received, queue);

	/* push a message */
	printf ("Test 01: publishing to the topic myqtt/test..\n");
	if (! myqtt_conn_pub (conn, "myqtt/test", "This is test message....", 24, MYQTT_QOS_0, axl_false, 0)) {
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
	if (myqtt_msg_get_app_msg_size (msg) != 24) {
		printf ("ERROR: expected payload size of 24 but found %d\n", myqtt_msg_get_app_msg_size (msg));
		return axl_false;
	} /* end if */

	if (myqtt_msg_get_type (msg) != MYQTT_PUBLISH) {
		printf ("ERROR: expected to receive PUBLISH message but found: %s\n", myqtt_msg_get_type_str (msg));
		return axl_false;
	} /* end if */

	/* check content */
	if (! axl_cmp ((const char *) myqtt_msg_get_app_msg (msg), "This is test message....")) {
		printf ("ERROR: expected to find different content..\n");
		return axl_false;
	} /* end if */

	/* close connection */
	myqtt_conn_close (conn);
	myqtt_exit_ctx (myqtt_ctx, axl_true);

	/* finish server */
	myqttd_exit (ctx, axl_true, axl_true);
		
	return axl_true;
}


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

/** 
 * @brief General regression test to check all features inside myqtt
 */
int main (int argc, char ** argv)
{
	char * run_test_name = NULL;

	printf ("** MyQtt: A high performance open source MQTT implementation\n");
	printf ("** Copyright (C) 2014 Advanced Software Production Line, S.L.\n**\n");
	printf ("** Regression tests: %s \n",
		VERSION);
	printf ("** To gather information about time performance you can use:\n**\n");
	printf ("**     time ./test_01 [--help] [--debug] [--no-unmap] [--run-test=NAME]\n**\n");
	printf ("** To gather information about memory consumed (and leaks) use:\n**\n");
	printf ("**     >> libtool --mode=execute valgrind --leak-check=yes --show-reachable=yes --error-limit=no ./test_01 [--debug]\n**\n");
	printf ("** Providing --run-test=NAME will run only the provided regression test.\n");
	printf ("** Available tests: test_00\n");
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

	printf ("All tests passed OK!\n");

	/* terminate */
	return 0;
	
} /* end main */
