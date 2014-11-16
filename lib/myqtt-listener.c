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
#if defined(AXL_OS_UNIX)
#include <netdb.h>
#endif

/* local include */
#include <myqtt-ctx-private.h>
#include <myqtt-conn-private.h>

#define LOG_DOMAIN "myqtt-listener"

typedef struct _MyQttListenerOnAcceptData {
	MyQttOnAcceptedConnection on_accept;
	axlPointer                 on_accept_data;
}MyQttListenerOnAcceptData;

int  __myqtt_listener_get_port (const char  * port)
{
	return strtol (port, NULL, 10);
}

/** 
 * \defgroup myqtt_listener MyQtt Listener: Set of functions to create MQTT Listeners (server applications that accept incoming requests)
 */

/** 
 * \addtogroup myqtt_listener
 * @{
 */

/** 
 * @brief Common task to be done to accept a connection before
 * greetings message is issued while working as a Listener.
 * 
 * @param connection The connection to accept.
 *
 * @param send_greetings If the greetings message must be sent or not.
 */
void myqtt_listener_accept_connection    (MyQttConn * connection, axl_bool  send_greetings)
{
	MyQttCtx                  * ctx;
	axl_bool                     result;
	int                          iterator;
	MyQttListenerOnAcceptData * data;
	MyQttOnAcceptedConnection   on_accept;
	axlPointer                   on_accept_data;

	/* check received reference */
	if (connection == NULL)
		return;

	/* Get a reference to the connection, accept the new
	 * connection under the same domain as the context of the
	 * listener  */
	ctx = myqtt_conn_get_ctx (connection);
	
	/* call to the handler defined */
	iterator = 0;
	result   = axl_true;

	/* init lock */
	myqtt_mutex_lock (&ctx->listener_mutex);
	while (iterator < axl_list_length (ctx->listener_on_accept_handlers)) {
		/* call and check */
		data = axl_list_get_nth (ctx->listener_on_accept_handlers, iterator);

		/* internal check */
		if (data == NULL) {
			result = axl_false;
			break;
		} /* end if */

		/* get safe references to handlers */
		on_accept      = data->on_accept;
		on_accept_data = data->on_accept_data;

		/* unlock during notification */
		myqtt_mutex_unlock (&ctx->listener_mutex);

		/* check if the following handler accept the incoming
		 * connection */
		myqtt_log (MYQTT_LEVEL_DEBUG, "calling to accept connection id=%d, handler: %p, data: %p",
			    myqtt_conn_get_id (connection), data->on_accept, data->on_accept_data);
		if (! on_accept (connection, on_accept_data)) {

			/* init lock */
			myqtt_mutex_lock (&ctx->listener_mutex);

			myqtt_log (MYQTT_LEVEL_DEBUG, "on accept handler have denied to accept the connection, handler: %p, data: %p",
				    on_accept, on_accept_data);

			/* found that at least one handler do not
			 * accept the incoming connection, dropping */
			result = axl_false;
			break;
		}

		/* init lock */
		myqtt_mutex_lock (&ctx->listener_mutex);
		
		/* next iterator */
		iterator++;

	} /* end while */

	/* unlock */
	myqtt_mutex_unlock (&ctx->listener_mutex);

	/* check result */
	if (! result) {
		/* check connection status */
		if (myqtt_conn_is_ok (connection, axl_false)) {
			myqtt_log (MYQTT_LEVEL_CRITICAL, "the server application level have dropped the provided connection");
		} /* end if */

		/* flag the connection to be not connected */
		__myqtt_conn_shutdown_and_record_error (
			connection,  MyQttConnectionFiltered, "connection filtered by on accept handler");

		myqtt_conn_unref (connection, "myqtt listener");
		return;

	} /* end if */

	/* close connection and free resources */
	myqtt_log (MYQTT_LEVEL_DEBUG, "worker ended, connection (conn-id=%d) registered on manager (initial accept)",
		    myqtt_conn_get_id (connection));

	/* call to complete incoming connection register operation */
	myqtt_listener_complete_register (connection);
	
	return;
}

/** 
 * @internal Fucntion that allows to complete last parts once a
 * connection is accepted.
 */
void          myqtt_listener_complete_register    (MyQttConn     * connection)
{
	MyQttCtx * ctx;

	/* check connection received */
	if (connection == NULL)
		return;

	ctx = myqtt_conn_get_ctx (connection);

	/* flag the connection to be on initial step */
	connection->initial_accept = axl_true;

	/*
	 * register the connection on myqtt reader from here, the
	 * connection get a non-blocking state
	 */
	myqtt_reader_watch_connection      (ctx, connection);

	/*
	 * Because this is a listener, we don't want to pay attention
	 * to free connection on errors. connection already have 1
	 * reference (reader), so let's reference counting to the job
	 * of free connection resources.
	 */
	myqtt_conn_unref (connection, "myqtt listener (initial accept)");

	return;
}

void __myqtt_listener_release_master_ref (axlPointer ptr)
{
	/* release master reference */
	myqtt_conn_unref ((MyQttConn *) ptr, "master release");
	return;
}

