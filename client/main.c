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

#include <myqtt.h>

/* local private header */
#include <myqtt-ctx-private.h>

#if defined(ENABLE_TLS_SUPPORT)
#include <myqtt-tls.h>
#endif

#include <exarg.h>

#define HELP_HEADER "MyQtt Client: command line tool to MQTT servers \n\
Copyright (C) 2015  Advanced Software Production Line, S.L.\n\n"

#define POST_HEADER "\n\
If you have question, bugs to report, patches, you can reach us\n\
at <myqtt@lists.aspl.es>. \n\
\n\
Some examples: \n\n\
- Publish: \n\
  >> myqtt-client --host localhost --port 1883 --client-id aspl --username aspl --password test1234 --publish \"0,myqtt/this/is a test,This is a test message for this case\"\n\n\
- Subscribe:\n\
  >> myqtt-client --host localhost --port 1883 --client-id aspl --username aspl --password test1234 --subscribe \"0,myqtt/this/is a test\"\n\n\
"


MyQttCtx * ctx;
axl_bool   disable_ansi_chars = axl_false;
axl_bool   enable_debug       = axl_false;

#define msg(m,...)   do{_msg (ctx, __AXL_FILE__, __AXL_LINE__, m, ##__VA_ARGS__);}while(0)

void  _msg   (MyQttCtx * ctx, const char * file, int line, const char * format, ...)
{
	/* get myqttd context */
	va_list            args;

	/* do not print if NULL is received */
	if (format == NULL || ctx == NULL || ! enable_debug)
		return;

	/* check extended console log */
	if (! disable_ansi_chars) {
		fprintf (stdout, "\e[1;32mI: \e[0m");
	} else {
		fprintf (stdout, "I: ");
	} /* end if */
	
	va_start (args, format);
	
	/* report to console */
	vfprintf (stdout, format, args);

	va_end (args);
	va_start (args, format);

	va_end (args);

	fprintf (stdout, "\n");
	
	fflush (stdout);
	
	return;
}

MyQttConn * make_connection (void)
{

	MyQttConn     * conn;
	const char    * proto = "mqtt";
	MyQttConnOpts * opts;
	axl_bool        clean_session = axl_false;

	/* check if clean session is registered */
	if (exarg_is_defined ("clean-session"))
		clean_session = axl_true;

	/* get proto if defined */
	if (exarg_is_defined ("proto") && exarg_get_string ("proto"))
		proto = exarg_get_string ("proto");

	/* disable verification */
	opts = myqtt_conn_opts_new ();

	/* configure username and password */
	if (exarg_is_defined ("username") && exarg_is_defined ("username"))
		myqtt_conn_opts_set_auth (opts, exarg_get_string ("username"), exarg_get_string ("password"));

	/* create connection */
	if (axl_cmp (proto, "mqtt"))
		conn = myqtt_conn_new (ctx, exarg_get_string ("client-id"), clean_session, 30, exarg_get_string ("host"), exarg_get_string ("port"), opts, NULL, NULL);
#if defined(ENABLE_TLS_SUPPORT)
	else if (axl_cmp (proto, "mqtt-tls")) {
		myqtt_tls_opts_ssl_peer_verify (opts, axl_false);

		/* create connection */
		conn = myqtt_tls_conn_new (ctx, exarg_get_string ("client-id"), clean_session, 30, exarg_get_string ("host"), exarg_get_string ("port"), opts, NULL, NULL);
#endif
	} else {
		printf ("ERROR: protocol not supported (%s), unable to connect to %s:%s\n", 
			proto, exarg_get_string ("host"), exarg_get_string ("port"));
		exit (-1);
	} /* end if */

	/* check connection */
	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: unable to connect to %s:%s (error = %d : %s)..\n", exarg_get_string ("host"), exarg_get_string ("port"), 
			myqtt_conn_get_last_err (conn), myqtt_conn_get_code_to_err (myqtt_conn_get_last_err (conn)));
		exit (-1);
	} /* end if */

	/* report connection created */
	msg ("connection OK to %s:%s (%s)", exarg_get_string ("host"), exarg_get_string ("port"), proto);
	return conn;
}


