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

#ifdef AXL_OS_UNIX
#include <signal.h>
#endif

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

axl_bool test_common_enable_debug = axl_false;

/* default listener location */
const char * listener_host = "localhost";
const char * listener_port = "1909";

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

/** 
 * @brief General regression test to check all features inside myqtt
 */
int main (int argc, char ** argv)
{
	MyQttConn * listener;

	printf ("** MyQtt: A high performance open source MQTT implementation\n");
	printf ("** Copyright (C) 2014 Advanced Software Production Line, S.L.\n**\n");
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
		argc--;
	} /* end if */

	/* call to init the base library and close it */
	ctx = init_ctx ();

	/* start a listener */
	listener = myqtt_listener_new (ctx, listener_host, listener_port, NULL, NULL);
	if (! myqtt_conn_is_ok (listener, axl_false)) {
		printf ("ERROR: failed to start listener at: %s:%s..\n", listener_host, listener_port);
		exit (-1);
	} /* end if */

	/* wait for listeners */
	printf ("Ready and accepting connections..OK\n");
	myqtt_listener_wait (ctx);

	/* now close the library */
	myqtt_exit_ctx (ctx, axl_true);

	/* terminate */
	return 0;
	
} /* end main */