/** 
 * @internal
 *
 * Internal myqtt library function. This function does the initial
 * accept for a new incoming connection. New connections are accepted
 * through two steps: an initial accept and final negotiation. A
 * connection to be totally accepted must step over these two steps.
 *
 * The reason to accept connections following this procedure is due to
 * MyQtt Library way of reading data from all connections. 
 * 
 * Reading data from remote MQTT peers is done inside the myqtt
 * reader which, basicly, is a loop executing a "select" call. 
 *
 * While reading data from those sockets accepted by the "select" call
 * is not a problem, however, it is actually a problem to accept new
 * connections because it implies reading data from remote peer: the
 * initial greeting, and writing data to remote peer: the initial
 * greeting response. 
 *
 * During the previous negotiation a malicious client can make
 * negotiation to be stopped, or sending data in an slow manner,
 * making the select loop to be blocked, even stopped. As a
 * consequence this malicious client could throw down the reception
 * for all connections.
 *
 * However, the myqtt reader loop is prepared to avoid this problem
 * with already accepted connections because it doesn't pay attention
 * to those connection which are not sending any data and also support
 * to receive fragmented msgs which are stored to be joined with
 * future fragment. Once connection is accepted the myqtt reader is
 * strong enough to avoid DOS (denial of service) attacks (well, it
 * should be ;-).
 *
 * That the mission for the first step: to only accept the new
 * connection and send the initial greeting to remote peer and *DO NOT
 * READING ANYTHING* to avoid DOS. On a second step, the response
 * reading is done and the connection is totally accepted in the
 * context of the myqtt reader.
 *
 * A connection initial accepted is flagged to be on that step so
 * myqtt reader can recognize it. 
 *
 * @param client_socket A new socket being accepted to be read.
 *
 * @param listener The listener where the operation was accepted.
 *
 * @param receive_handler Optional receive handler to be configured on
 * the connection accepted
 *
 * @param send_handler Optional send handler to be configured on the
 * connection accepted
 *
 * @param user_data_key Optional key label for the data to be
 * associated to the connection provided.
 *
 * @param user_data Optional user pointer reference, associated to the
 * connection with the label provided.
 *
 * @return A reference to the connection initially accepted.
 */
MyQttConn * __myqtt_listener_initial_accept (MyQttCtx            * ctx,
						     MYQTT_SOCKET          client_socket, 
						     MyQttConn     * listener,
						     axl_bool               register_conn)
{
	MyQttConn     * connection = NULL;

	/* before doing anything, we have to create a connection */
	connection = myqtt_conn_new_empty (ctx, client_socket, MyQttRoleListener);
	myqtt_log (MYQTT_LEVEL_DEBUG, "received connection from: %s:%s (conn-id=%d)", 
		    myqtt_conn_get_host (connection),
		    myqtt_conn_get_port (connection),
		    myqtt_conn_get_id (connection));

	/* configure the relation between this connection and the
	 * master listener connection */
	if (myqtt_conn_ref (listener, "master ref"))
		myqtt_conn_set_data_full (connection, "_vo:li:master", listener, NULL, __myqtt_listener_release_master_ref);

	/* call to register the connection */
	if (! register_conn)
		return connection;

	/*
	 * Perform an initial accept, flagging the connection to be
	 * into the initial accept stage, and send the initial greetings.
	 */
	myqtt_listener_accept_connection (connection, axl_true);

	return connection;
}

/** 
 * @internal
 *
 * This function is for internal myqtt library purposes. This
 * function actually does the second accept step, that is, to read the
 * greeting response and finally accept the connection is that response
 * is ok.
 *
 * You can also read the doc for __myqtt_listener_initial_accept to
 * get an idea about the initial step.
 *
 * Once the greeting response is ok, the function unflag this
 * connection to be "being accepted" so the connection starts to work.
 * 
 * @param msg The msg which should contains the greeting response
 * @param connection the connection being on initial accept step
 */
void __myqtt_listener_second_step_accept (MyQttMsg * msg, MyQttConn * connection)
{


	/** check incoming content here */
	
	/*** CONNECTION COMPLETELY ACCEPTED ***/
	/* flag the connection to be totally accepted. */
	connection->initial_accept = axl_false;

	return;	
}

/** 
 * @brief Public function that performs a TCP listener accept.
 *
 * @param server_socket The listener socket where the accept() operation will be called.
 *
 * @return Returns a connected socket descriptor or -1 if it fails.
 */
MYQTT_SOCKET myqtt_listener_accept (MYQTT_SOCKET server_socket)
{
	struct sockaddr_storage inet_addr;
#if defined(AXL_OS_WIN32)
	int               addrlen = sizeof (inet_addr);
#else
	socklen_t         addrlen = sizeof (inet_addr);
#endif

	/* accept the connection new connection */
	return accept (server_socket, (struct sockaddr *)&inet_addr, &addrlen);
}

void myqtt_listener_accept_connections (MyQttCtx        * ctx,
					 int                server_socket, 
					 MyQttConn * listener)
{
	int   soft_limit, hard_limit, client_socket;

	/* accept the connection new connection */
	client_socket = myqtt_listener_accept (server_socket);
	if (client_socket == MYQTT_SOCKET_ERROR) {
		/* get values */
		myqtt_conf_get (ctx, MYQTT_SOFT_SOCK_LIMIT, &soft_limit);
		myqtt_conf_get (ctx, MYQTT_HARD_SOCK_LIMIT, &hard_limit);

		myqtt_log (MYQTT_LEVEL_CRITICAL, "accept () failed, server_socket=%d, soft-limit=%d, hard-limit=%d: (errno=%d) %s\n",
			    server_socket, soft_limit, hard_limit, errno, myqtt_errno_get_last_error ());
		return;
	} /* end if */

	/* check we can support more sockets, if not close current
	 * connection: function already closes client socket in the
	 * case of failure */
	if (! myqtt_conn_check_socket_limit (ctx, client_socket))
		return;

	/* instead of negotiate the connection at this point simply
	 * accept it to negotiate it inside myqtt_reader loop.  */
	__myqtt_listener_initial_accept (myqtt_conn_get_ctx (listener), client_socket, listener, axl_true);

	return;
}

typedef struct _MyQttListenerData {
	char                     * host;
	int                        port;
	MyQttListenerReady         on_ready;
	axlPointer                 user_data;
	axl_bool                   threaded;
	axl_bool                   register_conn;
	MyQttCtx                 * ctx;
	MyQttNetTransport          transport;
}MyQttListenerData;

/** 
 * @internal Function used to create a listen process.
 */