/** 
 * @internal Init for all exarg functions provided by the myqttd
 * command line.
 */
int  main_init_exarg (int argc, char ** argv)
{
	/* install headers for help */
	exarg_add_usage_header  (HELP_HEADER);
	exarg_add_help_header   (HELP_HEADER);
	exarg_post_help_header  (POST_HEADER);
	exarg_post_usage_header (POST_HEADER);

	/* install default debug options. */
	exarg_install_arg ("version", "v", EXARG_NONE,
			   "Show myqttd version.");

	/* install default debug options. */
	exarg_install_arg ("debug", "d", EXARG_NONE,
			   "Makes all log produced by the application, to be also dropped to the console in sort form.");

	/* connection options */
	exarg_install_arg ("host", "h", EXARG_STRING,
			   "Location of the myqtt server.");

	exarg_install_arg ("port", "o", EXARG_STRING,
			   "Port where the myqtt server is running.");

	exarg_install_arg ("client-id", "i", EXARG_STRING,
			   "Client ID to use while connecting (if not provided, myqtt-client will create one for you).");

	exarg_install_arg ("username", "u", EXARG_STRING,
			   "Optional username to use when connecting to the server.");

	exarg_install_arg ("password", "x", EXARG_STRING,
			   "Optional password to use when connecting to the server. It can be also a file to read the password from.");

	exarg_install_arg ("protocol", "l", EXARG_STRING,
			   "By default MQTT protocol is used. You can especify any of the following: mqtt mqtt-tls mqtt-ws mqtt-wss.");

	/* operations */
	exarg_install_arg ("publish", "p", EXARG_STRING,
			   "Publish a new message to the server with the following format: qos,topic,message  Publish argument can also be a file that holds same format as the argument expresed before");
	exarg_install_arg ("subscribe", "s", EXARG_STRING,
			   "Subscribe to the provided topic: qos,topic");
	exarg_install_arg ("get-subscriptions", "n", EXARG_NONE,
			   "If supported by the server, allows to get current subscriptions registered by the connecting client.");
	exarg_install_arg ("get-msgs", "m", EXARG_NONE,
			   "Connect to the server and wait for messages to arrive. Every incoming message received will be printed to the console.");
	exarg_install_arg ("clean-session", "e", EXARG_NONE,
			   "Request client session on the next connect operation.");

	/* operation options */
	exarg_install_arg ("wait", "w", EXARG_STRING,
			   "By default 10secs wait is implemented when publishing or subscribing. You can configure this option to wait for publish/subscribe return code from server. Option is configured in seconds");
	exarg_install_arg ("enable-retain", "r", EXARG_NONE,
			   "Use this option to enable retain flag in publish operations");

	/* call to parse arguments */
	exarg_parse (argc, argv);

	/* check for version request */
	if (exarg_is_defined ("version")) {
		printf ("%s\n", VERSION);
		/* terminates exarg */
		exarg_end ();
		return axl_false;
	}

	/* enable debug */
	enable_debug = exarg_is_defined ("debug");

	/* exarg properly configured */
	return axl_true;
}

void __message_sent (MyQttCtx * ctx, MyQttConn * conn, unsigned char * msg, int msg_size, MyQttMsgType msg_type, axlPointer user_data)
{
	MyQttAsyncQueue * queue = user_data;
	msg ("Message sent..");
	myqtt_async_queue_push (queue, INT_TO_PTR (3));
	return;
}

