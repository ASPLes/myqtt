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
at <myqtt@lists.aspl.es>."

MyQttCtx * ctx;

MyQttConn * make_connection (void)
{

	MyQttConn     * conn;
	const char    * proto = "mqtt";
	MyQttConnOpts * opts;

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
		conn = myqtt_conn_new (ctx, exarg_get_string ("client-id"), axl_true, 30, exarg_get_string ("host"), exarg_get_string ("port"), opts, NULL, NULL);
#if defined(ENABLE_TLS_SUPPORT)
	else if (axl_cmp (proto, "mqtt-tls")) {
		myqtt_tls_opts_ssl_peer_verify (opts, axl_false);

		/* create connection */
		conn = myqtt_tls_conn_new (ctx, exarg_get_string ("client-id"), axl_true, 30, exarg_get_string ("host"), exarg_get_string ("port"), opts, NULL, NULL);
#endif
	} else {
		printf ("ERROR: protocol not supported (%s), unable to connect to %s:%s\n", 
			proto, exarg_get_string ("host"), exarg_get_string ("port"));
		exit (-1);
	} /* end if */

	/* check connection */
	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: unable to connect to %s:%s..\n", exarg_get_string ("host"), exarg_get_string ("port"));
		return axl_false;
	} /* end if */

	/* report connection created */
	printf ("INFO: connection OK to %s:%s (%s)\n", exarg_get_string ("host"), exarg_get_string ("port"), proto);
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

	exarg_install_arg ("debug2", NULL, EXARG_NONE,
			   "Increase the level of log to console produced.");

	exarg_install_arg ("debug3", NULL, EXARG_NONE,
			   "Makes logs produced to console to inclue more information about the place it was launched.");

	exarg_install_arg ("color-debug", "c", EXARG_NONE,
			   "Makes console log to be colorified. Calling to this option makes --debug to be activated.");

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

	/* operation options */
	exarg_install_arg ("wait-publish", "w", EXARG_STRING,
			   "By default no wait is implemented when publishing. You can configure this option to wait for publish return code from server. Option is configured in seconds");
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

	/* exarg properly configured */
	return axl_true;
}

void client_handle_publish_operation (int argc, char ** argv)
{
	char          * arg;
	int             iterator;
	int             qos;
	const char    * topic;
	const char    * message;
	axl_bool        found;
	MyQttConn     * conn;
	axl_bool        retain       = axl_false;
	int             wait_publish = 0; /* by default do not wait */

	/* get argument */
	arg = exarg_get_string ("publish");

	/* check if it is a file */
	printf ("INFO: processing argument: %s\n", arg);
	if (myqtt_support_file_test (arg, FILE_EXISTS)) {
		printf ("INFO: reading publish info from file %s\n", arg);
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

	printf ("INFO: sending qos=%d, topic=%s, message=%s\n", qos, topic, message);

	/* connect to the remote server */
	conn = make_connection ();

	/* get retain status */
	if (exarg_is_defined ("enable-retain"))
		retain = axl_true;

	/* get wait publish configuration */
	if (exarg_is_defined ("wait-publish"))
		wait_publish = myqtt_support_strtod (exarg_get_string ("wait-publish"), NULL);

	printf ("INFO: wait_publish=%d, retain=%d\n", wait_publish, retain);

	/* send publish operation */
	if (! myqtt_conn_pub (conn, topic, (axlPointer) message, strlen (message), qos, retain, wait_publish)) {
		printf ("ERROR: unable to publish message to get client identifier..\n");
		exit (-1);
		return;
	} /* end if */

	printf ("INFO: message published, closing connection..\n");

	/* close connection */
	myqtt_conn_close (conn);

	return;
}

void client_handle_subscribe_operation (int argc, char ** argv)
{
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