MYQTT_SOCKET     myqtt_listener_sock_listen_common      (MyQttCtx            * ctx,
							   const char           * host,
							   const char           * port,
							   axlError            ** error,
							   MyQttNetTransport     transport)
{
	struct hostent     * he    = NULL;
	struct in_addr     * haddr = NULL;
	struct sockaddr_in   saddr;
	struct sockaddr_in   sin;

	struct sockaddr_in6  sin6;

	MYQTT_SOCKET        fd;
#if defined(AXL_OS_WIN32)
/*	BOOL                 unit      = axl_true; */
	int                  sin_size  = sizeof (sin);
	int                  sin_size6 = sizeof (sin6);
#else    	
	int                  unit      = 1; 
	socklen_t            sin_size  = sizeof (sin);
	socklen_t            sin_size6 = sizeof (sin6);
#endif	
	uint16_t             int_port;
	int                  backlog   = 0;
	int                  bind_res  = MYQTT_SOCKET_ERROR;
	int                  result;
#if defined(ENABLE_MYQTT_LOG)
	char               * str_out_buf[INET6_ADDRSTRLEN];
#endif
	struct addrinfo      req, *ans;
	int                  ret_val;

	v_return_val_if_fail (ctx,  -2);
	v_return_val_if_fail (host, -2);
	v_return_val_if_fail (port || strlen (port) == 0, -2);

	/* create socket */
	switch (transport) {
	case MYQTT_IPv6:
		/* resolve hostname */
		memset (&req, 0, sizeof(struct addrinfo));
		req.ai_flags    = AI_PASSIVE | AI_NUMERICHOST; 
		req.ai_family   = AF_INET6;
		req.ai_socktype = SOCK_STREAM;

		/* try to resolve */
		ret_val = getaddrinfo (host, port, &req, &ans);
		if (ret_val != 0) {
			myqtt_log (MYQTT_LEVEL_CRITICAL, "Unable to get hostname by calling getaddrinfo(%s:%s) = %d, errno=%d (%s)", 
				    host, port, ret_val, errno, 
				    gai_strerror (ret_val));
			axl_error_report (error, MyQttNameResolvFailure, "Unable to get hostname by calling getaddrinfo()");
			return -1;
		} /* end if */

		/* IPv6 */
		fd = socket (ans->ai_family, ans->ai_socktype, ans->ai_protocol);
		break;
	case MYQTT_IPv4:
		/* resolve old way */
		he = gethostbyname (host);
		if (he == NULL) {
			myqtt_log (MYQTT_LEVEL_CRITICAL, "Unable to get hostname by calling gethostbyname(%s), errno=%d", host, errno);
			axl_error_report (error, MyQttNameResolvFailure, "Unable to get hostname by calling gethostbyname()");
			return -1;
		} /* end if */
		
		haddr = ((struct in_addr *) (he->h_addr_list)[0]);

		/* IPv4 */
		fd = socket (PF_INET, SOCK_STREAM, 0);
		break;
	default:
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Received unsupported transport. Unable to create listener");
		axl_error_report (error, MyQttSocketCreationError, "Received unsupported transport. Unable to create listener");
		return -1;
	} /* end if */

	/* check socket created */
	if (fd <= 2) {
		/* do not allow creating sockets reusing stdin (0),
		   stdout (1), stderr (2) */
		myqtt_log (MYQTT_LEVEL_CRITICAL, "failed to create listener socket: %d (errno=%d:%s)", fd, errno, myqtt_errno_get_error (errno));
		axl_error_report (error, MyQttSocketCreationError, 
				  "failed to create listener socket: %d (errno=%d:%s)", fd, errno, myqtt_errno_get_error (errno));
		return -1;
	} /* end if */

#if defined(AXL_OS_WIN32)
	/* Do not issue a reuse addr which causes on windows to reuse
	 * the same address:port for the same process. Under linux,
	 * reusing the address means that consecutive process can
	 * reuse the address without being blocked by a wait
	 * state.  */
	/* setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char  *)&unit, sizeof(BOOL)); */
#else
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &unit, sizeof (unit));
#endif 

	/* get integer port */
	int_port  = (uint16_t) atoi (port);

	switch (transport) {
	case MYQTT_IPv6:
		/* call bind */
		bind_res = bind(fd, ans->ai_addr,  ans->ai_addrlen);

		/* release ans object */
		freeaddrinfo (ans); 
		break;
	case MYQTT_IPv4:
		memset (&saddr, 0, sizeof(struct sockaddr_in));
		saddr.sin_family          = AF_INET;
		saddr.sin_port            = htons(int_port);
		memcpy (&saddr.sin_addr, haddr, sizeof(struct in_addr));

		/* call bind */
		bind_res = bind(fd, (struct sockaddr *)&saddr,  sizeof (struct sockaddr_in));

		break;
	} /* end */

	/* call to bind */
	myqtt_log (MYQTT_LEVEL_DEBUG, "bind() call returned: %d, errno: %d (%s)", bind_res, errno, errno != 0 ? myqtt_errno_get_error (errno) : "");
	if (bind_res == MYQTT_SOCKET_ERROR) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "unable to bind address (port:%u already in use or insufficient permissions). Closing socket: %d", int_port, fd);
		axl_error_report (error, MyQttBindError, "unable to bind address (port:%u already in use or insufficient permissions). Closing socket: %d", int_port, fd);
		myqtt_close_socket (fd);
		return -1;
	}
	
	/* get current backlog configuration */
	myqtt_conf_get (ctx, MYQTT_LISTENER_BACKLOG, &backlog);
	
	if (listen (fd, backlog) == MYQTT_SOCKET_ERROR) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "listen() failed, errno: %d. Closing socket: %d", errno, fd);
		myqtt_close_socket (fd);
		axl_error_report (error, MyQttSocketCreationError, "an error have occur while executing listen");
		return -1;
        } /* end if */

	/* notify listener */
	result = -1;
	switch (transport) {
	case MYQTT_IPv4:
		result = getsockname (fd, (struct sockaddr *) &sin, &sin_size);
		break;
	case MYQTT_IPv6:
		result = getsockname (fd, (struct sockaddr *) &sin6, &sin_size6);
		break;
	}

	if (result < 0) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "getsockname () reported %d, errno %d", result, errno);
		myqtt_close_socket (fd);
		axl_error_report (error, MyQttNameResolvFailure, "an error have happen while executing getsockname");
		return -1;
	} /* end if */

	/* report and return fd */
	switch (transport) {
	case MYQTT_IPv4:
		myqtt_log  (MYQTT_LEVEL_DEBUG, "running listener at %s:%d (socket: %d)", 
			     inet_ntoa(sin.sin_addr), ntohs (sin.sin_port), fd);
		break;
	case MYQTT_IPv6:
		myqtt_log  (MYQTT_LEVEL_DEBUG, "running listener at %s:%d (socket: %d)", 
			     inet_ntop (sin6.sin6_family, sin6.sin6_addr.s6_addr, (void *) str_out_buf, INET6_ADDRSTRLEN), ntohs (sin6.sin6_port), fd);
		break;
	} /* end */
	return fd;
}