void client_handle_publish_operation (int argc, char ** argv)
{
	char              * arg;
	int                 iterator;
	int                 qos;
	const char        * topic;
	const char        * message;
	axl_bool            found;
	MyQttConn         * conn;
	axl_bool            retain       = axl_false;
	int                 wait_publish = 10; /* by default wait 10 */
	MyQttAsyncQueue   * queue;

	/* get argument */
	arg = exarg_get_string ("publish");

	/* check if it is a file */
	msg ("processing argument: %s", arg);
	if (myqtt_support_file_test (arg, FILE_EXISTS)) {
		msg ("reading publish info from file %s", arg);
		/* still not implemented, read the content and leave it into arg */
	}

	/* prepare arguments */
	iterator = 0;
	found    = axl_false;
	while (arg && arg[iterator]) {
		if (arg[iterator] == ',') {
			arg[iterator] = 0;
			qos           = myqtt_support_strtod (arg, NULL);
			found         = axl_true;
			break;
		} /* end if */

		/* next position */
		iterator++;
	} /* end while */

	if (! found) {
		printf ("ERROR: Unable to find first QoS part. You must provide qos,topic,message\n");
		exit (-1);
	} /* end if */


	/* now find topic */
	found    = axl_false;
	iterator ++;
	topic    = arg + iterator;
	while (arg && arg[iterator]) {
		if (arg[iterator] == ',') {
			arg[iterator] = 0;
			found         = axl_true;
			break;
		} /* end if */

		/* next position */
		iterator++;
	} /* end while */

	if (! found) {
		printf ("ERROR: Unable to find second topic part. You must provide qos,topic,message\n");
		exit (-1);
	} /* end if */

	/* now find topic */
	found    = axl_false;
	iterator ++;
	message  = arg + iterator;

	msg ("sending qos=%d, topic=%s, message=%s", qos, topic, message);

	/* connect to the remote server */
	conn = make_connection ();

	/* get retain status */
	if (exarg_is_defined ("enable-retain"))
		retain = axl_true;

	/* get wait publish configuration */
	if (exarg_is_defined ("wait"))
		wait_publish = myqtt_support_strtod (exarg_get_string ("wait"), NULL);

	msg ("wait_publish=%d, retain=%d", wait_publish, retain);

	/* configure on complete message when qos is 0 to ensure the
	 * message is sent */
	if (qos == MYQTT_QOS_0) {
		queue = myqtt_async_queue_new ();
		myqtt_conn_set_on_msg_sent (conn, __message_sent, queue);
	}

	/* send publish operation */
	if (! myqtt_conn_pub (conn, topic, (axlPointer) message, strlen (message), qos, retain, wait_publish)) {
		printf ("ERROR: unable to publish message to get client identifier..\n");
		exit (-1);
		return;
	} /* end if */

	if (qos == MYQTT_QOS_0) {
		/* wait for message sent */
		myqtt_async_queue_timedpop (queue, wait_publish * 1000000);
	}

	msg ("message published, closing connection..");

	/* close connection */
	myqtt_conn_close (conn);

	return;
}

void client_handle_subscribe_operation (int argc, char ** argv)
{
	char          * arg;
	int             iterator;
	int             qos;
	const char    * topic;
	axl_bool        found;
	MyQttConn     * conn;
	int             subs_result = 0;
	int             wait_sub    = 10;

	/* get argument */
	arg = exarg_get_string ("subscribe");

	/* check if it is a file */
	msg ("processing argument: %s", arg);
	if (myqtt_support_file_test (arg, FILE_EXISTS)) {
		msg ("reading publish info from file %s", arg);
		/* still not implemented, read the content and leave it into arg */
	} /* end if */

	/* prepare arguments */
	iterator = 0;
	found    = axl_false;
	while (arg && arg[iterator]) {
		if (arg[iterator] == ',') {
			arg[iterator] = 0;
			qos           = myqtt_support_strtod (arg, NULL);
			found         = axl_true;
			break;
		} /* end if */

		/* next position */
		iterator++;
	} /* end while */

	if (! found) {
		printf ("ERROR: Unable to find first QoS part. You must provide qos,topic,message\n");
		exit (-1);
	} /* end if */


	/* now find topic */
	iterator ++;
	topic    = arg + iterator;

	msg ("subscribing to qos=%d, topic=%s", qos, topic);

	/* connect to the remote server */
	conn = make_connection ();

	/* get wait publish configuration */
	if (exarg_is_defined ("wait"))
		wait_sub = myqtt_support_strtod (exarg_get_string ("wait"), NULL);

	/* send publish operation */
	if (! myqtt_conn_sub (conn, wait_sub, topic, qos, &subs_result)) {
		printf ("ERROR: unable to subscribe to the provided topic..\n");
		exit (-1);
		return;
	} /* end if */

	msg ("user subscribed to %s, closing connection..", topic);

	/* close connection */
	myqtt_conn_close (conn);

	return;
}