/** 
 * @brief Starts a generic TCP/IPv4 listener on the provided address and
 * port. This function is used internally by the myqtt listener
 * module to startup the myqtt listener TCP session associated,
 * however the function can be used directly to start TCP listeners.
 *
 * @param ctx The context where the listener is started.
 *
 * @param host Host address to allocate. It can be "127.0.0.1" to only
 * listen for localhost connections or "0.0.0.0" to listen on any
 * address that the server has installed. It cannot be NULL.
 *
 * @param port The port to listen on. It cannot be NULL and it must be
 * a non-zero string.
 *
 * @param error Optional axlError reference where a textual diagnostic
 * will be reported in case of error.
 *
 * @return The function returns the listener socket or -1 if it
 * fails. Optionally the axlError reports the textual especific error
 * found. If the function returns -2 then some parameter provided was
 * found to be NULL.
 */
MYQTT_SOCKET     myqtt_listener_sock_listen      (MyQttCtx   * ctx,
						    const char  * host,
						    const char  * port,
						    axlError   ** error)
{
	return myqtt_listener_sock_listen_common (ctx, host, port, error, MYQTT_IPv4);
}

/** 
 * @brief Starts a generic TCP/IPv6 listener on the provided address
 * and port. This function is used internally by the myqtt listener
 * module to startup the myqtt listener TCP session associated,
 * however the function can be used directly to start TCP listeners.
 *
 * @param ctx The context where the listener is started.
 *
 * @param host Host address to allocate. It can be "::1" (IPv4
 * equivalent of 127.0.0.1) to only listen for localhost connections
 * or "::" (IPv4 equivalent of 0.0.0.0) to listen on any address that
 * the server has installed. It cannot be NULL.
 *
 * @param port The port to listen on. It cannot be NULL and it must be
 * a non-zero string.
 *
 * @param error Optional axlError reference where a textual diagnostic
 * will be reported in case of error.
 *
 * @return The function returns the listener socket or -1 if it
 * fails. Optionally the axlError reports the textual especific error
 * found. If the function returns -2 then some parameter provided was
 * found to be NULL.
 */
MYQTT_SOCKET     myqtt_listener_sock_listen6      (MyQttCtx   * ctx,
						    const char  * host,
						    const char  * port,
						    axlError   ** error)
{
	return myqtt_listener_sock_listen_common (ctx, host, port, error, MYQTT_IPv6);
}

axlPointer __myqtt_listener_new (MyQttListenerData * data)
{
	char               * host          = data->host;
	axl_bool             threaded      = data->threaded;
	axl_bool             register_conn = data->register_conn;
	char               * str_port      = axl_strdup_printf ("%d", data->port);
	axlPointer           user_data     = data->user_data;
	const char         * message       = NULL;
	MyQttConn          * listener      = NULL;
	MyQttCtx           * ctx           = data->ctx;
	MyQttStatus          status        = MyQttOk;
	char               * host_used;
	axlError           * error         = NULL;
	MYQTT_SOCKET         fd;
	struct sockaddr_in   sin;
	MyQttNetTransport    transport     = data->transport;

	/* handlers received (may be both null) */
	MyQttListenerReady   on_ready       = data->on_ready;
	
	/* free data */
	axl_free (data);

	/* call to load local storage first (before an incoming connection) */
	myqtt_storage_load (ctx);

	/* allocate listener, try to guess IPv6 support */
	if (strstr (host, ":") || transport == MYQTT_IPv6) {
		myqtt_log (MYQTT_LEVEL_DEBUG, "Detected IPv6 listener: %s:%s..", host, str_port);
		fd = myqtt_listener_sock_listen6 (ctx, host, str_port, &error);
	} else
		fd = myqtt_listener_sock_listen (ctx, host, str_port, &error);

	if (fd == MYQTT_SOCKET_ERROR || fd == -1) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Failed to create listener socket, fd reported is an error %d. Unable to find on %s:%s. Error found: %s", fd,
			    host, str_port, axl_error_get (error));
		/* unref the host and port value */
		axl_free (str_port);
		axl_free (host);
		axl_error_free (error);
		return NULL;
	}
	
	/* listener ok */
	/* seems listener to be created, now create the MQTT connection around it */
	listener = myqtt_conn_new_empty (ctx, fd, MyQttRoleMasterListener);
	if (listener) {
		myqtt_log (MYQTT_LEVEL_DEBUG, "listener reference created (%p, id: %d, socket: %d)", listener, 
			    myqtt_conn_get_id (listener), fd);
	} else {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "myqtt_conn_new_empty() failed to create listener at %s:%s, closing socket: %d", 
			    host, str_port, fd);
		myqtt_close_socket (fd);
	} /* end if */

	/* unref the host and port value */
	axl_free (str_port);
	axl_free (host);

	/* handle returned socket or error */
	switch (fd) {
	case -2:
		__myqtt_conn_shutdown_and_record_error (
			listener, MyQttWrongReference, "Failed to start listener because myqtt_listener_sock_listener reported NULL parameter received");
		/* nullify reference */
		listener = NULL;
		break;
	case -1:
		__myqtt_conn_shutdown_and_record_error (
			listener, MyQttProtocolError, "Failed to start listener, myqtt_listener_sock_listener reported (code: %d): %s",
			axl_error_get_code (error), axl_error_get (error));
		/* nullify reference */
		listener = NULL;
		break;
	default:
		/* register the listener socket at the MyQtt Reader process.  */
		if (register_conn && listener)
			myqtt_reader_watch_listener (ctx, listener);
		if (threaded) {
			myqtt_log (MYQTT_LEVEL_DEBUG, "doing listener notification (threaded mode)");
			/* notify listener created */
			host_used = myqtt_support_inet_ntoa (ctx, &sin);
			if (on_ready != NULL) {
				on_ready (host_used, ntohs (sin.sin_port), MyQttOk, "server ready for requests", listener, user_data);
			} /* end if */

			axl_free (host_used);
		} /* end if */
		
		/* do not continue in the case of NULL reference */
		if (! listener)
			return NULL;

		/* the listener reference */
		myqtt_log (MYQTT_LEVEL_DEBUG, "returning listener running at %s:%s (non-threaded mode)", 
			    myqtt_conn_get_host (listener), myqtt_conn_get_port (listener));
		return listener;
	} /* end switch */

	/* according to the invocation */
	if (threaded) {
		/* notify error found to handlers */
		if (on_ready != NULL) 
			on_ready (NULL, 0, status, (char*) message, NULL, user_data);
	} else {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "unable to start myqtt server, error was: %s, unblocking myqtt_listener_wait",
		       message);
		/* notify the listener that an error was found
		 * (because the server didn't suply a handler) */
		myqtt_mutex_lock (&ctx->listener_unlock);
		QUEUE_PUSH (ctx->listener_wait_lock, INT_TO_PTR (axl_true));
		ctx->listener_wait_lock = NULL;
		myqtt_mutex_unlock (&ctx->listener_unlock);
	} /* end if */

	/* unref error */
	axl_error_free (error);

	/* return listener created */
	return listener;
}

/** 
 * @internal Implementation to support listener creation functions myqtt_listener_new*
 */
MyQttConn * __myqtt_listener_new_common  (MyQttCtx                * ctx,
					  const char              * host,
					  int                       port,
					  axl_bool                  register_conn,
					  MyQttConnOpts           * opts,
					  MyQttListenerReady        on_ready, 
					  MyQttNetTransport         transport,
					  axlPointer                user_data)
{
	MyQttListenerData * data;

	/* check context is initialized */
	if (! myqtt_init_check (ctx))
		return NULL;
	
	/* init listener module */
	myqtt_listener_init (ctx);
	
	/* prepare function data */
	data                = axl_new (MyQttListenerData, 1);
	data->host          = axl_strdup (host);
	data->port          = port;
	data->on_ready      = on_ready;
	data->user_data     = user_data;
	data->ctx           = ctx;
	data->register_conn = register_conn;
	data->threaded      = (on_ready != NULL);
	data->transport     = transport;
	
	/* make request */
	if (data->threaded) {
		myqtt_log (MYQTT_LEVEL_DEBUG, "invoking listener_new threaded mode");
		myqtt_thread_pool_new_task (ctx, (MyQttThreadFunc) __myqtt_listener_new, data);
		return NULL;
	}

	myqtt_log (MYQTT_LEVEL_DEBUG, "invoking listener_new non-threaded mode");
	return __myqtt_listener_new (data);	
}


/** 
 * @brief Allows to start a MQTT server on the provided local host
 * address and port.
 *
 * <b>Important note:</b> you must call to \ref myqtt_storage_set_path
 * to define the path first before creating any listener. This is
 * because creating a listener activates all server side code which
 * among other things includes the storage loading (client
 * subscriptions, offline publishing, etc). In the case direction,
 * once the storage path is loaded it cannot be changed after
 * restarting the particular context used in this operation (\ref MyQttCtx).
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
 */
MyQttConn * myqtt_listener_new (MyQttCtx             * ctx,
				const char           * host, 
				const char           * port, 
				MyQttConnOpts        * opts,
				MyQttListenerReady     on_ready, 
				axlPointer             user_data)
{
	/* call to int port API */
	return __myqtt_listener_new_common (ctx, host, __myqtt_listener_get_port (port), axl_true, opts, on_ready, MYQTT_IPv4, user_data);
}

/** 
 * @brief Creates a new TCP/IPv6 MyQtt Listener accepting incoming
 * connections on the given <b>host:port</b> configuration.
 *
 * Take a look to \ref myqtt_listener_new for additional
 * information. This functions provides same features plus IPv6
 * support.
 *
 * <b>Important note:</b> you must call to \ref myqtt_storage_set_path
 * to define the path first before creating any listener. This is
 * because creating a listener activates all server side code which
 * among other things includes the storage loading (client
 * subscriptions, offline publishing, etc). In the case direction,
 * once the storage path is loaded it cannot be changed after
 * restarting the particular context used in this operation (\ref MyQttCtx).
 *
 * @param ctx The context where the operation will be performed.
 *
 * @param host The host to listen on.
 *
 * @param port The port to listen on.
 *
 * @param opts Optional connection options to change default behviour.
 *
 * @param on_ready A optional callback to get a notification when
 * myqtt listener is ready to accept requests.
 *
 * @param user_data A user defined pointer to be passed in to
 * <i>on_ready</i> handler.
 *
 * @return The listener connection created (represented by a \ref
 * MyQttConn reference). You must use \ref
 * myqtt_conn_is_ok to check if the server was started.
 * 
 * See additional notes at \ref myqtt_listener_new
 */
MyQttConn * myqtt_listener_new6 (MyQttCtx             * ctx,
				 const char           * host, 
				 const char           * port, 
				 MyQttConnOpts        * opts,
				 MyQttListenerReady     on_ready, 
				 axlPointer             user_data)
{
	/* call to int port API */
	return __myqtt_listener_new_common (ctx, host, __myqtt_listener_get_port (port), axl_true, opts, on_ready, MYQTT_IPv6, user_data);
}