void configure_reception_queue_received (MyQttCtx * ctx, MyQttConn * conn, MyQttMsg * msg, axlPointer user_data)
{
	MyQttAsyncQueue * queue = user_data;

	/* push message received */
	myqtt_msg_ref (msg);
	myqtt_async_queue_push (queue, msg);
	return;
}

MyQttAsyncQueue * configure_reception (MyQttConn * conn) {
	MyQttAsyncQueue * queue = myqtt_async_queue_new ();

	/* configure reception on queue  */
	myqtt_conn_set_on_msg (conn, configure_reception_queue_received, queue);

	return queue;
}

void client_handle_get_subscriptions_operation (int argc, char ** argv)
{
	MyQttConn       * conn;
	MyQttAsyncQueue * queue;
	MyQttMsg        * msg;
	axlDoc          * doc;
	axlNode         * node;

	/* create connection */
	conn  = make_connection ();
	queue = configure_reception (conn);
	
	/* send request */
	if (! myqtt_conn_pub (conn, "myqtt/my-status/get-subscriptions", "", 0, 0, axl_false, 20)) {
		printf ("ERROR: unable to publish message to get client identifier..\n");
		exit (-1);
		return;
	} /* end if */

	/* get message and release queue */
	msg   = myqtt_async_queue_timedpop (queue, 30000000);
	myqtt_async_queue_unref (queue);

	doc = axl_doc_parse ((const char *) myqtt_msg_get_app_msg (msg), myqtt_msg_get_app_msg_size (msg), NULL);
	myqtt_msg_unref (msg);
	if (doc == NULL) {
		printf ("ERROR: failed to parse subscriptions report received: %s\n", (const char *) myqtt_msg_get_app_msg (msg));
		exit (-1);
	} /* end if */

	msg ("subscriptions found: ");
	node = axl_doc_get (doc, "/reply/sub");
	while (node) {
		printf (" - %s (qos %s)\n", ATTR_VALUE (node, "topic"), ATTR_VALUE (node, "qos"));

		/* get next node called <sub /> */
		node = axl_node_get_next_called (node, "sub");
	}
	axl_doc_free (doc);

	/* close connection */
	myqtt_conn_close (conn);
	return;
	
}

void client_handle_get_msgs_operation (int argc, char ** argv)
{
	MyQttConn       * conn;
	MyQttAsyncQueue * queue;
	MyQttMsg        * msg;

	/* create connection */
	conn  = make_connection ();
	queue = configure_reception (conn);
	
	while (axl_true) {
		/* get message and release queue */
		msg   = myqtt_async_queue_timedpop (queue, 30000000);
		if (msg == NULL)
			continue;

		printf ("Id: %d\nTopic: %s\nMessage: %s\n\n", 
			myqtt_msg_get_id (msg), 
			myqtt_msg_get_topic (msg),
			(const char *) myqtt_msg_get_app_msg (msg));

		/* release message */
		myqtt_msg_unref (msg);
	} /* end while */

	/* close connection */
	myqtt_conn_close (conn);
	
	return;
}

int main (int argc, char ** argv)
{
	/*** init exarg library ***/
	if (! main_init_exarg (argc, argv))
		return 0;

	/* create the myqttd and myqtt context */
	ctx = myqtt_ctx_new ();
	if (! myqtt_init_ctx (ctx)) {
		printf ("ERROR: unable to initialize myqtt library, unable to start");
		return -1;
	} /* end if */

	/* check for operations */
	if (exarg_is_defined ("publish"))
		client_handle_publish_operation (argc, argv);
	else if (exarg_is_defined ("subscribe"))
		client_handle_subscribe_operation (argc, argv);
	else if (exarg_is_defined ("get-subscriptions"))
		client_handle_get_subscriptions_operation (argc, argv);
	else if (exarg_is_defined ("get-msgs"))
		client_handle_get_msgs_operation (argc, argv);
	else {
		printf ("ERROR: no operation defined, please run %s --help to get information\n", argv[0]);
		exit (-1);
	} /* end if */

	/* terminate exarg */
	exarg_end ();

	/* free context (the very last operation) */
	myqtt_exit_ctx (ctx, axl_true);

	return 0;
}