/** 
 * @brief Creates a new MyQtt Listener accepting incoming connections
 * on the given <b>host:port</b> configuration, receiving the port
 * configuration as an integer value.
 *
 * See \ref myqtt_listener_new for more information. 
 *
 * <b>Important note:</b> you must call to \ref myqtt_storage_set_path
 * to define the path first before creating any listener. This is
 * because creating a listener activates all server side code which
 * among other things includes the storage loading (client
 * subscriptions, offline publishing, etc). In the case direction,
 * once the storage path is loaded it cannot be changed after
 * restarting the particular context used in this operation (\ref MyQttCtx).
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @param host The host to listen to.
 *
 * @param port The port to listen to. Value defined for the port must be between 0 up to 65536.
 *
 * @param opts Optional connection options to modify behviour
 *
 * @param on_ready A optional notify callback to get when myqtt
 * listener is ready to perform replies.
 *
 * @param user_data A user defined pointer to be passed in to
 * <i>on_ready</i> handler.
 *
 * @return The listener connection created, or NULL if the optional
 * handler is provided (on_ready).
 *
 * <b>NOTE:</b> the reference returned is only owned by the myqtt
 * engine. This is not the case of \ref myqtt_conn_new where
 * the caller acquires automatically a reference to the connection (as
 * well as the myqtt engine).
 * 
 * In this case, if your intention is to keep a reference for later
 * operations, you must call to \ref myqtt_conn_ref to avoid
 * losing the reference if the system drops the connection. In the
 * same direction, you can't call to \ref myqtt_conn_close if
 * you don't own the reference returned by this function.
 * 
 * To close immediately a listener you can use \ref myqtt_conn_shutdown.
 */
MyQttConn * myqtt_listener_new2    (MyQttCtx             * ctx,
				    const char           * host,
				    int                    port,
				    MyQttConnOpts        * opts,
				    MyQttListenerReady     on_ready, 
				    axlPointer             user_data)
{

	/* call to common API */
	return __myqtt_listener_new_common (ctx, host, port, axl_true, opts, on_ready, MYQTT_IPv4, user_data);
}



/** 
 * @brief Blocks a listener (or listeners) launched until myqtt finish.
 * 
 * This function should be called after creating a listener (o
 * listeners) calling to \ref myqtt_listener_new to block current
 * thread.
 * 
 * This function can be avoided if the program structure can ensure
 * that the programm will not exist after calling \ref
 * myqtt_listener_new. This happens when the program is linked to (or
 * implements) and internal event loop.
 *
 * This function will be unblocked when the myqtt listener created
 * ends or a failure have occur while creating the listener. To force
 * an unlocking, a call to \ref myqtt_listener_unlock must be done.
 * 
 * @param ctx The context where the operation will be performed.
 */
void myqtt_listener_wait (MyQttCtx * ctx)
{
	MyQttAsyncQueue * temp;

	/* check reference received */
	if (ctx == NULL)
		return;

	/* check and init listener_wait_lock if it wasn't: init
	   lock */
	myqtt_mutex_lock (&ctx->listener_mutex);

	if (PTR_TO_INT (myqtt_ctx_get_data (ctx, "vo:listener:skip:wait"))) {
		/* seems someone called to unlock before we get
		 * here */
		/* unlock */
		myqtt_mutex_unlock (&ctx->listener_mutex);
		return;
	} /* end if */
	
	/* create listener locker */
	if (ctx->listener_wait_lock == NULL) 
		ctx->listener_wait_lock = myqtt_async_queue_new ();

	/* unlock */
	myqtt_mutex_unlock (&ctx->listener_mutex);

	/* double locking to ensure waiting */
	myqtt_log (MYQTT_LEVEL_DEBUG, "Locking listener");
	if (ctx->listener_wait_lock != NULL) {
		/* get a local reference to the queue and work with it */
		temp = ctx->listener_wait_lock;

		/* get blocked until the waiting lock is released */
		myqtt_async_queue_pop   (temp);
		
		/* unref the queue */
		myqtt_async_queue_unref (temp);
	}
	myqtt_log (MYQTT_LEVEL_DEBUG, "(un)Locked listener");

	return;
}

/** 
 * @brief Unlock the thread blocked at the \ref myqtt_listener_wait.
 * 
 * @param ctx The context where the operation will be performed.
 **/
void myqtt_listener_unlock (MyQttCtx * ctx)
{
	/* check reference received */
	if (ctx == NULL || myqtt_ctx_ref_count (ctx) < 1)
		return;

	/* unlock listener */
	myqtt_mutex_lock (&ctx->listener_unlock);
	if (ctx->listener_wait_lock != NULL) {

		/* push to signal listener unblocking */
		myqtt_log (MYQTT_LEVEL_DEBUG, "(un)Locking listener..");

		/* notify waiters */
		if (myqtt_async_queue_waiters (ctx->listener_wait_lock) > 0) {
			QUEUE_PUSH (ctx->listener_wait_lock, INT_TO_PTR (axl_true));
		} else {
			/* unref */
			myqtt_async_queue_unref (ctx->listener_wait_lock);
		} /* end if */

		/* nullify */
		ctx->listener_wait_lock = NULL;

		myqtt_mutex_unlock (&ctx->listener_unlock);
		return;
	} else {
		/* flag this context to unlock myqtt_listener_wait
		 * caller because he still didn't reached */
		myqtt_log (MYQTT_LEVEL_DEBUG, "myqtt_listener_wait was not called, signalling to do fast unlock");
		myqtt_ctx_set_data (ctx, "vo:listener:skip:wait", INT_TO_PTR (axl_true));
	}

	myqtt_log (MYQTT_LEVEL_DEBUG, "(un)Locking listener: already unlocked..");
	myqtt_mutex_unlock (&ctx->listener_unlock);
	return;
}

/** 
 * @internal
 * 
 * Internal myqtt function. This function allows
 * __myqtt_listener_new_common to initialize myqtt listener module
 * only if a listener is installed.
 **/
void myqtt_listener_init (MyQttCtx * ctx)
{
	/* do not lock if the data is already initialized */
	if (ctx != NULL && ctx->listener_wait_lock != NULL &&
	    ctx->listener_on_accept_handlers != NULL)
		return;

	/* init lock */
	myqtt_mutex_lock (&ctx->listener_mutex);
	
	/* init the server on accept connection list */
	if (ctx->listener_on_accept_handlers == NULL)
		ctx->listener_on_accept_handlers = axl_list_new (axl_list_always_return_1, axl_free);

	/* unlock */
	myqtt_mutex_unlock (&ctx->listener_mutex);
	return;
}

/** 
 * @internal Allows to cleanup the myqtt listener state.
 * 
 * @param ctx The myqtt context to cleanup.
 */
void myqtt_listener_cleanup (MyQttCtx * ctx)
{
	MyQttAsyncQueue * queue;
	v_return_if_fail (ctx);

	axl_list_free (ctx->listener_on_accept_handlers);
	ctx->listener_on_accept_handlers = NULL;
	
	axl_free (ctx->listener_default_realm);
	ctx->listener_default_realm = NULL;

	/* acquire queue and nullify */
	queue = ctx->listener_wait_lock;
	ctx->listener_wait_lock = NULL;
	myqtt_log (MYQTT_LEVEL_DEBUG, "listener wait queue ref: %p", queue);
	if (queue) {
		/* remove pending items from the queue */
		while (myqtt_async_queue_items (queue) > 0)
			myqtt_async_queue_pop (queue);
		/* unref the queue */
		myqtt_async_queue_unref (queue);
	}


	return;
}

/** 
 * @brief Allows to configure a handler that is executed once a
 * connection have been accepted.
 *
 * The handler to be configured could be used as a way to get
 * notifications about connections created, but also as a filter for
 * connections that must be dropped.
 *
 * @param ctx The context where the operation will be performed. 
 *
 * @param on_accepted The handler to be executed.
 *
 * @param _data User space data to be passed in to the handler
 * executed.
 *
 * Note this handler is called before any socket exchange to allow
 * denying as soon as possible. Though the handler receives a
 * reference to the \ref MyQttConn to be accepted/denied, it is only
 * provided to allow storing or reconfiguring the connection. 
 * 
 * In other words, when the handler is called, the MQTT session is
 * still not established. If you need to execute custom operations
 * once the connection is fully registered with the MQTT session
 * established, see \ref myqtt_conn_set_connection_actions with
 * \ref CONNECTION_STAGE_POST_CREATED.
 *
 * This function supports setting up several handlers which will be
 * called in the order they were configured. The function is thread
 * safe.
 *
 */
void          myqtt_listener_set_on_connection_accepted (MyQttCtx                  * ctx,
							 MyQttOnAcceptedConnection   on_accepted, 
							 axlPointer                   _data)
{
	MyQttListenerOnAcceptData * data;

	/* check reference received */
	if (ctx == NULL || on_accepted == NULL)
		return;

	/* init lock */
	myqtt_mutex_lock (&ctx->listener_mutex);

	/* store handler configuration */
	data                 = axl_new (MyQttListenerOnAcceptData, 1);
	if (data == NULL) {
		/* memory allocation failed. */
		myqtt_mutex_unlock (&ctx->listener_mutex);
		return;
	}
	data->on_accept      = on_accepted;
	data->on_accept_data = _data;

	/* init the list if it wasn't */
	if (ctx->listener_on_accept_handlers == NULL) {
		/* alloc list */
		ctx->listener_on_accept_handlers = axl_list_new (axl_list_always_return_1, axl_free);

		/* check allocated list */
		if (ctx->listener_on_accept_handlers == NULL) {
			/* memory allocation failed. */
			myqtt_mutex_unlock (&ctx->listener_mutex);
			return;
		} /* end if */
	}

	/* add the item */
	axl_list_add (ctx->listener_on_accept_handlers, data);

	myqtt_log (MYQTT_LEVEL_DEBUG, "received new handler: %p with data %p, list: %d", 
		    on_accepted, data, axl_list_length (ctx->listener_on_accept_handlers));

	/* unlock */
	myqtt_mutex_unlock (&ctx->listener_mutex);

	/* nothing more */
	return;
}

/** 
 * @internal type definition used to hold port share handlers.
 */
typedef struct _MyQttPortShareData {
	char                    * local_addr;
	char                    * local_port;
	MyQttPortShareHandler    handler;
	axlPointer                user_data;
} MyQttPortShareData;

void __myqtt_listener_release_port_share_data (axlPointer _ptr)
{
	MyQttPortShareData * data = _ptr;
	axl_free (data->local_port);
	axl_free (data->local_addr);
	axl_free (data);
	return;
}

/** 
 * @brief Allows to install a port share handler that will be called
 * to detect and activate alternative transports that must be enabled
 * before activating normal MQTT session.
 *
 * @param ctx The context the operation will take place. Handlers
 * installed on this context will not affect to other running contexts
 * on the same process.
 *
 * @param local_addr Reference to the local address this handler must
 * be limited. Pass in NULL in the case you don't want any filtering
 * (that is, to avoid calling this handler if local address value
 * doesn't match).
 *
 * @param local_port Reference to the local port this handler must be
 * limited. Pass in NULL in the case you don't want any filtering
 * (that is, to avoid calling this handler if local port value doesn't
 * match).
 *
 * @param handler The handler that will be called to detect the transport. 
 *
 * @param user_data User defined pointer that will be passed in into
 * the handler when called.
 *
 * @return A handle that represent the installed port sharing handler
 * or NULL if it fail. The value returned can be used to remove this
 * handler later.
 *
 */
axlPointer          myqtt_listener_set_port_sharing_handling (MyQttCtx                * ctx, 
							      const char              * local_addr,
							      const char              * local_port, 
							      MyQttPortShareHandler     handler,
							      axlPointer                user_data)
{
	MyQttPortShareData * data = NULL;

	v_return_val_if_fail (ctx && handler, NULL);

	data = axl_new (MyQttPortShareData, 1);
	if (data == NULL)
		return NULL;

	/* record local address and local port published */
	if (local_addr)
		data->local_addr = axl_strdup (local_addr);
	if (local_port)
		data->local_port = axl_strdup (local_port);

	/* store the handler and user data associated to the handler */
	data->handler   = handler;
	data->user_data = user_data;

	/* lock during operation */
	myqtt_mutex_lock (&ctx->port_share_mutex);

	if (ctx->port_share_handlers == NULL)
		ctx->port_share_handlers = axl_list_new (axl_list_equal_ptr, __myqtt_listener_release_port_share_data);

	/* add configuration */
	axl_list_append (ctx->port_share_handlers, data);

	/* unlock */
	myqtt_mutex_unlock (&ctx->port_share_mutex);
	
	return data;
}


/** 
 * @internal Function used to shutdown connections associated to a
 * listener.
 */
void __myqtt_listener_shutdown_foreach (MyQttConn * conn,
					 axlPointer         user_data)
{
	/* get the listener id associated to the connection */
	int         listener_id;
#if defined(ENABLE_MYQTT_LOG)
	MyQttCtx * ctx;
#endif

	/* check the role (if it is a listener, skip) */
	if (myqtt_conn_get_role (conn) == MyQttRoleMasterListener)
		return;

#if defined(ENABLE_MYQTT_LOG)
	/* get the listener ctx */
	ctx = myqtt_conn_get_ctx (conn);
#endif

	/* get the listener id associated to the connection */
	listener_id = myqtt_conn_get_id (myqtt_conn_get_listener (conn));

	/* check connection */
	myqtt_log (MYQTT_LEVEL_DEBUG, "checking connection to shutdown: %d == %d", 
		    listener_id, PTR_TO_INT (user_data));
	if (listener_id == PTR_TO_INT (user_data)) { 
		myqtt_log (MYQTT_LEVEL_DEBUG, "shutdown connection: %d..",
			    myqtt_conn_get_id (conn));
		__myqtt_conn_shutdown_and_record_error (conn, MyQttOk, "Shutting down connection due to listener close");
	}
	return;
}

/** 
 * @brief Allows to shutdown the listener provided and all connections
 * that were created due to its function.
 *
 * The function perform a shutdown (no MQTT close session phase) on
 * the listener and connections created.
 * 
 * @param listener The listener to shutdown.
 *
 * @param also_created_conns axl_true to shutdown all connections 
 */
void          myqtt_listener_shutdown (MyQttConn * listener,
					axl_bool   also_created_conns)
{
	MyQttCtx        * ctx;
	MyQttAsyncQueue * notify = NULL;

	/* check parameters */
	if (! myqtt_conn_is_ok (listener, axl_false))
		return;
	
	/* get ctx */
	ctx = myqtt_conn_get_ctx (listener);

	myqtt_log (MYQTT_LEVEL_DEBUG, "shutting down listener..");
	
	/* ref the listener during the operation */
	myqtt_conn_ref (listener, "listener-shutdown");

	/* now do a foreach over all connections registered in the
	 * reader */
	if (also_created_conns) {
		/* call to shutdown all associated connections */
		notify = myqtt_reader_foreach (ctx, 
						__myqtt_listener_shutdown_foreach, 
						INT_TO_PTR (myqtt_conn_get_id (listener)));
		/* wait to finish */
		myqtt_async_queue_pop (notify);
		myqtt_async_queue_unref (notify);
	} /* end if */

	/* shutdown the listener */
	__myqtt_conn_shutdown_and_record_error (listener, MyQttOk, "listener shutted down");

	/* unref the listener now finished */
	myqtt_conn_unref (listener, "listener-shutdown");

	return;
}

/** 
 * @internal PORT SHARING: Function used to detect previous transports
 * that must be activated other the provided connection before doing
 * any read operation. 
 *
 * The function tries to locate port sharing handlers that may detect
 * transports that should be negotiated first before continue to later
 * let the connection to work as usual.
 *
 * If the function doesn't find anything, it just return axl_true
 * ("connection ready to use").
 */
axl_bool __myqtt_listener_check_port_sharing (MyQttCtx * ctx, MyQttConn * connection)
{
	char                  buffer[5];
	int                   position = 0;
	int                   result;
	MyQttPortShareData * data;
	MyQttConn    * listener;

	myqtt_log (MYQTT_LEVEL_DEBUG, "Checking port sharing support for conn-id=%d (role %d == %d ?, handlers %p ?, already tp detected: %d ?",
		    connection->id, connection->role, MyQttRoleListener, ctx->port_share_handlers, connection->transport_detected);

	/* check if the connection is a listener connection to avoid
	 * running all this code. Transport detection is only for
	 * listener peers */
	if (connection->role != MyQttRoleListener)
		return axl_true; /* nothing to detect here */

	/* check if we have some handler installed */
	if (ctx->port_share_handlers == NULL)
		return axl_true; /* nothing to detect here, no handlers found */

	/* check if transport is already detected to avoid installing
	 * this */
	if (connection->transport_detected)
		return axl_true; /* nothing to detect here */

	/* call to prepare this port */
#if defined(AXL_OS_WIN32)
	if (recv (connection->session, buffer, 4, MSG_PEEK) != 4) 
#else
	if (recv (connection->session, buffer, 4, MSG_PEEK | MSG_DONTWAIT) != 4) 
#endif
		return axl_true; /* nothing found, do not activate this */
	buffer[4] = 0;

	myqtt_log (MYQTT_LEVEL_DEBUG, "Transport detection for conn-id=%d, content detected: '%d %d %d %d'", 
		    myqtt_conn_get_id (connection),
		    buffer[0], buffer[1], buffer[2], buffer[3]);
	if (axl_memcmp (buffer, "RPY", 3)) {
		/* detected MQTT transport, finishing detection here */
		connection->transport_detected = axl_true;
		return axl_true;
	} /* end if */

	/* ok, get connection listener */
	listener = myqtt_conn_get_listener (connection);

	/* lock */
	myqtt_mutex_lock (&ctx->port_share_mutex);

	/* for each position call the call back */
	while (position < axl_list_length (ctx->port_share_handlers)) {

		/* get data handler */
		data = axl_list_get_nth (ctx->port_share_handlers, position);

		/* call handler */
		result = data->handler (ctx, listener, connection, connection->session, buffer, data->user_data);
		if (result == 2) {
			/* ok, transport found */
			connection->transport_detected = axl_true;
			break;
		} else if (result == -1) {
			myqtt_log (MYQTT_LEVEL_CRITICAL, "Found connection-id=%d with a port share handler that failed, closing connection", 
				    myqtt_conn_get_id (connection));
			myqtt_conn_shutdown (connection);
			break;
		} /* end if */
		
		/* next position */
		position++;
	}

	/* unlock */
	myqtt_mutex_unlock (&ctx->port_share_mutex);


	/* do nothing */
	return axl_true;
}


/* @} */
