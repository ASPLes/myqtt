/* 
 *  MyQtt: A high performance open source MQTT implementation
 *  Copyright (C) 2016 Advanced Software Production Line, S.L.
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

/* global include */
#include <myqtt.h>

/* private include */
#include <myqtt-ctx-private.h>
#include <myqtt-hash-private.h>
#include <myqtt-msg-private.h>

/* include connection internal definition */
#include <myqtt-conn-private.h>
#include <myqtt-addrinfo.h>

#if defined(AXL_OS_UNIX)
# include <netinet/tcp.h>
#endif

#define LOG_DOMAIN "myqtt-connection"

MyQttNetTransport __myqtt_conn_detect_transport (MyQttCtx * ctx, const char * host)
{
	/* if something os not configured, report IPv4 support */
	if (ctx == NULL || host == NULL) {
		return MYQTT_IPv4;
	} /* end if */

	if (strstr (host, ":")) {
		/* IPv6 resolution */
		return MYQTT_IPv6;
	} /* end if */

	/* IPv4 resolution (default with for IPv4 address too) */
	return MYQTT_IPv4;
}



/**
 * @internal Function to init all mutex associated to this particular
 * connection 
 */
void __myqtt_conn_init_mutex (MyQttConn * connection)
{
	/* inits all mutex associated to the connection provided. */
	myqtt_mutex_create (&connection->ref_mutex);
	myqtt_mutex_create (&connection->op_mutex);
	myqtt_mutex_create (&connection->handlers_mutex);
	myqtt_mutex_create (&connection->pending_errors_mutex);
	return;
}


/** 
 * @internal Function that perform the connection socket sanity check.
 * 
 * This prevents from having problems while using socket descriptors
 * which could conflict with reserved file descriptors such as 0,1,2..
 *
 * @param ctx The context where the operation will be performed. 
 *
 * @param connection The connection to check.
 * 
 * @return axl_true if the socket sanity check have passed, otherwise
 * axl_false is returned.
 */
axl_bool      myqtt_conn_do_sanity_check (MyQttCtx * ctx, MYQTT_SOCKET session)
{
	/* warn the user if it is used a socket descriptor that could
	 * be used */
	if (ctx && ctx->connection_enable_sanity_check) {
		
		if (session < 0) {
			myqtt_log (MYQTT_LEVEL_CRITICAL,
				    "Socket receive is not working, invalid socket descriptor=%d", session);
			return axl_false;
		} /* end if */

		/* check for a valid socket descriptor. */
		switch (session) {
		case 0:
		case 1:
		case 2:
			myqtt_log (MYQTT_LEVEL_CRITICAL, 
			       "created socket descriptor using a reserved socket descriptor (%d), this is likely to cause troubles",
			       session);
			/* return sanity check have failed. */
			return axl_false;
		}
	}

	/* return the sanity check is ok. */
	return axl_true;
}
/** 
 * @internal
 * @brief Default handler used to send data.
 * 
 * See \ref myqtt_conn_set_send_handler.
 */
int  myqtt_conn_default_send (MyQttConn           * connection,
			      const unsigned char * buffer,
			      int                   buffer_len)
{
	/* send the message */
	return send (connection->session, buffer, buffer_len, 0);
}

/** 
 * @internal
 * @brief Default handler to be used while receiving data
 *
 * See \ref myqtt_conn_set_receive_handler.
 */
int  myqtt_conn_default_receive (MyQttConn          * connection,
				 unsigned char      * buffer,
				 int                  buffer_len)
{
	/* receive content */
	return recv (connection->session, buffer, buffer_len, 0);
}

/** 
 * @internal Support function for connection identifications.
 *
 * This is used to generate and return the next connection identifier.
 *
 * @param ctx The context where the operation will be performed.
 *
 * @return Next connection identifier available.
 */
int  __myqtt_conn_get_next_id (MyQttCtx * ctx)
{
	/* get current context */
	int         result;

	/* lock */
	myqtt_mutex_lock (&ctx->connection_id_mutex);
	
	/* increase */
	result = ctx->connection_id;
	ctx->connection_id++;

	/* unlock */
	myqtt_mutex_unlock (&ctx->connection_id_mutex);

	return result;
}


typedef struct _MyQttConnNewData {
	MyQttConn            * connection;
	MyQttConnOpts        * opts;
	MyQttConnNew           on_connected;
	axlPointer             user_data;
	axl_bool               threaded;
}MyQttConnNewData;


typedef struct _MyQttConnOnCloseData {
	MyQttConnOnClose handler;
	axlPointer       data;
} MyQttConnOnCloseData;


/**
 * \defgroup myqtt_conn MyQtt Conn: Related function to create and manage MyQtt Conns.
 */

/**
 * \addtogroup myqtt_conn
 * @{
 */

/** 
 * @brief Allows to create a new \ref MyQttConn from a socket
 * that is already connected.
 *
 * This function only takes the given underlying transport descriptor
 * (that is, the socket) and gets the remote peer name in order to set
 * which is the remote host peer.
 *
 * This function also sets the default handler for sending and
 * receiving data using the send and recv operations. If you are
 * developing a MQTT solution which needs to notify the way data is actually
 * sent or received you should change that behaviour after calling
 * this function. This is done by using:
 *
 *  - \ref myqtt_conn_set_send_handler
 *  - \ref myqtt_conn_set_receive_handler
 *
 * @param ctx     The context where the operation will be performed.
 * @param socket  An already connected socket.  
 * @param role    The role to be set to the connection being created.
 * 
 * @return a newly allocated \ref MyQttConn. 
 */
MyQttConn * myqtt_conn_new_empty            (MyQttCtx *    ctx, 
					     MYQTT_SOCKET  socket, 
					     MyQttPeerRole role)
{
	/* creates a new connection */
	return myqtt_conn_new_empty_from_conn (ctx, socket, NULL, role);
}

/** 
 * @internal
 *
 * Internal function used to create new connection starting from a
 * socket, and optional from internal data stored on the provided
 * connection. It is supposed that the socket provided belongs to the
 * connection provided.
 *
 * The function tries to creates a new connection, using the socket
 * provided. The function also keeps all internal connection that for
 * the new connection creates extracting that data from the connection
 * reference provided. This is useful while performing tuning
 * operations that requires to reset the connection but retain data
 * stored on the connection.
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @param socket The socket to be used for the new connection.
 *
 * @param connection The connection where the user space data will be
 * extracted. This reference is optional.
 *
 * @param role The connection role to be set to this function.
 * 
 * @return A newly created connection, using the provided data, that
 * must be de-allocated using \ref myqtt_conn_close.
 */
MyQttConn * myqtt_conn_new_empty_from_conn (MyQttCtx        * ctx,
					    MYQTT_SOCKET      socket,
					    MyQttConn       * __connection,
					    MyQttPeerRole     role)
{
	MyQttConn   * connection;

	/* create connection object without setting socket (this is
	 * done by myqtt_conn_set_sock) */
	connection                     = axl_new (MyQttConn, 1);
	MYQTT_CHECK_REF (connection, NULL);

	/* for now, set default connection error */
	connection->last_err           = MYQTT_CONNACK_UNKNOWN_ERR;
	connection->ctx                = ctx;
	myqtt_ctx_ref2 (ctx, "new connection"); /* acquire a reference to context */
	connection->id                 = __myqtt_conn_get_next_id (ctx);

	connection->is_connected       = axl_true;
	connection->ref_count          = 1;

	/* wait reply hash */
	connection->wait_replies       = axl_hash_new (axl_hash_int, axl_hash_equal_int);
	connection->peer_wait_replies  = axl_hash_new (axl_hash_int, axl_hash_equal_int);

	/* subscriptions **/
	connection->subs               = axl_hash_new (axl_hash_string, axl_hash_equal_string);
	connection->wild_subs          = axl_hash_new (axl_hash_string, axl_hash_equal_string);

	/* call to init all mutex associated to this particular connection */
	__myqtt_conn_init_mutex (connection);

	/* check connection that is accepting connections */
	if (role != MyQttRoleMasterListener) {

		/* creates the user space data */
		if (__connection != NULL) {
			/* transfer hash used by previous connection into the new one */
			connection->data       = __connection->data;
			/* creates a new hash to keep the connection internal state consistent */
			__connection->data     = myqtt_hash_new_full (axl_hash_string, axl_hash_equal_string,
								       NULL,
								       NULL);

			/* remove being closed flag if found */
			myqtt_conn_set_data (connection, "being_closed", NULL);
		} else 
			connection->data       = myqtt_hash_new_full (axl_hash_string, axl_hash_equal_string,
								       NULL, 
								       NULL);
		
		/* set default send and receive handlers */
		connection->send               = myqtt_conn_default_send;
		connection->receive            = myqtt_conn_default_receive;

	} else {
		/* create the hash data table for master listener connections */
		connection->data       = myqtt_hash_new_full (axl_hash_string, axl_hash_equal_string,
							       NULL, 
							       NULL);
		
	} /* end if */
	
	/* set by default to close the underlying connection when the
	 * connection is closed */
	connection->close_session      = axl_true;

	/* establish the connection role  */
	connection->role               = role;

	/* set socket provided (do not allow stdin(0), stdout(1), stderr(2) */
	if (socket > 2) {
		if (! myqtt_conn_set_socket (connection, socket, NULL, NULL)) {
			myqtt_log (MYQTT_LEVEL_CRITICAL, "failed to configure socket associated to connection");
			myqtt_conn_unref (connection, "myqtt_conn_new_empty_from_connection");
			return NULL;
		} /* end if */

		/* set accepted */
		connection->last_err           = MYQTT_CONNACK_ACCEPTED;

	} else {
		/* set a wrong socket connection in the case a not
		   proper value is received */
		myqtt_log (MYQTT_LEVEL_WARNING, "received wrong socket fd, setting invalid fd beacon: -1");
		connection->session = -1;
	} /* end if */

	return connection;	
}

/** 
 * @brief Allows to configure the socket to be used by the provided
 * connection. This function is usually used in conjunction with \ref
 * myqtt_conn_new_empty.
 *
 * @param conn The connection to be configured with the socket
 * provided.
 *
 * @param _socket The socket connection to configure.
 *
 * @param real_host Optional reference that can configure the host
 * value associated to the socket provided. This is useful on
 * environments were a proxy or TUNNEL is used. In such environments
 * the host and port value get from system calls return the middle hop
 * but not the destination host. You can safely pass NULL, causing the
 * function to figure out which is the right value using the socket
 * provided.
 *
 * @param real_port Optional reference that can configure the port
 * value associated to the socket provided. See <b>real_host</b> param
 * for more information. In real_host is defined, it is required to
 * define this parameter.
 *
 * @return axl_true in the case the function has configured the
 * provided socket, otherwise axl_false is returned.
 */
axl_bool            myqtt_conn_set_socket                (MyQttConn * conn,
								 MYQTT_SOCKET      _socket,
								 const char       * real_host,
								 const char       * real_port)
{

	struct sockaddr_storage   sin;
#if defined(AXL_OS_WIN32)
	/* windows flavors */
	int                  sin_size = sizeof (sin);
#else
	/* unix flavors */
	socklen_t            sin_size = sizeof (sin);
#endif
	MyQttCtx          * ctx;
	char                 host_name[NI_MAXHOST];
	char                 srv_name[NI_MAXSERV]; 

	/* check conn reference */
	if (conn == NULL)
		return axl_false;

	ctx  = CONN_CTX(conn);

	/* perform connection sanity check */
	if (!myqtt_conn_do_sanity_check (ctx, _socket)) 
		return axl_false;

	/* disable nagle */
	myqtt_conn_set_sock_tcp_nodelay (_socket, axl_true);

	/* set socket */
	conn->session = _socket;
	
	/* get remote peer name */
	if (real_host && real_port) {
		/* set host and port from user values */
		conn->host = axl_strdup (real_host);
		conn->port = axl_strdup (real_port);
	} else {
		/* clear structures */
		memset (host_name, 0, NI_MAXHOST);
		memset (srv_name, 0, NI_MAXSERV);
		if (conn->role == MyQttRoleMasterListener) {
			if (getsockname (_socket, (struct sockaddr *) &sin, &sin_size) < 0) {
				myqtt_log (MYQTT_LEVEL_CRITICAL, "unable to get local hostname and port from socket=%d, errno=%d (%s)", 
					    _socket, errno, myqtt_errno_get_error (errno));
				return axl_false;
			} /* end if */
		} else {
			if (getpeername (_socket, (struct sockaddr *) &sin, &sin_size) < 0) {
				myqtt_log (MYQTT_LEVEL_CRITICAL, "unable to get remote hostname and port from socket=%d (errno=%d)", 
					    _socket, errno);
				return axl_false;
			} /* end if */
		} /* end if */

		/* set host and port from socket recevied */
		if (getnameinfo ((struct sockaddr *) &sin, sin_size, host_name, NI_MAXHOST, srv_name, NI_MAXSERV, NI_NUMERICSERV | NI_NUMERICHOST) != 0) {
			myqtt_log (MYQTT_LEVEL_CRITICAL, "getnameinfo () call failed, error was errno=%d", errno);
			return axl_false;
		} /* end if */

		/* copy values */
		conn->host = axl_strdup (host_name);
		conn->port = axl_strdup (srv_name);
	} /* end if */

	/* now set local address */
	if (getsockname (_socket, (struct sockaddr *) &sin, &sin_size) < 0) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "unable to get local hostname and port to resolve local address from socket=%d", _socket);
		return axl_false;
	} /* end if */

	/* set host and port from socket recevied */
	if (getnameinfo ((struct sockaddr *) &sin, sin_size, host_name, NI_MAXHOST, srv_name, NI_MAXSERV, NI_NUMERICSERV | NI_NUMERICHOST) != 0) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "getnameinfo () call failed, error was errno=%d", errno);
		return axl_false;
	}

	/* set local addr and local port */
	conn->local_addr = axl_strdup (host_name);
	conn->local_port = axl_strdup (srv_name);

	return axl_true;
}


/** 
 * \brief Allows to change connection semantic to blocking.
 *
 * This function should not be useful for MyQtt Library consumers
 * because the internal MyQtt Implementation requires connections to
 * be non-blocking semantic.
 * 
 * @param connection the connection to set as blocking
 * 
 * @return axl_true if blocking state was set or axl_false if not.
 */
axl_bool      myqtt_conn_set_blocking_socket (MyQttConn    * connection)
{
#if defined(ENABLE_MYQTT_LOG)
	MyQttCtx * ctx;
#endif
#if defined(AXL_OS_UNIX)
	int  flags;
#endif
	/* check the connection reference */
	if (connection == NULL)
		return axl_false;

#if defined(ENABLE_MYQTT_LOG)
	/* get a reference to the ctx */
	ctx = connection->ctx;
#endif
	
#if defined(AXL_OS_WIN32)
	if (!myqtt_win32_blocking_enable (connection->session)) {
		__myqtt_conn_shutdown_and_record_error (
			connection, MyQttError, "unable to set blocking I/O");
		return axl_false;
	}
#else
	if ((flags = fcntl (connection->session, F_GETFL, 0)) < 0) {
		__myqtt_conn_shutdown_and_record_error (
			connection, MyQttError,
			"unable to get socket flags to set non-blocking I/O");
		return axl_false;
	}
	myqtt_log (MYQTT_LEVEL_DEBUG, "actual flags state before setting blocking: %d", flags);
	flags &= ~O_NONBLOCK;
	if (fcntl (connection->session, F_SETFL, flags) < 0) {
		__myqtt_conn_shutdown_and_record_error (
			connection, MyQttError, "unable to set non-blocking I/O");
		return axl_false;
	}
	myqtt_log (MYQTT_LEVEL_DEBUG, "actual flags state after setting blocking: %d", flags);
#endif
	myqtt_log (MYQTT_LEVEL_DEBUG, "setting connection as blocking");
	return axl_true;
}

/** 
 * \brief Allows to change connection semantic to non-blocking.
 *
 * Sets a connection to be non-blocking while sending and receiving
 * data. This function should not be useful for MyQtt Library
 * consumers.
 * 
 * @param connection the connection to set as non-blocking.
 * 
 * @return axl_true if non-blocking state was set or axl_false if not.
 */
axl_bool      myqtt_conn_set_nonblocking_socket (MyQttConn * connection)
{
#if defined(ENABLE_MYQTT_LOG)
	MyQttCtx * ctx;
#endif

#if defined(AXL_OS_UNIX)
	int  flags;
#endif
	/* check the reference */
	if (connection == NULL)
		return axl_false;

#if defined(ENABLE_MYQTT_LOG)
	/* get a reference to context */
	ctx = connection->ctx;
#endif
	
#if defined(AXL_OS_WIN32)
	if (!myqtt_win32_nonblocking_enable (connection->session)) {
		__myqtt_conn_shutdown_and_record_error (
			connection, MyQttError, "unable to set non-blocking I/O");
		return axl_false;
	}
#else
	if ((flags = fcntl (connection->session, F_GETFL, 0)) < 0) {
		__myqtt_conn_shutdown_and_record_error (
			connection, MyQttError,
			"unable to get socket flags to set non-blocking I/O");
		return axl_false;
	}

	myqtt_log (MYQTT_LEVEL_DEBUG, "actual flags state before setting non-blocking: %d", flags);
	flags |= O_NONBLOCK;
	if (fcntl (connection->session, F_SETFL, flags) < 0) {
		__myqtt_conn_shutdown_and_record_error (
			connection, MyQttError, "unable to set non-blocking I/O");
		return axl_false;
	}
	myqtt_log (MYQTT_LEVEL_DEBUG, "actual flags state after setting non-blocking: %d", flags);
#endif
	myqtt_log (MYQTT_LEVEL_DEBUG, "setting connection as non-blocking");
	return axl_true;
}

/** 
 * @brief Allows to configure tcp no delay flag (enable/disable Nagle
 * algorithm).
 * 
 * @param socket The socket to be configured.
 *
 * @param enable The value to be configured, axl_true to enable tcp no
 * delay.
 * 
 * @return axl_true if the operation is completed.
 */
axl_bool                 myqtt_conn_set_sock_tcp_nodelay   (MYQTT_SOCKET socket,
								   axl_bool      enable)
{
	/* local variables */
	int result;

#if defined(AXL_OS_WIN32)
	BOOL   flag = enable ? TRUE : FALSE;
	result      = setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, (char  *)&flag, sizeof(BOOL));
#else
	int    flag = enable;
	result      = setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof (flag));
#endif
	if (result < 0) {
		return axl_false;
	}

	/* properly configured */
	return axl_true;
} /* end */

/** 
 * @brief Allows to enable/disable non-blocking/blocking behaviour on
 * the provided socket.
 * 
 * @param socket The socket to be configured.
 *
 * @param enable axl_true to enable blocking I/O, otherwise use
 * axl_false to enable non blocking I/O.
 * 
 * @return axl_true if the operation was properly done, otherwise axl_false is
 * returned.
 */
axl_bool                 myqtt_conn_set_sock_block         (MYQTT_SOCKET socket,
								   axl_bool      enable)
{
#if defined(AXL_OS_UNIX)
	int  flags;
#endif

	if (enable) {
		/* enable blocking mode */
#if defined(AXL_OS_WIN32)
		if (!myqtt_win32_blocking_enable (socket)) {
			return axl_false;
		}
#else
		if ((flags = fcntl (socket, F_GETFL, 0)) < 0) {
			return axl_false;
		} /* end if */

		flags &= ~O_NONBLOCK;
		if (fcntl (socket, F_SETFL, flags) < 0) {
			return axl_false;
		} /* end if */
#endif
	} else {
		/* enable non-blocking mode */
#if defined(AXL_OS_WIN32)
		/* win32 case */
		if (!myqtt_win32_nonblocking_enable (socket)) {
			return axl_false;
		}
#else
		/* unix case */
		if ((flags = fcntl (socket, F_GETFL, 0)) < 0) {
			return axl_false;
		}
		
		flags |= O_NONBLOCK;
		if (fcntl (socket, F_SETFL, flags) < 0) {
			return axl_false;
		}
#endif
	} /* end if */

	return axl_true;
}

void __free_addr_info (axlPointer ptr)
{
	freeaddrinfo (ptr);
	return;
}

/** 
 * @internal wrapper to avoid possible problems caused by the
 * gethostbyname implementation which is not required to be re-entrant
 * (thread safe).
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @param hostname The host to translate.
 * 
 * @return A reference to the struct hostent or NULL if it fails to
 * resolve the hostname.
 */
struct addrinfo * myqtt_gethostbyname (MyQttCtx           * ctx, 
					const char          * hostname, 
					const char          * port,
					MyQttNetTransport    transport)
{
	/* get current context */
	struct addrinfo    hints, *res = NULL;
	char             * key;

	/* check that context and hostname are valid */
	if (ctx == NULL || hostname == NULL)
		return NULL;
	
	/* lock and resolve */
	myqtt_mutex_lock (&ctx->connection_hostname_mutex);

	/* resolve using the hash */
	key = axl_strdup_printf ("%s:%s", hostname, port);
	res = axl_hash_get (ctx->connection_hostname, (axlPointer) key);
	if (res) {
		/* release key */
		axl_free (key);

		/* unlock and return the result */
		myqtt_mutex_unlock (&ctx->connection_hostname_mutex);

		return res;

	}

	/* reached this point, key wasn't found, now try to DNS-resolve */

	/* clear hints structure */
	memset (&hints, 0, sizeof(struct addrinfo));

	switch (transport) {
	case MYQTT_IPv4:
		hints.ai_family = AF_INET;
		break;
	case MYQTT_IPv6:
		hints.ai_family = AF_INET6;
		break;
	} /* end switch */
	hints.ai_socktype = SOCK_STREAM;

	/* resolve hostname with hints */
	myqtt_log (MYQTT_LEVEL_DEBUG, "Calling getaddrinfo (%s:%s), transport=%s", hostname, port, transport == MYQTT_IPv6 ? "IPv6" : "IPv4");
	if (getaddrinfo (hostname, port, &hints, &res) != 0) {
		axl_free (key);
		freeaddrinfo (res);
		
		myqtt_mutex_unlock (&ctx->connection_hostname_mutex);
		myqtt_log (MYQTT_LEVEL_CRITICAL, "getaddrinfo (%s:%s) call failed, found errno=%d", hostname, port, errno);
		return NULL;
	}

	/* now store the result */
	axl_hash_insert_full (ctx->connection_hostname, 
			      /* the hostname */
			      key, axl_free,
			      /* the address */
			      res, (axlDestroyFunc) __free_addr_info);

	/* unlock and return the result */
	myqtt_mutex_unlock (&ctx->connection_hostname_mutex);

	/* no need to release key here: this will be done once
	 * ctx->connection_hostname hash is finished */

	return res;
	
}

/** 
 * @internal Function to perform a wait operation on the provided
 * socket, assuming the wait operation will be performed on a
 * nonblocking socket. The function configure the socket to be
 * non-blocking.
 * 
 * @param wait_for The kind of operation to wait for to be available.
 *
 * @param session The socket associated to the MQTT connection.
 *
 * @param wait_period How many seconds to wait for the connection.
 * 
 * @return The error code to return:
 *    -4: Add operation into the file set failed.
 *     1: Wait operation finished.
 *    -2: Timeout
 *    -3: Fatal error found.
 */
int __myqtt_conn_wait_on (MyQttCtx           * ctx,
				MyQttIoWaitingFor    wait_for, 
				MYQTT_SOCKET         session,
				int                 * wait_period)
{
	int           err         = -2;
	axlPointer    wait_set;
	int           start_time;
#if defined(AXL_OS_UNIX)
 	int           sock_err = 0;       
 	unsigned int  sock_err_len;
#endif

	/* do not perform a wait operation if the wait period is zero
	 * or less */
	if (*wait_period <= 0) {
		myqtt_log (MYQTT_LEVEL_WARNING, 
			    "requested to perform a wait operation but the wait period configured is 0 or less: %d",
			    *wait_period);
		return -2;
	} /* end if */

	/* make the socket to be nonblocking */
	myqtt_conn_set_sock_block (session, axl_false);

	/* create a waiting set using current selected I/O
	 * waiting engine. */
	wait_set     = myqtt_io_waiting_invoke_create_fd_group (ctx, wait_for);

	/* flag the starting time */
	start_time   = time (NULL);

	myqtt_log (MYQTT_LEVEL_DEBUG, 
		    "starting connect timeout during %d seconds (starting from: %d)", 
		    *wait_period, start_time);

	/* add the socket in connection transit */
	while ((start_time + (*wait_period)) > time (NULL)) {
		/* clear file set */
		myqtt_io_waiting_invoke_clear_fd_group (ctx, wait_set);

		/* add the socket into the file set (we can pass the
		 * socket and a NULL reference to the MyQttConn
		 * because we won't use dispatch API: we are only
		 * checking for changes) */
		if (! myqtt_io_waiting_invoke_add_to_fd_group (ctx, session, NULL, wait_set)) {
			myqtt_log (MYQTT_LEVEL_WARNING, "failed to add session to the waiting socket");
			err = -4;
			break;
		} /* end if */
				
		/* perform wait operation */
		err = myqtt_io_waiting_invoke_wait (ctx, wait_set, session, wait_for);
		myqtt_log (MYQTT_LEVEL_DEBUG, "__myqtt_conn_wait_on (sock=%d) operation finished, err=%d, errno=%d (%s), wait_for=%d (elapsed: %d)",
			    session, err, errno, myqtt_errno_get_error (errno) ? myqtt_errno_get_error (errno) : "", wait_for, (int) (time (NULL) - start_time));

		/* check for bad file description error */
		if (errno == EBADF) {
			myqtt_log (MYQTT_LEVEL_CRITICAL, "Found bad file descriptor while waiting");
			/* destroy waiting set */
			myqtt_io_waiting_invoke_destroy_fd_group (ctx, wait_set);
			return -3;
		} /* end if */

		if(err == -1 /* EINTR */ || err == -2 /* SSL */)
			continue;
		else if (!err) 
			continue; /*select, poll, epoll timeout*/
		else if (err > 0) {
#if defined(AXL_OS_UNIX)
			/* check estrange case on older linux 2.4 glib
			 * versions that returns err > 0 but it is not
			 * really connected */
			sock_err_len = sizeof(sock_err);
			if (getsockopt (session, SOL_SOCKET, SO_ERROR, (char*)&sock_err, &sock_err_len) < 0){
				myqtt_log (MYQTT_LEVEL_WARNING, "failed to get error level on waiting socket");
				err = -5;
			} else if (sock_err) {
				myqtt_log (MYQTT_LEVEL_WARNING, "error level set on waiting socket");
				err = -6;
			}
#endif
			/* connect ok */
			break; 
		} else if (err /*==-3, fatal internal error, other errors*/)
			return -1;

	} /* end while */
	
	myqtt_log (MYQTT_LEVEL_DEBUG, "timeout operation finished, with err=%d, errno=%d, elapsed time=%d (seconds)", 
		    err, errno, (int) (time (NULL) - start_time));

	/* destroy waiting set */
	myqtt_io_waiting_invoke_destroy_fd_group (ctx, wait_set);

	/* update the return timeout code to signal that the timeout
	 * period was reached */
	if ((start_time + (*wait_period)) <= time (NULL))
		err = -2;

	/* update wait period */
	(*wait_period) -= (time (NULL) - start_time);

	/* return error code */
	return err;
}

/** 
 * @brief Allows to create a plain socket connection against the host
 * and port provided. 
 *
 * @param ctx The context where the connection happens.
 *
 * @param host The host server to connect to.
 *
 * @param port The port server to connect to.
 *
 * @param timeout Parameter where optionally is returned the timeout
 * defined by the library (\ref myqtt_conn_get_connect_timeout)
 * that remains after only doing a socket connected. The value is only
 * returned if the caller provide a reference.
 *
 * @param error Optional axlError reference to report an error code
 * and a textual diagnostic.
 *
 * @return A connected socket or -1 if it fails. The particular error
 * is reported at axlError optional reference.
 */
MYQTT_SOCKET myqtt_conn_sock_connect (MyQttCtx   * ctx,
					      const char  * host,
					      const char  * port,
					      int         * timeout,
					      axlError   ** error)
{
	return myqtt_conn_sock_connect_common (ctx, host, port, timeout, MYQTT_IPv4, error);
}

/** 
 * @brief Allows to create a plain socket connection against the host
 * and port provided allowing to configure the transport. 
 *
 * This function differs from \ref myqtt_conn_sock_connect in
 * the sense it allows to configure the transport (\ref
 * MyQttNetTransport) so you can create TCP/IPv4 (\ref MYQTT_IPv4)
 * and TCP/IPv6 (\ref MYQTT_IPv6) connections.
 *
 * @param ctx The context where the connection happens.
 *
 * @param host The host server to connect to.
 *
 * @param port The port server to connect to.
 *
 * @param timeout Parameter where optionally is returned the timeout
 * defined by the library (\ref myqtt_conn_get_connect_timeout)
 * that remains after only doing a socket connected. The value is only
 * returned if the caller provide a reference.
 *
 * @param transport The network transport to use for this connect operation.
 *
 * @param error Optional axlError reference to report an error code
 * and a textual diagnostic.
 *
 * @return A connected socket or -1 if it fails. The particular error
 * is reported at axlError optional reference.
 */
MYQTT_SOCKET myqtt_conn_sock_connect_common (MyQttCtx            * ctx,
						     const char           * host,
						     const char           * port,
						     int                  * timeout,
						     MyQttNetTransport     transport,
						     axlError            ** error)
{

	struct addrinfo    * res;
	int		     err          = 0;
	MYQTT_SOCKET        session      = -1;

	/* do resolution according to the transport */
	res = myqtt_gethostbyname (ctx, host, port, transport);
        if (res == NULL) {
		myqtt_log (MYQTT_LEVEL_WARNING, "unable to get host name by using myqtt_gethostbyname () host=%s",
			    host);
		axl_error_report (error, MyQttNameResolvFailure, "unable to get host name by using myqtt_gethostbyname ()");
		return -1;
	} /* end if */

	/* create the socket and check if it */
	switch (transport) {
	case MYQTT_IPv4:
		session      = socket (AF_INET, SOCK_STREAM, 0);
		break;
	case MYQTT_IPv6:
		session      = socket (AF_INET6, SOCK_STREAM, 0);
		break;
	default:
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Transport value is not supported (%d), unable to create socket", transport);
		axl_error_report (error, MyQttNameResolvFailure, "Transport value is not supported, unable to create socket");
		return -1;
	}
	if (session == MYQTT_INVALID_SOCKET) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "unable to create socket");
		axl_error_report (error, MyQttNameResolvFailure, "unable to create socket (socket call have failed)");
		return -1;
	} /* end if */

	/* check socket limit */
	if (! myqtt_conn_check_socket_limit (ctx, session)) {
		axl_error_report (error, MyQttSocketSanityError, "Unable to create more connections, socket limit reached");
		return -1;
	}

	/* do a sanity check on socket created */
	if (!myqtt_conn_do_sanity_check (ctx, session)) {
		/* close the socket */
		myqtt_close_socket (session);

		/* report error */
		axl_error_report (error, MyQttSocketSanityError, 
				  "created socket descriptor using a reserved socket descriptor (%d), this is likely to cause troubles");
		return -1;
	} /* end if */
	
	/* disable nagle */
	myqtt_conn_set_sock_tcp_nodelay (session, axl_true);

	/* get current myqtt connection timeout to check if the
	 * application have requested to configure a particular TCP
	 * connect timeout. */
	if (timeout) {
		(*timeout)  = myqtt_conn_get_connect_timeout (ctx); 
		if ((*timeout) > 0) {
			/* translate hold value for timeout into seconds  */
			(*timeout) = (int) (*timeout) / (int) 1000000;
			
			/* set non blocking connection */
			myqtt_conn_set_sock_block (session, axl_false);
		} /* end if */
	} /* end if */

	/* do a tcp connect */
        if (connect (session, res->ai_addr, res->ai_addrlen) < 0) {
		if(timeout == 0 || (errno != MYQTT_EINPROGRESS && errno != MYQTT_EWOULDBLOCK)) { 
			shutdown (session, SHUT_RDWR);
			myqtt_close_socket (session);
			myqtt_log (MYQTT_LEVEL_WARNING, "unable to connect(), session=%d to remote host errno=%d (%s), timeout reached", session, errno, strerror (errno));
			axl_error_report (error, MyQttConnectionError, "unable to connect to remote host");
			return -1;
		} /* end if */
	} /* end if */

	/* if a connection timeout is defined, wait until connect */
	if (timeout && ((*timeout) > 0)) {
		/* wait for write operation, signalling that the
		 * connection is available */
		err = __myqtt_conn_wait_on (ctx, WRITE_OPERATIONS, session, timeout);

#if defined(AXL_OS_WIN32)
		/* under windows we have to also we to be readable */

#endif
		
		if(err <= 0){
			/* timeout reached while waiting for the connection to terminate */
			shutdown (session, SHUT_RDWR);
			myqtt_close_socket (session);
			myqtt_log (MYQTT_LEVEL_WARNING, "unable to connect to remote host (timeout)");
			axl_error_report (error, MyQttNameResolvFailure, "unable to connect to remote host (timeout)");
			return -1;
		} /* end if */
	} /* end if */

	/* return socket created */


	return session;
}

/* now we have to send greetings and process them */
axl_bool __myqtt_conn_send_connect (MyQttCtx * ctx, MyQttConn * conn, MyQttConnOpts * opts) {
	unsigned char * msg;
	int             size;
	unsigned char   flags = 0;

	/* set clean session */
	if (conn->clean_session)
		myqtt_set_bit (&flags, 1);

	/* set opts: auth options */
	if (opts && opts->use_auth) {
		if (opts->username)
			myqtt_set_bit (&flags, 7);
		if (opts->password)
			myqtt_set_bit (&flags, 6);
	} /* end if */

	/* set opts: will options */
	if (opts && opts->will_message && opts->will_topic) {
		myqtt_set_bit (&flags, 2);
		switch (opts->will_qos) {
		case MYQTT_QOS_1:
			myqtt_set_bit (&flags, 3);
			break;
		case MYQTT_QOS_2:
			myqtt_set_bit (&flags, 4);
			break;
		default:
			break;
		} /* end if */
	} /* end if */


	myqtt_log (MYQTT_LEVEL_DEBUG, "Sending CONNECT package to %s:%s (conn-id=%d, flags=%x)", conn->host, conn->port, conn->id, flags);
	
	size = 0;
	msg  = myqtt_msg_build  (ctx, MYQTT_CONNECT, axl_false, 0, axl_false, &size,  /* 2 bytes */
				 MYQTT_PARAM_UTF8_STRING, 4, "MQTT", /* 6 bytes */
				 MYQTT_PARAM_8BIT_INT, 4, /* 1 byte */
				 /* connect flags */
				 MYQTT_PARAM_8BIT_INT, flags, /* 1 byte */
				 /* keep alive value, for now nothing */
				 MYQTT_PARAM_16BIT_INT, 0, /* 2 bytes */
				 /* client identifier */
				 MYQTT_PARAM_UTF8_STRING, strlen (conn->client_identifier), conn->client_identifier, /* 19 */
				 /* will topic */
				 (opts && opts->will_topic) ? MYQTT_PARAM_UTF8_STRING : MYQTT_PARAM_SKIP, (opts && opts->will_topic) ? strlen (opts->will_topic) : 0, opts ? opts->will_topic : NULL,
				 /* will message */
				 (opts && opts->will_message) ? MYQTT_PARAM_UTF8_STRING : MYQTT_PARAM_SKIP, (opts && opts->will_message) ? strlen (opts->will_message) : 0, opts ? opts->will_message : NULL,
				 /* user */
				 (opts && opts->use_auth && opts->username) ? MYQTT_PARAM_UTF8_STRING : MYQTT_PARAM_SKIP, (opts && opts->username) ? strlen (opts->username) : 0, opts ? opts->username : NULL,
				 /* password */
				 (opts && opts->use_auth && opts->password) ? MYQTT_PARAM_UTF8_STRING : MYQTT_PARAM_SKIP, (opts && opts->password) ? strlen (opts->password) : 0, opts ? opts->password : NULL,
				 MYQTT_PARAM_END);

	/* in any case, remove this password from memory if defined */
	if (opts && opts->password) {
	        /* do not remove the password in the case a reconnect
		   was required: otherwise, without the password, the
		   reconnection is not possible. See \ref
		   myqtt_conn_opts_set_reconnect */
	        if (! opts->reconnect) {
		       axl_free (opts->password);
		       opts->password = NULL;
		} /* end if */
	} /* end if */

	/* now check myqtt_msg_build () result */
	if (msg == NULL || size == 0) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Failed to create CONNECT message, empty/NULL value reported by myqtt_msg_build()");
		return axl_false;
	} /* end if */

	myqtt_log (MYQTT_LEVEL_DEBUG, "Created a CONNECT message of %d bytes", size);
	
	if (! myqtt_msg_send_raw (conn, msg, size)) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Failed to send CONNECT message, errno=%d", errno);
		/* call to release */
		myqtt_msg_free_build (ctx, msg, size);
		return axl_false;
	} /* end if */
	
	/* report message sent */
	myqtt_log (MYQTT_LEVEL_DEBUG, "CONNECT message of %d bytes sent", size);

	/* call to release */
	myqtt_msg_free_build (ctx, msg, size);

	return axl_true;
}

axl_bool __myqtt_conn_parse_greetings (MyQttCtx * ctx, MyQttConn * connection, MyQttMsg * msg) {
	/* parse incoming greetings message received */
	if (msg->type != MYQTT_CONNACK) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Received unexpected control packet type (%d : %s)", msg->type, myqtt_msg_get_type_str (msg));
		return axl_false;
	} /* end if */

	if (msg->size != 2) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Expected packet reply of 2 but found: %d", msg->size);
		return axl_false;
	}

	if (msg->payload[1] != MYQTT_CONNACK_ACCEPTED) 
		myqtt_log (MYQTT_LEVEL_WARNING, "Connection denied by remote peer, conn ack value received is: %d", msg->payload[1]);

	/* record reported code */
	connection->last_err     = msg->payload[1];

	/* check the payload */
	return connection->last_err == MYQTT_CONNACK_ACCEPTED;
}

axl_bool      myqtt_conn_parse_greetings_and_enable (MyQttConn * connection, 
						     MyQttMsg  * msg)
{
	MyQttCtx        * ctx;

	/* check parameters received */
	if (connection == NULL || msg == NULL)
		return axl_false;

	/* get the connection context */
	ctx = connection->ctx;

	/* process msg response */
	if (msg != NULL) {
		/* now check reply received */
		if (!__myqtt_conn_parse_greetings (ctx, connection, msg)) {
			/* parse ok, free msg and establish new
			 * message */
			myqtt_msg_unref (msg);

			/* something wrong have happened while parsing
			 * XML greetings */
			return axl_false;
		} /* end if */
		myqtt_log (MYQTT_LEVEL_DEBUG, "greetings parsed..");
		
		/* parse ok, free msg and establish new message */
		myqtt_msg_unref (msg);

		myqtt_log (MYQTT_LEVEL_DEBUG, "new connection created to %s:%s (client id: %s)", connection->host, connection->port, connection->client_identifier);

		/* now, every initial message have been sent, we need
		 * to send this to the reader manager */
		myqtt_reader_watch_connection (ctx, connection); 

		return axl_true;
	}

	myqtt_log (MYQTT_LEVEL_CRITICAL, "received a null msg (null reply) from remote side when expected a greetings reply, closing session");

	__myqtt_conn_shutdown_and_record_error (
		connection, MyQttProtocolError,
		"received a null msg (null reply) from remote side when expected a greetings reply, closing session");
	return axl_false;
}

axl_bool __myqtt_conn_do_greetings_exchange (MyQttCtx      * ctx, 
					     MyQttConn     * connection, 
					     MyQttConnOpts * opts,
					     int             timeout)
{
	MyQttMsg            * msg;
	int                   err;

	v_return_val_if_fail (myqtt_conn_is_ok (connection, axl_false), axl_false);

	/* now we have to send greetings and process them */
	if (! __myqtt_conn_send_connect (ctx, connection, opts)) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Failed to send CONNECT package to broker (conn-id=%d)", connection->id);
		connection->last_err     = MYQTT_CONNACK_REFUSED;
		return axl_false;
	}
	
	myqtt_log (MYQTT_LEVEL_DEBUG, "greetings sent, waiting for reply");

	if (timeout == 0) {
		/* make the connection to be blocking during the
		 * greetings process (if it were not) */
		myqtt_conn_set_blocking_socket (connection);
	} /* end if */

	/* while we did not finish */
	while (axl_true) {

		/* block thread until received remote greetings */
		myqtt_log (MYQTT_LEVEL_DEBUG, "getting CONNECT reply..");
		msg = myqtt_msg_get_next (connection);
		myqtt_log (MYQTT_LEVEL_DEBUG, "finished wait for initial greetings msg=%p timeout=%d, conn-id=%d, socket=%d", 
			   msg, timeout, connection->id, connection->session);

		/* check msg received */
		if (msg != NULL) {

			myqtt_log (MYQTT_LEVEL_DEBUG, "greetings received, process reply msg");
			break;

		} else if (timeout > 0 && myqtt_conn_is_ok (connection, axl_false)) {

			myqtt_log (MYQTT_LEVEL_WARNING, 
				    "found NULL msg reference connection=%d, checking to wait timeout=%d seconds for read operation..",
				    connection->id, timeout);

			/* try to perform a wait operation */
			err = __myqtt_conn_wait_on (ctx, READ_OPERATIONS, connection->session, &timeout);
			if (err <= 0 || timeout <= 0) {
				/* timeout reached while waiting for the connection to terminate */
				myqtt_log (MYQTT_LEVEL_WARNING, 
					    "reached timeout=%d or general operation failure=%d while waiting for initial greetings msg for connection id=%d",
					    err, timeout, connection->id);
				
				/* close the connection */
				shutdown (connection->session, SHUT_RDWR);
				myqtt_close_socket (connection->session);
				connection->session      = -1;
				connection->last_err     = MYQTT_CONNACK_CONNECT_TIMEOUT;

				/* error found, stop greetings process */
				return axl_false;
			} /* end if */
				
			myqtt_log (MYQTT_LEVEL_DEBUG,
				    "found the connection is ready to provide data, checking..");
			continue;
		} else {

			if (errno == MYQTT_EWOULDBLOCK) {
				/* try to get connection again */
				continue;
			} /* end if */

			/* null msg received */
			myqtt_log (MYQTT_LEVEL_CRITICAL,
				   "Connection refused. Received null msg were it was expected initial greetings, finish connection id=%d", connection->id);
			
			/* timeout reached while waiting for the connection to terminate */
			shutdown (connection->session, SHUT_RDWR);
			myqtt_close_socket (connection->session);
			connection->session      = -1;
			connection->is_connected = axl_false;
			connection->last_err     = MYQTT_CONNACK_REFUSED;

			return axl_false;
		} /* end if */
		
	} /* end while */

	/* process msg response */
	if (!myqtt_conn_parse_greetings_and_enable (connection, msg))
		return axl_false;

	myqtt_log (MYQTT_LEVEL_DEBUG, "greetings exchange ok");

	return axl_true;
} 
			

void __myqtt_conn_new_internal (MyQttCtx * ctx, MyQttConn * connection, MyQttConnOpts * opts)
{
	int                       d_timeout       = myqtt_conn_get_connect_timeout (ctx);
	axlError                * error           = NULL;
	struct sockaddr_storage   sin;
#if defined(AXL_OS_WIN32)
	/* windows flavors */
	int                    sin_size           = sizeof (sin);
#else
	/* unix flavors */
	socklen_t              sin_size           = sizeof (sin);
#endif
	char                   host_name[NI_MAXHOST];
	char                   srv_name[NI_MAXSERV]; 
	axlPointer             setup_user_data;

	/* call session setup handler if defined */
	if (connection->setup_handler)  {
		/* init setup user data */
		setup_user_data = connection->setup_user_data;

		/* before calling session setup, check and call init session setup if defined */
		if (setup_user_data == NULL &&  opts && opts->init_session_setup_ptr) {
			/* call to create the session setup pointer */
			setup_user_data = opts->init_session_setup_ptr (ctx, connection, 
									opts->init_user_data, 
									opts->init_user_data2,
									opts->init_user_data3);

			/* check error reported */
			if (setup_user_data == NULL) {
				myqtt_log (MYQTT_LEVEL_CRITICAL, "Init Session setup pointer failed for conn-id=%d, session=%d..", connection->id, connection->session);
				connection->session = -1;
				connection->is_connected = axl_false;
				goto handle_error;
			} /* end if */

		} /* end if */

		/* by default, cleanup session */
		connection->session = -1;
		if (! connection->setup_handler (ctx, connection, opts, setup_user_data)) {
			myqtt_log (MYQTT_LEVEL_CRITICAL, "Session setup handler failed for conn-id=%d, session=%d..", connection->id, connection->session);
			myqtt_close_socket (connection->session);
			connection->session = -1;
		} /* end if */
	} else {
		/* configure the socket created */
		connection->session = myqtt_conn_sock_connect_common (ctx, connection->host, connection->port, &d_timeout, connection->transport, &error);
	} /* end if */

 handle_error:
	if (connection->session == -1) {
		/* configure error */
		connection->last_err     = MYQTT_CONNACK_UNABLE_TO_CONNECT;

		/* get error message and error status */
		axl_error_free (error);

		/* flag as not connected */
		connection->is_connected = axl_false;
	} else {
		/* flag as connected */
		connection->is_connected = axl_true;
	} /* end if */

	myqtt_log (MYQTT_LEVEL_DEBUG, "Did session socket work? conn-id=%d, session=%d..", connection->id, connection->session);
	
	/* according to the connection status (is_connected attribute)
	 * perform the final operations so the connection becomes
	 * usable. Later, the user app level is notified. */
	if (connection->is_connected) {

		/* configure local address used by this connection */
		/* now set local address */
		if (getsockname (connection->session, (struct sockaddr *) &sin, &sin_size) < 0) {
			myqtt_log (MYQTT_LEVEL_DEBUG, "unable to get local hostname and port to resolve local address");
			connection->is_connected = axl_false;
			connection->last_err     = MYQTT_CONNACK_UNABLE_TO_CONNECT;
			goto report_connection;

		} /* end if */

		/* set host and port from socket recevied */
		memset (host_name, 0, NI_MAXHOST);
		memset (srv_name, 0, NI_MAXSERV);
		if (getnameinfo ((struct sockaddr *) &sin, sin_size, host_name, NI_MAXHOST, srv_name, NI_MAXSERV, NI_NUMERICSERV | NI_NUMERICHOST) != 0) {
			myqtt_log (MYQTT_LEVEL_CRITICAL, "getnameinfo () call failed, error was errno=%d", errno);
			connection->is_connected = axl_false;
			connection->last_err     = MYQTT_CONNACK_UNABLE_TO_CONNECT;
			goto report_connection;
		}
	
		/* set local addr and local port */
		if (connection->local_addr)
			axl_free (connection->local_addr);
		connection->local_addr = axl_strdup (host_name);
		
		if (connection->local_port)
			axl_free (connection->local_port);
		connection->local_port = axl_strdup (srv_name);

		/* block thread until received remote greetings */
		if (! __myqtt_conn_do_greetings_exchange (ctx, connection, opts, d_timeout)) { 
			myqtt_log (MYQTT_LEVEL_CRITICAL, "MQTT greetings exchange failed (conn-id=%d) with MQTT broker, error was errno=%d", 
				   connection->id, errno);
			connection->is_connected = axl_false;
			goto report_connection;
		}

		/* connection ok */
		myqtt_log (MYQTT_LEVEL_DEBUG, "MQTT connection OK (conn-id=%d)", connection->id);
		connection->last_err           = MYQTT_CONNACK_ACCEPTED;
	} /* end if */

 report_connection:
	/* release conn opts if defined and reuse is not set */
	if (opts && ! opts->reuse && ! opts->reconnect) 
		myqtt_conn_opts_free (opts);

	/* check if the connection has pending messages to be sent to process them now  */
	if (connection->last_err == MYQTT_CONNACK_ACCEPTED && ! connection->clean_session) {
		myqtt_log (MYQTT_LEVEL_DEBUG, "RESENDING: checking for pending messages: %d", myqtt_storage_queued_messages (ctx, connection));
		/* check for pending messages */
		if (myqtt_storage_queued_messages (ctx, connection) > 0) {
			/* we have pending messages, order to deliver them */
			myqtt_storage_queued_flush (ctx, connection);
		} /* end if */
	}

	return; /* report nothing */
}

/** 
 * @internal
 * @brief Support function to myqtt_conn_new. 
 *
 * This function actually does the work for the myqtt_conn_new.
 * 
 * @param data To perform myqtt connection creation process
 * 
 * @return on thread model NULL on non-thread model the connection
 * created (connected or not connected).
 */
axlPointer __myqtt_conn_new (MyQttConnNewData * data)
{
	/* get current context */
	MyQttConn            * connection      = data->connection;
	MyQttConnOpts        * opts            = data->opts;
	MyQttCtx             * ctx             = connection->ctx;
	axl_bool               threaded        = data->threaded;
	MyQttConnNew           on_connected    = data->on_connected;
	axlPointer             user_data       = data->user_data;

	myqtt_log (MYQTT_LEVEL_DEBUG, "executing connection new in %s mode to %s:%s id=%d",
		   (data->threaded == axl_true) ? "thread" : "blocking", 
		   connection->host, connection->port,
		   connection->id);

	/* release data */
	axl_free (data);

	/* record reference for the future: we have to do it here
	   because it will be released in the case reconnect is not
	   enabled */
	if (opts && opts->reconnect) {
		/* setup connection options to use them in the future */
		connection->opts = opts;
	} /* end if */

	/* call to create connection */
	__myqtt_conn_new_internal (ctx, connection, opts);

	/* notify on callback or simply return */
	if (threaded) {
		/* notify connection */
		on_connected (connection, user_data);
		return NULL;
	} /* end if */

	return connection;
}

MyQttConn  * myqtt_conn_new_full_common        (MyQttCtx             * ctx,
						const char           * client_identifier,
						axl_bool               clean_session,
						int                    keep_alive,
						const char           * host, 
						const char           * port,
						MyQttSessionSetup      setup_handler,
						axlPointer             setup_user_data,
						MyQttConnNew           on_connected, 
						MyQttNetTransport      transport,
						MyQttConnOpts        * opts,
						axlPointer             user_data)
{
	MyQttConnNewData * data;
	struct timeval     stamp;
	char               hostname[513];

	/* check context is initialised */
	if ((! myqtt_init_check (ctx)) || ctx->myqtt_exit) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Failed to create connection myqtt_init_check () is failing or myqtt is exiting");
		return NULL;
	} /* end if */

	/* check parameters */
	if (setup_handler == NULL && (host == NULL || port == NULL)) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "You didn't provide a host/port value to connect to nor a setup handler. Unable to connect");
		return NULL;
	}

	data                                  = axl_new (MyQttConnNewData, 1);
	if (data == NULL) 
		return NULL;
	
	data->connection                      = axl_new (MyQttConn, 1);
	data->opts                            = opts;
	data->connection->setup_handler       = setup_handler;
	data->connection->setup_user_data     = setup_user_data;

	/* configure connection transport */
	data->connection->transport           = transport;

	/* for now, set default connection error */
	data->connection->last_err            = MYQTT_CONNACK_UNKNOWN_ERR;
	data->connection->clean_session       = clean_session;

	/* check allocated connection */
	if (data->connection == NULL) {
		axl_free (data);
		return NULL;
	} /* end if */

	if (client_identifier == NULL) {
		gettimeofday (&stamp, NULL);
		memset (hostname, 0, 512);
		gethostname (hostname, 512);

		data->connection->client_identifier   = axl_strdup_printf ("%s-%ld-%ld-%ld", hostname, data->connection, stamp.tv_sec, stamp.tv_usec);
		/* force clean session if client identifier is not provided */
		data->connection->clean_session       = axl_true;
	} else
		data->connection->client_identifier   = axl_strdup (client_identifier);

	data->connection->id                  = __myqtt_conn_get_next_id (ctx);
	data->connection->ctx                 = ctx;
	myqtt_ctx_ref2 (ctx, "new connection"); /* acquire a reference to context */
	data->connection->host                = axl_strdup (host);
	data->connection->port                = axl_strdup (port);
	data->connection->ref_count           = 1;

	/* wait reply hash */
	data->connection->wait_replies        = axl_hash_new (axl_hash_int, axl_hash_equal_int);
	data->connection->peer_wait_replies   = axl_hash_new (axl_hash_int, axl_hash_equal_int);

	/* subscriptions **/
	data->connection->subs                = axl_hash_new (axl_hash_string, axl_hash_equal_string);
	data->connection->wild_subs           = axl_hash_new (axl_hash_string, axl_hash_equal_string);

	/* call to init all mutex associated to this particular connection */
	__myqtt_conn_init_mutex (data->connection);

	data->connection->data                = myqtt_hash_new_full (axl_hash_string, axl_hash_equal_string,
								     NULL,
								     NULL);
	/* establish the connection role */
	data->connection->role                = MyQttRoleInitiator;

	/* set default send and receive handlers */
	data->connection->send                = myqtt_conn_default_send;
	data->connection->receive             = myqtt_conn_default_receive;

	/* set by default to close the underlying connection when the
	 * connection is closed */
	data->connection->close_session       = axl_true;

	/* establish connection status, connection negotiation method
	 * and user data */
	data->on_connected                    = on_connected;
	data->user_data                       = user_data;
	data->threaded                        = (on_connected != NULL);

	/* check allocated values */
	if (data->connection->host          == NULL ||
	    data->connection->port          == NULL ||
	    data->connection->data          == NULL) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Conn memory allocation failed.."); 
		/* connection allocation failed */
		myqtt_conn_free (data->connection);
		axl_free (data);
		return NULL;
	}

	if (data->threaded) {
		myqtt_log (MYQTT_LEVEL_DEBUG, "invoking connection_new threaded mode");
		myqtt_thread_pool_new_task (ctx, (MyQttThreadFunc) __myqtt_conn_new, data);
		return NULL;
	}
	myqtt_log (MYQTT_LEVEL_DEBUG, "invoking connection_new non-threaded mode");
	return __myqtt_conn_new (data);
}

/** 
 * @brief Allows to create a new MQTT connection to a MQTT broker/server.
 *
 * @param ctx The context where the operation will take place.
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
 * @param host The location of the MQTT server/broker
 *
 * @param port The port of the MQTT server/broker
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
 * @return A reference to the newly created connection or NULL if
 * on_connected handler is provided. In both cases, the reference
 * returned (or received at the on_connected handler) must be checked
 * with \ref myqtt_conn_is_ok. 
 *
 * <b>About pending messages / queued messages </b>
 *
 * After successful connection with clean_session set to axl_false and
 * defined client identifier, the library will resend any queued or in
 * flight QoS1/QoS2 messages (as well as QoS0 if they were
 * stored). This is done in background without interfering the caller.
 *
 * If you need to get the number of queued messages that are going to
 * be sent use \ref myqtt_storage_queued_messages_offline. In the case
 * you need the number remaining during the process use \ref
 * myqtt_storage_queued_messages.
 *
 */
MyQttConn  * myqtt_conn_new                    (MyQttCtx       * ctx,
						const char     * client_identifier,
						axl_bool         clean_session,
						int              keep_alive,
						const char     * host, 
						const char     * port,
						MyQttConnOpts  * opts,
						MyQttConnNew     on_connected, 
						axlPointer       user_data)
{
	MyQttNetTransport transport;
	
	/* get transport we have to use */
	transport = __myqtt_conn_detect_transport (ctx, host);

	/* call to create the connection */
	return myqtt_conn_new_full_common (ctx, client_identifier, clean_session, keep_alive, host, port, NULL, NULL, on_connected, transport, opts, user_data);
}

/** 
 * @brief Allows to create a new MQTT connection to a MQTT* broker/server, forcing IPv6 transport.
 *
 * @param ctx The context where the operation will take place.
 *
 * @param client_identifier The client identifier that uniquely
 * identifies this MQTT client from others. 
 *
 * @param clean_session Flag to clean client session or to reuse the existing one.
 *
 * @param keep_alive Keep alive configuration in seconds after which
 * the server/broker will close the connection if it doesn't detect
 * any activity. Setting 0 will disable keep alive mechanism.
 *
 * @param host The location of the MQTT server/broker
 *
 * @param port The port of the MQTT server/broker
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
 * @return A reference to the newly created connection or NULL if
 * on_connected handler is provided. In both cases, the reference
 * returned (or received at the on_connected handler) must be checked
 * with \ref myqtt_conn_is_ok.
 */
MyQttConn  * myqtt_conn_new6                   (MyQttCtx       * ctx,
						const char     * client_identifier,
						axl_bool         clean_session,
						int              keep_alive,
						const char     * host, 
						const char     * port,
						MyQttConnOpts  * opts,
						MyQttConnNew     on_connected, 
						axlPointer       user_data)
{
	/* call to create the connection */
	return myqtt_conn_new_full_common (ctx, client_identifier, clean_session, keep_alive, host, port, 
					   /* session setup handlers */
					   NULL, NULL, 
					   /* on connected handlers, transport, options and user data */
					   on_connected, MYQTT_IPv6, opts, user_data);
}

/** 
 * @brief Allows to reconnect the given connection, using actual connection settings.
 * 
 * Given a connection that is actually broken, this function can be
 * used to reconnect it trying to find, on the same host and port, the
 * peer that the connection was linked to.
 *
 * The connection "reconnected" will keep the connection identifier,
 * any data set on this connection using the \ref
 * myqtt_conn_set_data and \ref myqtt_conn_set_data_full
 * functions, the connection reference count....
 *
 * If the function detect that the given connection is currently
 * working and connected then the function does nothing.
 *
 * Here is an example:
 * \code
 * // check if the connection is broken, maybe remote site have
 * // failed.
 * if (! myqtt_conn_is_ok (connection, axl_false)) {
 *       // connection is not available, try to reconnect.
 *       if (! myqtt_conn_reconnect (connection,
 *                                          // no async notify
 *                                          NULL,
 *                                          // no user data
 *                                          NULL)) {
 *            printf ("Link failure detected..");
 *       }else {
 *            printf ("Unable to reconnect to remote site..");
 *       }
 * } // end if
 * \endcode
 * 
 * @param connection    The connection to reconnect to.
 *
 * @param on_connected Optional handler. This function works as \ref
 * myqtt_conn_new. This handler can be defined to notify when
 * the connection has been reconnected.
 *
 * @param user_data User defined data to be passed to on_connected.
 * 
 * @return axl_true if the connection was successfully reconnected or axl_false
 * if not. If the <i>on_connected</i> handler is defined, this
 * function will always return axl_true.
 */
axl_bool            myqtt_conn_reconnect              (MyQttConn    * connection,
						       MyQttConnNew   on_connected,
						       axlPointer     user_data)
{
	MyQttCtx               * ctx;
	MyQttConnNewData * data;

	/* we have to check a basic case. A null connection. Null
	 * connection no data to reestablish the connection. */
	if (connection == NULL)
		return axl_false;

	if (myqtt_conn_is_ok (connection, axl_false))
		return axl_true;

	/* get a reference to the ctx */
	ctx = connection->ctx;

	myqtt_log (MYQTT_LEVEL_DEBUG, "requested a reconnection on a connection which isn't null and is not connected..");

	/* create data needed to invoke the service */
	data               = axl_new (MyQttConnNewData, 1);
	/* check allocated value */
	if (data == NULL) 
		return axl_false;

	data->connection   = connection;
	data->on_connected = on_connected;
	data->user_data    = user_data;
	data->threaded     = (on_connected != NULL);

	myqtt_log (MYQTT_LEVEL_DEBUG, "reconnection request is prepared, doing so..");

	if (data->threaded) {
		myqtt_log (MYQTT_LEVEL_DEBUG, "reconnecting connection in threaded mode");
		myqtt_thread_pool_new_task (ctx, (MyQttThreadFunc) __myqtt_conn_new, data);
		return axl_true;
	}
	myqtt_log (MYQTT_LEVEL_DEBUG, "reconnecting connection in non-threaded mode");
	return myqtt_conn_is_ok (__myqtt_conn_new (data), axl_false);

}

/** 
 * @brief Allows to complete connect operation by sending a particular
 * response. This function is used in combination with \ref MYQTT_CONNACK_DEFERRED 
 * that allows a server side MQTT server to tell the library engine to skip reporting to the connecting client.
 *
 * This \ref MYQTT_CONNACK_DEFERRED response code is reported at the
 * \ref MyQttOnConnectHandler handler (for example, when configuring
 * the on connect handler at \ref myqtt_ctx_set_on_connect).
 *
 * @param conn The connection for which a response will be sent.
 *
 * @param response The response to send. 
 */
void            myqtt_conn_send_connect_reply (MyQttConn * conn, MyQttConnAckTypes response)
{
	MyQttCtx          * ctx;
	int                 size = 0;
	unsigned char     * reply;

	if (conn == NULL)
		return;

	/* handle deferred responses */
	if (response == MYQTT_CONNACK_DEFERRED) {
		return;
	} /* end if */

	if (! myqtt_conn_ref (conn, "send_connect_reply")) 
		return;

	/* get reference to the context used by the connection */
	ctx = conn->ctx;

	/* session recovery: connection accepted, if it has session recover */
	if (! ctx->skip_storage_init) {
		if (! conn->clean_session && response == MYQTT_CONNACK_ACCEPTED) {

			/* move subscriptions from offline to online */
			if (myqtt_storage_init (ctx, conn, MYQTT_STORAGE_ALL) && myqtt_storage_session_recover (ctx, conn)) {
			        /* session recovered, now remove offline subscriptions */
				__myqtt_reader_move_offline_to_online (ctx, conn);
			} else {
			        /* session recover failed, report it */
			        myqtt_log (MYQTT_LEVEL_CRITICAL, "Failed to recover session for the provided connection, unable to accept connection");
				response = MYQTT_CONNACK_SERVER_UNAVAILABLE;
			} /* end if */

		} else if (conn->clean_session && response == MYQTT_CONNACK_ACCEPTED) {

			/* init storage if it has session */
			if (! myqtt_storage_clear (ctx, conn, MYQTT_STORAGE_ALL)) {
				/* session clear failed, report it */
			        myqtt_log (MYQTT_LEVEL_CRITICAL, "Failed to clear session for the provided connection, unable to accept connection");
				response = MYQTT_CONNACK_SERVER_UNAVAILABLE;
			} 

		} /* end if */
		
	} /* end if */

	/* rest of cases, reply with the response */
	reply = myqtt_msg_build (ctx, MYQTT_CONNACK, axl_false, 0, axl_false, &size, 
				 /* variable header and payload */
				 MYQTT_PARAM_16BIT_INT, response, 
				 MYQTT_PARAM_END);

	/* send message */
	if (! myqtt_msg_send_raw (conn, reply, size)) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Failed to send CONNACK message, errno=%d", errno);
	} /* end if */

	/* free reply */
	myqtt_msg_free_build (ctx, reply, size);

	/* close connection in the case it is not an accepted */
	if (response != MYQTT_CONNACK_ACCEPTED) {
		myqtt_conn_shutdown (conn);
		myqtt_log (MYQTT_LEVEL_WARNING, "Connection conn-id=%d denied from %s:%s", conn->id, conn->host, conn->port);
	} else {
		myqtt_log (MYQTT_LEVEL_DEBUG, "Connection conn-id=%d accepted from %s:%s", conn->id, conn->host, conn->port);

		/* resend messages queued */
		if (myqtt_storage_queued_messages (ctx, conn) > 0) {
			/* we have pending messages, order to deliver them */
			myqtt_storage_queued_flush (ctx, conn);
		} /* end if */

		/* flag the connection as fully accepted */
		conn->initial_accept = axl_false;

	} /* end if */

	/* release connection reference */
	myqtt_conn_unref (conn, "send_connect_reply");

	return;
}

/** 
 * @brief After a \ref myqtt_conn_new you can call this function to get the last error reported.
 *
 * @param conn The connection where the last error is being queried.
 *
 * @return The error code (see \ref MyQttConnAckTypes for more
 * information) or \ref MYQTT_CONNACK_ACCEPTED if the connection went
 * ok.
 */
MyQttConnAckTypes   myqtt_conn_get_last_err    (MyQttConn * conn)
{
	if (conn == NULL)
		return MYQTT_CONNACK_UNKNOWN_ERR;
	return conn->last_err;
}

/** 
 * @brief Allows to get textual error from error code provided.
 *
 * @param code The error code to get textual diagnostics from.
 *
 * @return Textual error or NULL if it fails.
 */
const char        * myqtt_conn_get_code_to_err (MyQttConnAckTypes code)
{
	switch (code) {
	case MYQTT_CONNACK_UNKNOWN_ERR:
		return "Local error code reported when unknown error happened.";
	case MYQTT_CONNACK_CONNECT_TIMEOUT:
		return "Local error code reported when connection fails with a timeout";
	case MYQTT_CONNACK_UNABLE_TO_CONNECT:
		return "Local error code reported when connection was not possible (connection refused)";
	case MYQTT_CONNACK_DEFERRED:
		return "Report code used to indicate the library to skip replying the particular connect method";
	case MYQTT_CONNACK_ACCEPTED:
		return "Return code for connection accepted.";
	case MYQTT_CONNACK_REFUSED:
		return "Return code for Connection refused, unacceptable protocol version";
	case MYQTT_CONNACK_IDENTIFIER_REJECTED:
		return "Return code for connection refused, server unavailable";
	case MYQTT_CONNACK_SERVER_UNAVAILABLE:
		return "Return code for connection refused, bad user name or password";
	case MYQTT_CONNACK_BAD_USERNAME_OR_PASSWORD:
		return "Return code for connection refused, bad user name or password";
	case MYQTT_CONNACK_NOT_AUTHORIZED:
		return "Return code for connection refused, not authorised";
	}

	return "Unknown error code";
	
}

/** 
 * @internal Get the next package id available over the provided
 * connection.
 */
int __myqtt_conn_get_next_pkgid_aux (MyQttCtx * ctx, MyQttConn * conn, MyQttQos qos)
{
	int      pkg_id = 1;
	int      value;
	int      iterator;

	/* init pkgids database */
	if (! myqtt_storage_init (ctx, conn, MYQTT_STORAGE_PKGIDS)) {
		/* failed to initialise package ids */
		return pkg_id;
	} /* end if */

	/* lock operation mutex */
	myqtt_mutex_lock (&conn->op_mutex);

	/* get the list */
	if (conn->sent_pkgids == NULL)
		conn->sent_pkgids = axl_list_new (axl_list_equal_int, NULL);

	if (axl_list_length (conn->sent_pkgids) == 0 && axl_hash_items (conn->wait_replies) == 0) {

		while (pkg_id < 65535 && ! myqtt_storage_lock_pkgid (ctx, conn, pkg_id)) {
			/* id is already in use, go for the next */
			pkg_id ++;
		} /* end if */

		/* report default pkgid = 1 */
		axl_list_append (conn->sent_pkgids, INT_TO_PTR (pkg_id));

	} else {
		iterator = 0;
		while (iterator < axl_list_length (conn->sent_pkgids)) {
			/* get stored value */
			value = PTR_TO_INT (axl_list_get_nth (conn->sent_pkgids, iterator));

			/* if pkg id is already in use, jump to next value */
			if (pkg_id == value) {
				pkg_id ++;
			} else if (pkg_id < value) {

				/* check there is no wait reply still looked on the provided hash which represents 
				   wait replies for packages sent which are still waiting */
				if (axl_hash_exists (conn->wait_replies, INT_TO_PTR (pkg_id))) {
					pkg_id ++;
					iterator++;
					continue;
				} /* end if */

				/* we have session support, so we have to ensure we can use this packet id, 
				   that is, it is not used by any other operation in progress */
				if (! myqtt_storage_lock_pkgid (ctx, conn, pkg_id)) {
					/* id is already in use, go for the next */
					pkg_id ++;
					iterator++;
					continue;
				} /* end if */
					

				/* insert value into this position */
				axl_list_add_at (conn->sent_pkgids, INT_TO_PTR (pkg_id), iterator);

				/* unlock operation mutex */
				myqtt_mutex_unlock (&conn->op_mutex);

				return pkg_id;
			} /* end if */

			/* next operation */
			iterator++;
		}

		/* reached this point we are adding it on the end */
		axl_list_append (conn->sent_pkgids, INT_TO_PTR (pkg_id));
	} /* end if */

	/* unlock operation mutex */
	myqtt_mutex_unlock (&conn->op_mutex);

	return pkg_id;
}

int __myqtt_conn_get_next_pkgid (MyQttCtx * ctx, MyQttConn * conn, MyQttQos qos) {
	int pkg_id;
	int iterator = 0;

	/* list current ids */
	while (iterator < axl_list_length (conn->sent_pkgids)) {
		myqtt_log (MYQTT_LEVEL_DEBUG, "NEW PACKET ID: ...item: %d) %d", 
			   iterator, PTR_TO_INT (axl_list_get_nth (conn->sent_pkgids, iterator)));

		iterator++;
	} /* end while */

	pkg_id = __myqtt_conn_get_next_pkgid_aux (ctx, conn, qos);
	myqtt_log (MYQTT_LEVEL_DEBUG, "NEW PACKET ID: Reporting next available pkg-id=%d for ctx=%p conn=%p conn-id=%d qos=%d",
		   pkg_id, ctx, conn, conn->id, qos);
	

	return pkg_id;
}

void __myqtt_conn_release_pkgid (MyQttCtx * ctx, MyQttConn * conn, int pkg_id)
{
	/* unlock operation mutex */
	myqtt_mutex_lock (&conn->op_mutex);

	axl_list_remove (conn->sent_pkgids, INT_TO_PTR (pkg_id));
	myqtt_storage_release_pkgid (ctx, conn, pkg_id);

	/* unlock operation mutex */
	myqtt_mutex_unlock (&conn->op_mutex);

	return;
}

/** 
 * @internal Function used to send message and handle reply handle for
 * PUBLISH (qos 0, 1 and 2). It also handles local storage.
 */
axl_bool __myqtt_conn_pub_send_and_handle_reply (MyQttCtx      * ctx, 
						 MyQttConn     * conn, 
						 int             packet_id, 
						 MyQttQos        qos, 
						 axlPointer      handle, 
						 int             wait_publish, 
						 unsigned char * msg, 
						 int             size)
{

	MyQttMsg   * reply;
	axl_bool     result = axl_true;

	/* skip storage if requested by the caller. */
	axl_bool     skip_storage         = (qos & MYQTT_QOS_SKIP_STORAGE) == MYQTT_QOS_SKIP_STORAGE;
	axl_bool     wait_reply_installed = axl_false;

	if (msg == NULL || size == 0) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Failed to create PUBLISH message, empty/NULL value reported by myqtt_msg_build()");
		return axl_false;
	} /* end if */

	if (((qos & MYQTT_QOS_1) == MYQTT_QOS_1 || (qos & MYQTT_QOS_2) == MYQTT_QOS_2) && wait_publish > 0) {
		/* prepare reply */
		__myqtt_reader_prepare_wait_reply (conn, packet_id, axl_false);
		wait_reply_installed = axl_true;
	} /* end if */

	/* configure package to send */
	myqtt_log (MYQTT_LEVEL_DEBUG, "Sending PUBLISH with packet_id=%d conn-id=%d conn=%p qos=%d wait_publish=%d", 
		   packet_id, conn->id, conn, qos, wait_publish);
	if (! myqtt_sequencer_send (conn, MYQTT_PUBLISH, msg, size)) {

		/* release wait reply queue */
		if (wait_reply_installed) 
			__myqtt_reader_remove_wait_reply (conn, packet_id, axl_false);

		/* release packet id */
		/* do not release package id on failure to avoid overwriting packages ids */
		/* __myqtt_conn_release_pkgid (ctx, conn, packet_id); */

		/* only release message if it was stored */
		if (! skip_storage) {
			/* release message */
			myqtt_storage_release_msg (ctx, conn, handle, msg, size);
			handle = NULL; /* nullify handle now we have
					* released message stored */
		} /* end if */

		/* free build */
		/* REALLY IMPORTANT: do not release here because this
		   is already handled by myqtt_sequencer_send */
		/* myqtt_msg_free_build (ctx, msg, size); */

		myqtt_log (MYQTT_LEVEL_CRITICAL, "Unable to queue data for delivery, failed to send publish message");
		return axl_false;
	} /* end if */

	/* now wait here for PUBACK in the case of QoS 1 */
	if ((qos & MYQTT_QOS_1) == MYQTT_QOS_1 && wait_publish > 0) {

		/* wait here for puback reply limiting wait by wait_sub */
		reply = __myqtt_reader_get_reply (conn, packet_id, wait_publish, axl_false);
		if (reply == NULL || reply->type != MYQTT_PUBACK || reply->packet_id != packet_id) {
			myqtt_log (MYQTT_LEVEL_CRITICAL, "Expecting PUBACK, received empty reply, wrong reply type or different packet id in reply=%p (%s) (packet id=%d != %d) conn=%p conn-id=%d, wait_publish=%d",
				   reply, reply ? myqtt_msg_get_type_str (reply) : "UNKNOWN", reply ? reply->packet_id : -1, packet_id, conn, conn->id, wait_publish);
			result = axl_false; /* signal that publication didn't work */
		} /* end if */
		myqtt_msg_unref (reply);
	} /* end if */

	/* now wait here for PUBREC in the case of QoS 2 */
	if ((qos & MYQTT_QOS_2) == MYQTT_QOS_2 && wait_publish > 0) {

		/* wait here for puback reply limiting wait by wait_sub */
		myqtt_log (MYQTT_LEVEL_DEBUG, "Getting PUBREC reply for packet_id=%d conn-id=%d conn=%p", packet_id, conn->id, conn);
		reply = __myqtt_reader_get_reply (conn, packet_id, wait_publish, axl_false);
		if (reply == NULL || reply->type != MYQTT_PUBREC || reply->packet_id != packet_id) {
			myqtt_log (MYQTT_LEVEL_CRITICAL, "Expecting PUBREC, received empty reply (%p), wrong reply type (%s) or different packet id in reply (%d ? %d), wait_publish=%d",
				   reply, reply ? myqtt_msg_get_type_str (reply) : "UNKNOWN", reply ? reply->packet_id : -1, packet_id, wait_publish);
			result = axl_false; /* signal that publication didn't work */
		} /* end if */
		myqtt_log (MYQTT_LEVEL_DEBUG, "Received PUBREC reply for packet_id=%d conn-id=%d conn=%p (ok matches)", packet_id, conn->id, conn);

		/* release reply */
		myqtt_msg_unref (reply);

		/* in any case, remove message from local storage */
		if (! skip_storage) {
			/* release message */
			myqtt_storage_release_msg (ctx, conn, handle, msg, size);
			handle = NULL; /* nullify handle now we have
					* released message stored */
		} /* end if */

		/* check status to not continue in case of failure */
		if (! result) {
			myqtt_log (MYQTT_LEVEL_CRITICAL, "Unable to continue with QoS2 publication, PUBREC failed");

			/* release packet id */
			/* do not release package id on failure to avoid overwriting packages ids */
			/* __myqtt_conn_release_pkgid (ctx, conn, packet_id); */

			return axl_false;
		} /* end if */

		/* ok, so far, so good, now send pubrel, reusing packet id */
		/* build QOS=1/2 message */
		/* dup = axl_false, qos = 1, retain = axl_false */
		msg  = myqtt_msg_build  (ctx, MYQTT_PUBREL, axl_false, MYQTT_QOS_1, axl_false, &size,  /* 2 bytes */
					 /* packet id */
					 MYQTT_PARAM_16BIT_INT, packet_id,
					 MYQTT_PARAM_END);		
		if (msg == NULL || size == 0) {
			myqtt_log (MYQTT_LEVEL_CRITICAL, "Failed to create PUBREL message, empty/NULL value reported by myqtt_msg_build()");

			/* release packet id */
			/* do not release package id on failure to avoid overwriting packages ids */
			/* __myqtt_conn_release_pkgid (ctx, conn, packet_id); */

			return axl_false;
		} /* end if */

		/* prepare reply */
		__myqtt_reader_prepare_wait_reply (conn, packet_id, axl_false);

		/* configure package to send */
		myqtt_log (MYQTT_LEVEL_DEBUG, "Sending PUBREL for packet_id=%d conn-id=%d conn=%p", packet_id, conn->id, conn);
		if (! myqtt_sequencer_send (conn, MYQTT_PUBREL, msg, size)) {
			/* release packet id */
			/* do not release package id on failure to avoid overwriting packages ids */
			/* __myqtt_conn_release_pkgid (ctx, conn, packet_id); */
			
			/* free build */
			/* REALLY IMPORTANT: do not release here because this
			   is already handled by myqtt_sequencer_send */
			/* myqtt_msg_free_build (ctx, msg, size); */

			/* release wait reply queue */
			__myqtt_reader_remove_wait_reply (conn, packet_id, axl_false);
			
			myqtt_log (MYQTT_LEVEL_CRITICAL, "Unable to queue data for delivery (PUBREL), failed to send PUBREL message");
			return axl_false;
		} /* end if */

		/* wait here for puback reply limiting wait by wait_sub */
		myqtt_log (MYQTT_LEVEL_DEBUG, "Getting PUBCOMP reply for packet_id=%d conn-id=%d conn=%p", packet_id, conn->id, conn);
		reply = __myqtt_reader_get_reply (conn, packet_id, wait_publish, axl_false);
		if (reply == NULL || reply->type != MYQTT_PUBCOMP || reply->packet_id != packet_id) {
			myqtt_log (MYQTT_LEVEL_CRITICAL, "Expecting PUBCOMP, received empty reply (%p), wrong reply type (%s) or different packet id in reply (%d != %d), wait_reply=%d",
				   reply, reply ? myqtt_msg_get_type_str (reply) : "UNKNOWN", reply ? reply->packet_id : -1, packet_id, wait_publish);
			result = axl_false; /* signal that publication didn't work */
		} else { 
			myqtt_log (MYQTT_LEVEL_DEBUG, "Received PUBCOMP reply for packet_id=%d conn-id=%d conn=%p (ok matches)", packet_id, conn->id, conn);
		} /* end if */
		myqtt_msg_unref (reply);

	} /* end if */

	/** 
	 * @internal Only release packet_id when publish operation
	 * finishes ok to avoid reusing that packet id which may cause
	 * producing a new operation using same packet-id for a packet
	 * that may clash with the previous one which may be delaying.
	 */
	if (result) {
		/* release packet id */
		__myqtt_conn_release_pkgid (ctx, conn, packet_id);
	} /* end if */

	if (! skip_storage) {
		/* release message */
		myqtt_storage_release_msg (ctx, conn, handle, msg, size);
	} /* end if */
	
	/* report final result */
	return result;
}

/** 
 * @brief Allows to publish a new application message with the
 * provided topic name on the provided connection.
 *
 * The function will publish the application message provided with the QOS indicated. 
 *
 * Optionally, the function allows to wait for publish operation to
 * fully complete blocking the caller until that is done. For \ref
 * MYQTT_QOS_AT_MOST_ONCE (Qos 1) this has no effect. But for \ref
 * MYQTT_QOS_AT_LEAST_ONCE_DELIVERY (QoS 1) it implies waiting for
 * PUBACK control packet to be received. The same applies to \ref
 * MYQTT_QOS_EXACTLY_ONCE_DELIVERY (QoS 2) which implies waiting until
 * PUBCOMP is received.
 *
 * @param conn The connection where the publish operation will take place.
 *
 * @param topic_name The name of the topic for the application message published.
 *
 * @param app_message The application message to publish. From MyQtt's
 * perspective, this is binary data that has a format that is only
 * meaningful to the application on top using MQTT.
 *
 * @param app_message_size The size of the application message. 
 *
 * @param qos The quality of service under which the message will be
 * published. See \ref MyQttQos
 *
 * @param retain Enable message retention for this published
 * message. This way new subscribers will be able to get last
 * published retained message.
 *
 * @param wait_publish Wait for full publication of the message
 * blocking the caller until that happens. If 0 is provided no wait is
 * performed. If some value is provided that will be the max amount of
 * time, in seconds, to wait for complete publication. For \ref
 * MYQTT_QOS_0 this value is ignored. 
 *
 * 
 * @return The function returns axl_true in the case the message was
 * published, otherwise axl_false is returned. The function returns
 * axl_false when the connection received is broken or topic name is
 * NULL or it is bigger than 65535 bytes.
 *
 * <b>A note about closing connection right away after publication</b>
 *
 * Please, note that if you want to close the connection (\ref
 * myqtt_conn_close) just after publication (\ref myqtt_conn_pub) it
 * is recommended to setup a wait for publish (wait_publish param) to
 * ensure the function returns after actually publishing the message
 * and after undergoing with all exchanges that are required according
 * to the provided QoS.
 * 
 */
axl_bool            myqtt_conn_pub             (MyQttConn           * conn,
						const char          * topic_name,
						const axlPointer      app_message,
						int                   app_message_size,
						MyQttQos              qos,
						axl_bool              retain,
						int                   wait_publish)
{
	unsigned char       * msg;
	int                   size;
	MyQttCtx            * ctx;
	int                   packet_id = -1;
	axlPointer            handle = NULL;

	/* skip storage if requested by the caller. */
	axl_bool              skip_storage = (qos & MYQTT_QOS_SKIP_STORAGE) == MYQTT_QOS_SKIP_STORAGE;

	if (conn == NULL || conn->ctx == NULL)
		return axl_false;

	/* get reference to the context */
	ctx = conn->ctx;

	if (! myqtt_conn_is_ok (conn, axl_false)) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Unable to publish, connection received is not working");
		return axl_false;
	} /* end if */

	if (! topic_name || strlen (topic_name) > 65535) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Topic name is bigger than 65535 and this is not allowed by MQTT");
		return axl_false;
	} /* end if */

	/* create message */
	size = 0;

	if (qos == 0) {
		/* build QOS=0 message */
		/* dup = axl_false, qos = 0, retain = <as described by parameter> */
		msg  = myqtt_msg_build  (ctx, MYQTT_PUBLISH, axl_false, 0, retain, &size,  /* 2 bytes */
					 /* topic name */
					 MYQTT_PARAM_UTF8_STRING, strlen (topic_name), topic_name,
					 /* message */
					 MYQTT_PARAM_BINARY_PAYLOAD, app_message_size, app_message,
					 MYQTT_PARAM_END);

		myqtt_log (MYQTT_LEVEL_DEBUG, "Built PUBLISH qos=0 message size=%d packet-id=%d app-message-size=%d conn-id=%d",
			   size, packet_id, app_message_size, conn->id);


	} else if ((qos & MYQTT_QOS_1) == 1 || (qos & MYQTT_QOS_2) == 2) {

		/* get free packet id */
		packet_id = __myqtt_conn_get_next_pkgid (ctx, conn, (qos & MYQTT_QOS_1) == 1 ? 1 : 2);

		/* build QOS=1/2 message */
		/* dup = axl_false, qos = 0, retain = <as described by parameter> */
		msg       = myqtt_msg_build  (ctx, MYQTT_PUBLISH, axl_false, (qos & MYQTT_QOS_1) == 1 ? 1 : 2, retain, &size,  /* 2 bytes */
					      /* topic name */
					      MYQTT_PARAM_UTF8_STRING, strlen (topic_name), topic_name,
					      /* packet id */
					      MYQTT_PARAM_16BIT_INT, packet_id,
					      /* message */
					      MYQTT_PARAM_BINARY_PAYLOAD, app_message_size, app_message,
					      MYQTT_PARAM_END);

		if (msg == NULL || size == 0) {
			myqtt_log (MYQTT_LEVEL_CRITICAL, "Failed to create PUBLISH message, empty/NULL value reported by myqtt_msg_build()");
			return axl_false;
		} /* end if */

		myqtt_log (MYQTT_LEVEL_DEBUG, "Built PUBLISH qos=%d message size=%d packet-id=%d app-message-size=%d conn-id=%d",
			   qos, size, packet_id, app_message_size, conn->id);

		if (! skip_storage) {
			/* store message before attempting to deliver it */
			handle = myqtt_storage_store_msg (ctx, conn, packet_id, (qos & MYQTT_QOS_1) == 1 ? 1 : 2, msg, size);
			if (! handle) {
				/* release packet id */
				__myqtt_conn_release_pkgid (ctx, conn, packet_id);
				
				/* free build */
				myqtt_msg_free_build (ctx, msg, size);
				
				myqtt_log (MYQTT_LEVEL_CRITICAL, "Failed to storage message for publication, unable to continue");
				return axl_false;
			} /* end if */

			myqtt_log (MYQTT_LEVEL_DEBUG, "Stored PUBLISH message size=%d packet-id=%d app-message-size=%d conn-id=%d",
				   size, packet_id, app_message_size, conn->id);

		} /* end if */

	} else {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Wrong QoS value received=%d, unable to publish message", qos);
		return axl_false;
	} /* end if */

	/* call to send message and handle reply */
	return __myqtt_conn_pub_send_and_handle_reply (ctx, conn, packet_id, qos, handle, wait_publish, msg, size);
}

/** 
 * @brief Allows to queue PUBLISH messages on local storage,
 * associated to the provided client identifier, that will be sent
 * once the connection is created after this operation.
 *
 * @param ctx The context where the operation takes place.
 *
 * @param client_identifier The client identifier where the message
 * will be stored. Once a new connection is created with this client
 * identifier, then those messages queued, along those unfinished
 * in-flight messages, will be sent.
 *
 * @param topic_name The name of the topic for the application message published.
 *
 * @param app_message The application message to publish. From MyQtt's
 * perspective, this is binary data that has a format that is only
 * meaningful to the application on top using MQTT.
 *
 * @param app_message_size The size of the application message. 
 *
 * @param qos The quality of service under which the message will be
 * published. See \ref MyQttQos. This function do not support \ref MYQTT_QOS_SKIP_STORAGE. If provided the function will just fail.
 *
 * @param retain Enable message retention for this published
 * message. This way new subscribers will be able to get last
 * published retained message.
 *
 * @return The function returns axl_true in the case the message was
 * queued, otherwise axl_false is returned. The function returns
 * axl_false when the connection received is broken or topic name is
 * NULL or it is bigger than 65535 bytes. The function will also
 * return axl_false when provided \ref MYQTT_QOS_SKIP_STORAGE on qos
 * parameter.
 * 
 * <b>NOTE:</b> you can only queue a maximum of 65356 messages pending
 * to be sent (which is the maximum packet identifier allowed) with
 * QoS1 (\ref MYQTT_QOS_1) or Qos2 (\ref MYQTT_QOS_2). Of messages
 * with QoS0 there's no protocol/API limit (apart from local physical
 * storage available).
 */
axl_bool            myqtt_conn_offline_pub     (MyQttCtx            * ctx,
						const char          * client_identifier,
						const char          * topic_name,
						const axlPointer      app_message,
						int                   app_message_size,
						MyQttQos              qos,
						axl_bool              retain)
{
	int             pkg_id = 1;
	unsigned char * msg;
	int             size;
	axlPointer      handle;


	if (client_identifier == NULL || strlen (client_identifier) == 0) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "offline publish failed (myqtt_conn_offline_pub) because client_identifier=%p (%s) is not defined or it is empty",
			   client_identifier, client_identifier ? client_identifier : "");
		return axl_false;
	}

	if (! topic_name || strlen (topic_name) > 65535) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Topic name is bigger than 65535 and this is not allowed by MQTT");
		return axl_false;
	} /* end if */

	if ((qos & MYQTT_QOS_SKIP_STORAGE) == MYQTT_QOS_SKIP_STORAGE) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Provided skip storage flag for an offline function that in fact needs to store the message...");
		return axl_false;
	} /* end if */

	/* init message for the provide client identifier */
	if (! myqtt_storage_init_offline (ctx, client_identifier, MYQTT_STORAGE_PKGIDS)) {
		/* failed to initialise package ids */
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Failed to initialise pkgids database for the provided client identifier, unable to queue messages");
		return axl_false;
	} /* end if */

	if (qos == MYQTT_QOS_0) {
		pkg_id = 100000;
		while (pkg_id < 200000 && ! myqtt_storage_lock_pkgid_offline (ctx, client_identifier, pkg_id))
			pkg_id++;

		if (pkg_id == 200000) {
			myqtt_log (MYQTT_LEVEL_CRITICAL, "Unable to acquire an unique packet id for queueing the message provided, unable to queue message");
			return axl_false;
		} /* end if */

	} else if (qos == MYQTT_QOS_1 || qos == MYQTT_QOS_2) {
		pkg_id = 1;
		while (pkg_id < 65356 && ! myqtt_storage_lock_pkgid_offline (ctx, client_identifier, pkg_id))
			pkg_id++;

		if (pkg_id == 65536) {
			myqtt_log (MYQTT_LEVEL_CRITICAL, "Unable to acquire an unique packet id for queueing the message provided, unable to queue message");
			return axl_false;
		} /* end if */
	} /* end if */

	myqtt_log (MYQTT_LEVEL_DEBUG, "Queueing message with topic '%s', qos: %d, packet id: %d, client identifier: %s",
		   topic_name, qos, pkg_id, client_identifier);

	/* now build the message */
	switch (qos) {
	case MYQTT_QOS_0:
		/* build message */
		msg  = myqtt_msg_build  (ctx, MYQTT_PUBLISH, axl_false, 0, retain, &size,  /* 2 bytes */
					 /* topic name */
					 MYQTT_PARAM_UTF8_STRING, strlen (topic_name), topic_name,
					 /* message */
					 MYQTT_PARAM_BINARY_PAYLOAD, app_message_size, app_message,
					 MYQTT_PARAM_END);
		break;
	case MYQTT_QOS_1:
	case MYQTT_QOS_2:
		/* build message */
		msg       = myqtt_msg_build  (ctx, MYQTT_PUBLISH, axl_false, qos, retain, &size,  /* 2 bytes */
					      /* topic name */
					      MYQTT_PARAM_UTF8_STRING, strlen (topic_name), topic_name,
					      /* packet id */
					      MYQTT_PARAM_16BIT_INT, pkg_id,
					      /* message */
					      MYQTT_PARAM_BINARY_PAYLOAD, app_message_size, app_message,
					      MYQTT_PARAM_END);
		break;
	default:
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Wrong QoS value received=%d, unable to publish message", qos);
		return axl_false;
	}

	/* check reported values */
	if (msg == NULL || size == 0) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Failed to queue PUBLISH message, empty/NULL value reported by myqtt_msg_build()");
		return axl_false;
	} /* end if */

	/* now queue message */
	handle = myqtt_storage_store_msg_offline (ctx, client_identifier, pkg_id, qos, msg, size);
	if (! handle) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Unable to storage message offline, myqtt_storage_store_msg_offline() failed");

		/* release pkgid now it is not going to be used */
		myqtt_storage_release_pkgid_offline (ctx, client_identifier, pkg_id);

		/* free message now it is stored */
		myqtt_msg_free_build (ctx, msg, size);

		/* release handle */
		axl_free (handle);

		return axl_false;
	} /* end if */

	/* free message now it is stored */
	myqtt_msg_free_build (ctx, msg, size);

	myqtt_log (MYQTT_LEVEL_DEBUG, "Offline publish message stored at: %s for client_identifier=%s",
		   handle, client_identifier);

	/* release handle */
	axl_free (handle);

	/* message queued */
	return axl_true;
}


/**  
 * @brief Allows to subscribe to one topic.
 *
 * @param conn The connection where the operation will take place.
 *
 * @param topic_filter Topic filter to subscribe to.
 *
 * @param wait_sub How long to wait for subscription operation to complete. Value provided must be in microseconds: 10 seconds -> 10000000 If 0 is provided, no wait operation will be implemented. If -1 is provided, a default wait of 10 seconds will be implemented (10000000).
 * 
 * @param qos Requested QoS, maximum QoS level at which the server can
 * send publications to this client. Note that this is a request that
 * must be granted by the server. Use subs_result param to get an
 * indication about the QoS that was finally granted.
 *
 * @param subs_result Reference to a integer value where the function
 * will report subscription result. If subs_result is NULL, no
 * subscription result will be reported. In the case subs_result is
 * provided, it can either -1 (indicating that subscription was
 * rejected/denied by the server or there was any other error) or any
 * value of \ref MyQttQos.
 *
 * @return The function reports axl_true in the case the subscription
 * request was completed without any error. In the case of a
 * connection failure, subscription failure or timeout, the function
 * will report axl_false. The function also reports axl_false in the
 * case conn and any topic_filter provided is NULL.
 *
 * <b>About QoS to use for subscriptions: </b>
 *
 * From OASIS Standard definition: "The requested QoS gives the
 * maximum QoS level at which the Server can send publications to the
 * Client (3.4.3 Payload section).  
 *
 *
 * When you subscribe with a certain QoS to the broker you are telling
 * the server to send you all publish operations using that provided
 * QoS level. This has the direct implication that all guaranties (or
 * none) will be applied according to the QoS you select. 
 *
 * If a you receive a message that was published using QoS 2 but you
 * subscribed using QoS 1, then the message QoS will be downgraded to
 * QoS 1 (for this particular subscription). 
 *
 * - Use \ref MYQTT_QOS_0 in the case you don't care about missing some messages you can use
 * \ref MYQTT_QOS_0 which means that any message published to the
 * subscribed topic will be sent to you with QoS 0 (even though it was published with QoS 1 o 2). 
 *
 * - Use \ref MYQTT_QOS_1 in the case you want message retention while you are disconnected
 * (don't forget to setup clean_session to axl_false when connecting)
 * you can use \ref MYQTT_QOS_1. This will make the broker to send you
 * messages published with QoS 1, ensuring that the message reaches
 * your client at least once (but be aware you can receive duplicates, see \ref myqtt_msg_get_dup_flag).
 *
 * - Use \ref MYQTT_QOS_2 in the case you want all guaranties provided
 *  by MQTT protocol and you want to receive all messages and only one
 *  time. This is the highest level provided by MQTT which ensures all
 *  messages are delivered. This is recommended for critical
 *  applications where no message can be lost.  However, it is the QoS
 *  that causes more overhead because it needs to store and confirm
 *  every message on every step. Note that if you set a QoS of 2, it doesn't imply that a published message with QoS 0 will be upgraded to QoS 2.
 *
 * As you can see these is a different process when selecting the QOS
 * when publishing (\ref myqtt_conn_pub). That is, when sending
 * publications to the broker there is a configurable QoS and when
 * receiving those publications there is also another QoS configuration.
 *
 * <b>No upgrade of QoS happens (as opposed to downgrade described):</b>
 *
 * Keep in mind that an upgrade of QoS never happens (as opposed to the
 * downgrade when subscribing with a QoS that is lower than message's
 * QoS). 
 * 
 * This means that subscribing with QoS 2 ensures that you receive QoS
 * 2 published message as is, but for those messages having QoS 1 and
 * QoS 0 will be received as is too (no upgrade happens from 0 to 2 or
 * from 1 to 2).
 * 
 * That's why the standard says: Subscribing to a Topic Filter at QoS
 * 2 is equivalent to saying "I would like to receive Messages
 * matching this filter at the QoS with which they were
 * published". This means a publisher is responsible for determining
 * the maximum QoS a Message can be delivered at, but a subscriber is
 * able to require that the Server downgrades the QoS to one more
 * suitable for its usage.
 *
 *
 */
axl_bool            myqtt_conn_sub             (MyQttConn           * conn,
						int                   wait_sub,
						const char          * topic_filter,
						MyQttQos              qos,
						int                 * subs_result) 
{
	unsigned char       * msg;
	int                   size;
	MyQttCtx            * ctx;
	MyQttSequencerData  * data;
	MyQttMsg            * reply;
	int                   packet_id;
	int                   _subs_result;

	if (conn == NULL || conn->ctx == NULL)
		return axl_false;

	/* get reference to the context */
	ctx = conn->ctx;

	if (! myqtt_conn_is_ok (conn, axl_false)) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Unable to publish, connection received is not working");
		return axl_false;
	} /* end if */

	if (! topic_filter || strlen (topic_filter) > 65535) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Topic filter is bigger than 65535 and this is not allowed by MQTT");
		return axl_false;
	} /* end if */

	/* create message */
	size = 0;

	/* generate a packet id */
	packet_id = __myqtt_conn_get_next_pkgid (ctx, conn, -1);

	/* build SUBSCRIBE message */
	/* dup = axl_false, qos = 1, retain = axl_false */
	msg  = myqtt_msg_build  (ctx, MYQTT_SUBSCRIBE, axl_false, 1, axl_false, &size,  /* 2 bytes */
				 /* packet id */
				 MYQTT_PARAM_16BIT_INT, packet_id,
				 /* topic name */
				 MYQTT_PARAM_UTF8_STRING, strlen (topic_filter), topic_filter,
				 /* qos */
				 MYQTT_PARAM_8BIT_INT, qos,
				 MYQTT_PARAM_END);

	if (msg == NULL || size == 0) {
		/* release pkgid */
		__myqtt_conn_release_pkgid (ctx, conn, packet_id);

		myqtt_log (MYQTT_LEVEL_CRITICAL, "Failed to create SUBSCRIBE message, empty/NULL value reported by myqtt_msg_build()");
		return axl_false;
	} /* end if */

	/* queue package to be sent */
	data = axl_new (MyQttSequencerData, 1);
	if (data == NULL) {
		/* release pkgid */
		__myqtt_conn_release_pkgid (ctx, conn, packet_id);

		/* free build */
		myqtt_msg_free_build (ctx, msg, size);

		myqtt_log (MYQTT_LEVEL_CRITICAL, "Failed to acquire memory to send message, unable to subscribe");
		return axl_false;
	} /* end if */

	/* prepare wait reply */
	myqtt_log (MYQTT_LEVEL_DEBUG, "Creating wait reply for conn %p, packet id: %d", conn, packet_id);
	__myqtt_reader_prepare_wait_reply (conn, packet_id, axl_false);

	/* configure package to send */
	data->conn         = conn;
	data->message      = msg;
	data->message_size = size;
	data->type         = MYQTT_SUBSCRIBE;

	if (! myqtt_sequencer_queue_data (ctx, data)) {
		/* release pkgid */
		__myqtt_conn_release_pkgid (ctx, conn, packet_id);

		/* release wait reply queue */
		__myqtt_reader_remove_wait_reply (conn, packet_id, axl_false);

		myqtt_log (MYQTT_LEVEL_CRITICAL, "Unable to queue data for delivery, failed to send publish message");
		return axl_false;
	} /* end if */

	myqtt_log (MYQTT_LEVEL_DEBUG, "SUBSCRIBE to %s sent (packet id=%d, conn-id=%d), waiting reply (%d secs)..", topic_filter, packet_id, conn->id, wait_sub);

	/* configure default wait_sub */
	if (wait_sub == -1)
		wait_sub = 10000000;

	/* wait here for suback reply limiting wait by wait_sub */
	reply = __myqtt_reader_get_reply (conn, packet_id, wait_sub, axl_false);
	if (reply == NULL) {
		/* release pkgid */
		__myqtt_conn_release_pkgid (ctx, conn, packet_id);

		myqtt_log (MYQTT_LEVEL_CRITICAL, "Received NULL reply while waiting for reply (SUBACK) to SUBSCRIBE request");
		return axl_false;
	} /* end if */

	/* handle reply */
	if (reply->type != MYQTT_SUBACK) {
		/* release pkgid */
		__myqtt_conn_release_pkgid (ctx, conn, packet_id);

		myqtt_log (MYQTT_LEVEL_CRITICAL, "Expected SUBACK reply but found: %s", myqtt_msg_get_type_str (reply));
		myqtt_msg_unref (reply);
		return axl_false;
	} /* end if */

	if (reply->packet_id != packet_id) {
		/* release pkgid */
		__myqtt_conn_release_pkgid (ctx, conn, packet_id);

		myqtt_log (MYQTT_LEVEL_CRITICAL, "Expected to receive packet id = %d in reply but found %d", 
			   reply->packet_id, packet_id);
		myqtt_msg_unref (reply);
		return axl_false;
	} /* end if */

	/* get subscription result */
	_subs_result = myqtt_get_8bit (reply->payload);
	myqtt_msg_unref (reply);

	if (subs_result)
		(*subs_result) = _subs_result;

	if (_subs_result == 128) {
		myqtt_log (MYQTT_LEVEL_WARNING, "SUBSCRIBE reply was = %d (denied/error) (packet id=%d, conn-id=%d)", _subs_result, packet_id, conn->id);

		/* release pkgid */
		__myqtt_conn_release_pkgid (ctx, conn, packet_id);

		return axl_false;
	} /* end if */

	myqtt_log (MYQTT_LEVEL_DEBUG, "SUBSCRIBE reply was = %d (packet id=%d, conn-id=%d)", _subs_result, packet_id, conn->id);

	/* release pkgid */
	__myqtt_conn_release_pkgid (ctx, conn, packet_id);
	
	/* report final result */
	return axl_true;
}

/** 
 * @brief Allows to unsubscribe the given particular topic filter on
 * the provided connection.
 *
 * @param conn The connection where the unsubscribe operation will take place.
 *
 * @param topic_filter The topic filter to unsubscribe (that was
 * subscribed previously using \ref myqtt_conn_pub).
 *
 * @param wait_unsub Wait for unsubscribe reply blocking the caller
 * until that happens. If 0 is provided no wait is performed. If some
 * value is provided that will be the max amount of time, in seconds,
 * to wait for unsubscribe reply.
 *
 * @return The function reports axl_true in the case the unsubscribe
 * request was completed without any error. In the case of a
 * connection failure, unsubscribe timeout, the function
 * will report axl_false. The function also reports axl_false in the
 * case conn and any topic_filter provided is NULL.
 */
axl_bool            myqtt_conn_unsub           (MyQttConn           * conn,
						const char          * topic_filter,
						int                   wait_unsub)
{
	unsigned char       * msg;
	int                   size;
	MyQttCtx            * ctx;
	MyQttMsg            * reply;
	int                   packet_id;

	if (conn == NULL || conn->ctx == NULL)
		return axl_false;

	/* get reference to the context */
	ctx = conn->ctx;

	if (! myqtt_conn_is_ok (conn, axl_false)) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Unable to publish, connection received is not working");
		return axl_false;
	} /* end if */

	if (! topic_filter || strlen (topic_filter) > 65535) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Topic filter is bigger than 65535 and this is not allowed by MQTT");
		return axl_false;
	} /* end if */

	/* create message */
	size = 0;

	/* generate a packet id */
	packet_id = __myqtt_conn_get_next_pkgid (ctx, conn, -1);

	/* build UNSUBSCRIBE message */
	/* dup = axl_false, qos = 0, retain = axl_false */
	msg  = myqtt_msg_build  (ctx, MYQTT_UNSUBSCRIBE, axl_false, MYQTT_QOS_0, axl_false, &size,  /* 2 bytes */
				 /* packet id */
				 MYQTT_PARAM_16BIT_INT, packet_id,
				 /* topic name */
				 MYQTT_PARAM_UTF8_STRING, strlen (topic_filter), topic_filter,
				 MYQTT_PARAM_END);

	if (msg == NULL || size == 0) {
		/* release packet id */
		__myqtt_conn_release_pkgid (ctx, conn, packet_id);

		myqtt_log (MYQTT_LEVEL_CRITICAL, "Failed to create UNSUBSCRIBE message, empty/NULL value reported by myqtt_msg_build()");
		return axl_false;
	} /* end if */

	if (wait_unsub > 0) {
		/* prepare wait reply */
		__myqtt_reader_prepare_wait_reply (conn, packet_id, axl_false);
	} /* end if */

	/* queue package to be sent */
	if (! myqtt_sequencer_send (conn, MYQTT_UNSUBSCRIBE, msg, size)) {
		/* release packet id */
		__myqtt_conn_release_pkgid (ctx, conn, packet_id);

		/* free build */
		/* REALLY IMPORTANT: do not release here because this
		   is already handled by myqtt_sequencer_send */
		/* myqtt_msg_free_build (ctx, msg, size); */

		/* release wait reply queue */
		__myqtt_reader_remove_wait_reply (conn, packet_id, axl_false);

		myqtt_log (MYQTT_LEVEL_CRITICAL, "Failed to acquire memory to send message, unable to subscribe");
		return axl_false;
	} /* end if */

	myqtt_log (MYQTT_LEVEL_DEBUG, "UNSUBSCRIBE for %s sent (packet id=%d, conn-id=%d), waiting reply (%d secs)..", topic_filter, packet_id, conn->id, wait_unsub);

	/* check if caller didn't request to wait for reply */
	if (wait_unsub == 0) {
		/* release packet id */
		__myqtt_conn_release_pkgid (ctx, conn, packet_id);
		return axl_true;
	} /* end if */

	/* wait here for suback reply limiting wait by wait_sub */
	reply = __myqtt_reader_get_reply (conn, packet_id, wait_unsub, axl_false);
	if (reply == NULL) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Received NULL reply while waiting for reply (UNSUBACK) to UNSUBSCRIBE request");

		/* release packet id */
		__myqtt_conn_release_pkgid (ctx, conn, packet_id);

		return axl_false;
	} /* end if */

	/* handle reply */
	if (reply->type != MYQTT_UNSUBACK) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Expected UNSUBACK reply but found: %s", myqtt_msg_get_type_str (reply));
		myqtt_msg_unref (reply);

		/* release packet id */
		__myqtt_conn_release_pkgid (ctx, conn, packet_id);

		return axl_false;
	} /* end if */

	if (reply->packet_id != packet_id) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Expected to receive packet id = %d in reply but found %d", 
			   reply->packet_id, packet_id);
		myqtt_msg_unref (reply);

		/* release packet id */
		__myqtt_conn_release_pkgid (ctx, conn, packet_id);

		return axl_false;
	} /* end if */

	myqtt_log (MYQTT_LEVEL_DEBUG, "UNSUBSCRIBE reply received (packet id=%d, conn-id=%d)", packet_id, conn->id);
	myqtt_msg_unref (reply);

	/* release packet id */
	__myqtt_conn_release_pkgid (ctx, conn, packet_id);
	
	/* report final result */
	return axl_true;
}

/** 
 * @brief Allows to send a ping request (PINGREQ) to the server over
 * the provided connection.
 *
 * The function handles server reply (PINGRESP) and allows to limit
 * reply wait time to the value provided by wait_pingresp. 
 *
 * @param conn The connection where the PINGREQ will be sent.
 *
 * @param wait_pingresp How many seconds to wait for PINGRESP reply to
 * come.
 *
 * @return axl_true if the PINGREQ operation went ok and a PINGRESP
 * was received. Otherwise, axl_false is returned. Remember you can
 * always use \ref myqtt_conn_is_ok to check if the connection is
 * connected and working. Alternatively you can use \ref
 * myqtt_conn_set_on_close to get a notification just when the
 * connection is lost.
 * 
 */
axl_bool            myqtt_conn_ping            (MyQttConn           * conn,
						int                   wait_pingresp)
{
	unsigned char       * msg;
	int                   size = 0;
	MyQttCtx            * ctx;
	MyQttMsg            * reply;

	if (conn == NULL || conn->ctx == NULL)
		return axl_false;

	/* get reference to the context */
	ctx = conn->ctx;

	if (! myqtt_conn_is_ok (conn, axl_false)) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Unable to ping server, connection received is not working");
		return axl_false;
	} /* end if */

	/* build UNSUBSCRIBE message */
	/* dup = axl_false, qos = 0, retain = axl_false */
	msg  = myqtt_msg_build  (ctx, MYQTT_PINGREQ, axl_false, MYQTT_QOS_0, axl_false, &size,  /* 2 bytes */
				 MYQTT_PARAM_END);

	if (msg == NULL || size == 0) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Failed to create UNSUBSCRIBE message, empty/NULL value reported by myqtt_msg_build()");
		return axl_false;
	} /* end if */

	if (wait_pingresp > 0) {
		/* ensure waiting queue is in place so we can make
		 * this part thread safe */
		myqtt_mutex_lock (&conn->op_mutex);
		if (conn->ping_resp_queue == NULL)
			conn->ping_resp_queue = myqtt_async_queue_new ();
		myqtt_mutex_unlock (&conn->op_mutex);
	} /* end if */

	/* queue package to be sent */
	if (! myqtt_sequencer_send (conn, MYQTT_PINGREQ, msg, size)) {
		/* free build */
		/* REALLY IMPORTANT: do not release here because this
		   is already handled by myqtt_sequencer_send */
		/* myqtt_msg_free_build (ctx, msg, size); */

		myqtt_log (MYQTT_LEVEL_CRITICAL, "Failed to acquire memory to send message, unable to subscribe");
		return axl_false;
	} /* end if */

	myqtt_log (MYQTT_LEVEL_DEBUG, "PINGREQ  sent (conn-id=%d), waiting reply (%d secs)..", conn->id, wait_pingresp);

	/* check if caller didn't request to wait for reply */
	if (wait_pingresp == 0) 
		return axl_true;

	/* wait here for suback reply limiting wait by wait_sub */
	reply = myqtt_async_queue_timedpop (conn->ping_resp_queue, wait_pingresp * 1000000);
	if (reply == NULL) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Received NULL reply while waiting for reply (PINGRESP) to PINGREQ request");
		return axl_false;
	} /* end if */

	/* handle reply */
	if (reply->type != MYQTT_PINGRESP) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Expected PINGRESP reply but found: %s", myqtt_msg_get_type_str (reply));
		myqtt_msg_unref (reply);
		return axl_false;
	} /* end if */

	myqtt_log (MYQTT_LEVEL_DEBUG, "PINGRESP reply received (conn-id=%d)", conn->id);
	myqtt_msg_unref (reply);
	
	/* report final result */
	return axl_true;
}

/** 
 * @brief Allows to cleanly close the connection by sending the
 * DISCONNECT control packet.
 *
 * After the connection is closed it must not be used. If no other
 * references are acquired to the connection, it is then released
 * (resources).
 *
 * In the case the connection is a listener or a master listener, it
 * is just closed without sending DISCONNECT message.
 *
 * @param connection The connection to be closed. 
 *
 * @return axl_true if the connection was closed, otherwise axl_false
 * is returned. 
 */
axl_bool                    myqtt_conn_close                  (MyQttConn * connection)
{
	int             refcount = 0;
	unsigned char * msg;
	int             size;
	int             timeout;

#if defined(ENABLE_MYQTT_LOG)
	MyQttCtx * ctx;
#endif

	/* if the connection is a null reference, just return axl_true */
	if (connection == NULL)
		return axl_true;

#if defined(ENABLE_MYQTT_LOG)
	/* get a reference to the ctx */
	ctx = connection->ctx;
#endif
	/* ensure only one call to myqtt_conn_close will
	   progress */
	myqtt_mutex_lock (&connection->op_mutex);
	if (connection->close_called) {
		myqtt_mutex_unlock (&connection->op_mutex);
		return axl_true;
	}
	/* flag as connection close called */
	connection->close_called = axl_true;
	myqtt_mutex_unlock (&connection->op_mutex);

	/* get timeout to limit close operation internally */
	timeout = myqtt_conn_get_timeout (ctx);

	/* release connection options if they were configured having
	   reconnect enabled and reuse disabled */
	if (connection && connection->opts) {
		if (! connection->opts->reuse && connection->opts->reconnect) {
			myqtt_conn_opts_free (connection->opts);
			connection->opts = NULL;
		} /* end if */
	}

	/* ensure that all messages were sent before closing the
	 * connection in the case it is found to be working */
	while (timeout > 0 && connection->sequencer_messages > 0 && myqtt_conn_is_ok (connection, axl_false)) {

		if (connection->sequencer_messages > 0) {

			/* still pending messages, wait a little bit */
			timeout = timeout - 10000;
			myqtt_sleep (10000);
			

			continue;
		}
		break; /* messages pending are zero */
	}

	/** ensure that all wait and peer replies are satisfied ***/
	while (timeout > 0 && myqtt_conn_is_ok (connection, axl_false)) {
		if (axl_hash_items (connection->wait_replies) > 0 || axl_hash_items (connection->peer_wait_replies) > 0) {
			/* still pending messages, wait a little bit */
			timeout = timeout - 10000;
			myqtt_sleep (10000);
			continue;
		} /* end if */

		break; /* reached this point no wait replies or peer wait replies there */
	} /* end while */
	
	/* close this connection */
	if (myqtt_conn_is_ok (connection, axl_false) && connection->role == MyQttRoleInitiator) {
		myqtt_log (MYQTT_LEVEL_DEBUG, "closing a connection id=%d", connection->id);

		/* send disconnect message */
		size = 0;
		/* dup = axl_false, qos = 0, retain = axl_false */
		msg  = myqtt_msg_build  (ctx, MYQTT_DISCONNECT, axl_false, 0, axl_false, &size,  /* 2 bytes */
					 /* no variable header and no payload */
					 MYQTT_PARAM_END);
		if (msg == NULL || size == 0) {
			myqtt_log (MYQTT_LEVEL_CRITICAL, "Failed to create DISCONNECT message, empty/NULL value reported by myqtt_msg_build()");
			return axl_false;
		} /* end if */


		/* update the connection reference to avoid race
		 * conditions caused by de-allocations */
		if (! myqtt_conn_ref (connection, "myqtt_conn_close")) {

			__myqtt_conn_shutdown_and_record_error (
				connection, MyQttError,
				"failed to update reference counting on the connection during close operation, skipping clean close operation..");
			myqtt_conn_unref (connection, "myqtt_conn_close");
			return axl_true;
		} /* end if */

		/* send DISCONNECT message */
		if (! myqtt_msg_send_raw (connection, msg, size)) {
			myqtt_log (MYQTT_LEVEL_CRITICAL, "Failed to send DISCONNECT message, errno=%d", errno);

			/* don't release anything because we are not
			 * waiting for reply, just notify the
			 * server */
		} /* end if */
	
		/* report message sent */
		myqtt_log (MYQTT_LEVEL_DEBUG, "DISCONNECT message of %d bytes sent", size);

		/* call to release */
		myqtt_msg_free_build (ctx, msg, size);

		/* set the connection to be not connected */
		__myqtt_conn_shutdown_and_record_error (
			connection, MyQttConnectionCloseCalled, "close connection called");

		/* update the connection reference to avoid race
		 * conditions caused by de-allocations */
		refcount = connection->ref_count;
		myqtt_conn_unref (connection, "myqtt_conn_close");

		/* check special case where the caller have stopped a
		 * listener reference without taking a reference to it */
		if ((refcount - 1) == 0) {
			return axl_true;
		}
	} else {
		myqtt_log (MYQTT_LEVEL_DEBUG, "closing a connection which is not opened, unref resources..");
		/* check here for the case when a close operation is
		   requested for a listener that was started but not
		   created (port binding problem, port bind
		   permission, etc). This is allowed because under
		   such cases, myqtt reader do not own a reference to
		   the listener and it is required to remove it,
		   allowing to do so by the user. */
		if (connection->role == MyQttRoleMasterListener)  {
			myqtt_conn_unref (connection, "myqtt_conn_close");
			return axl_true;
		}
	}

 	/* unref myqtt connection resources, but check first this is
 	 * not a listener connection, which is already owned by the
 	 * myqtt engine */
 	if (connection->role == MyQttRoleInitiator) {
 		myqtt_log (MYQTT_LEVEL_DEBUG, "connection close finished, now unrefering (refcount=%d)..", 
 			    connection->ref_count);
 		myqtt_conn_unref (connection, "myqtt_conn_close");
 	} /* end if */

	return axl_true;
}

/** 
 * @brief Allows to create an empty connection options. 
 *
 * See the following functions to handle the object before passing it
 * to any of the connect functions (\ref myqtt_conn_new).
 *
 * - \ref myqtt_conn_opts_set_auth
 * - \ref myqtt_conn_opts_set_reuse
 * - \ref myqtt_conn_opts_set_will
 *
 * @return The function returns a newly created and empty connection
 * options object or NULL if it fails. 
 *
 * IMPORTANT NOTE: You don't have to release it by default because
 * this is automatically done by the connecting function (for example
 * \ref myqtt_conn_new). In the case you want to reuse this connection
 * options object across several calls, please call \ref
 * myqtt_conn_opts_set_reuse. After you are done with those connection
 * options, you can release it by calling to \ref myqtt_conn_opts_free.
 */
MyQttConnOpts     * myqtt_conn_opts_new (void) {
	/* create an empty connection option object */
	return axl_new (MyQttConnOpts, 1);
}

/** 
 * @brief Allows to configure auth options to be used by the
 * connection function. Any previous configuration 
 *
 * @param opts The connection options where the operation will take place. It cannot be NULL.
 *
 * @param username The username to be configured on the connection. If
 * the value provide is NULL, all authentication info will be removed
 * from the connection options.
 *
 * @param password The password to be configured on the connection.
 *
 */
void                myqtt_conn_opts_set_auth (MyQttConnOpts * opts, 
					      const    char * username, 
					      const    char * password)
{
	if (opts == NULL)
		return;
	/* username */
	if (opts->username) {
		axl_free (opts->username);
		opts->username = NULL;
	} /* end if */

	/* password */
	if (opts->password) {
		axl_free (opts->password);
		opts->password = NULL;
	} /* end if */

	/* copy values from user space */
	if (username) {
		opts->username = axl_strdup (username);
		/* only use name enables use_auth: that way we support
		   user password scheme and just user  */
		opts->use_auth = axl_true;
	} /* end if */
	if (password) 
		opts->password = axl_strdup (password);

	return;
}

/** 
 * @brief Allows to configure the reuse option. By default connection
 * options aren't used across connection calls. In the case you want
 * to do so, call this function passing reuse=axl_true.
 *
 * @param opts The connection option where the operation will take place.
 *
 * @param reuse The reuse option to configure.
 */
void                myqtt_conn_opts_set_reuse (MyQttConnOpts * opts,
					       axl_bool        reuse)
{
	if (opts == NULL)
		return;
	opts->reuse = reuse;
	return;
}

/** 
 * @brief Allows to configure Will options on the provided connection options.
 *
 * @param opts The connection option where the operation will take place.
 *
 * @param will_qos Qos to be configured in the Will.
 *
 * @param will_topic The Will topic where the will message will be
 * posted on abnormal connection. This value cannot be NULL or empty.
 *
 * @param will_message The Will message that will be posted. This
 * value cannot be NULL but can be empty.
 *
 * @param will_retain The Will retain flag configured.
 *
 */
void                myqtt_conn_opts_set_will (MyQttConnOpts  * opts,
					      MyQttQos         will_qos,
					      const char     * will_topic,
					      const char     * will_message,
					      axl_bool         will_retain)
{
	if (opts == NULL || will_topic == NULL || will_message == NULL)
		return;

	/* setup will topic */
	if (opts->will_topic)
		axl_free (opts->will_topic);
	opts->will_topic = axl_strdup (will_topic);

	/* setup will message */
	if (opts->will_message)
		axl_free (opts->will_message);
	opts->will_message = axl_strdup (will_message);

	/* set retain flag and qos */
	opts->will_retain  = will_retain;
	opts->will_qos     = will_qos;

	return;
}

/** 
 * @brief Allows to set/unset reconnect flag on the provided
 * connection options object.
 *
 * See also \ref myqtt_conn_set_on_reconnect to get a notification
 * when a reconnect operation takes place and finishes without error.
 *
 * @param opts The connection options where the configuration is done.
 *
 * @param enable_reconnect axl_true to enable reconnect every time a
 * connection is closed, axl_false to disable this mechanism.
 *
 * SECURITY NOTE: keep in mind that using this option along with
 * user/password at \ref myqtt_conn_opts_set_auth will make the
 * library to keep the password in memory. Otherwise, practical
 * reconnection is not possible because that value (the password) is
 * needed to successfully complete it.
 */
void                myqtt_conn_opts_set_reconnect (MyQttConnOpts * opts,
						   axl_bool        enable_reconnect)
{
	if (! opts)
		return;
	/* set reconnect flag */
	opts->reconnect = enable_reconnect;
	return;
}

/** 
 * @brief Allows to configure a session setup ptr init function
 * used by the session setup handler to implement reconnect
 * operations.
 *
 * For MQTT and MQTT-TLS protocols this function is not
 * required. However, for MQTT-WS, it is required additional
 * information to init the session, and in particular, it is
 * required a function that allows to restore the noPollConn reference
 * used by the \ref MyQttSessionSetup handler.
 *
 * If you don't provide these session init function, myqtt-web-socket module will
 * disable the reconnect option if requested the user.
 *
 * Please note that this function is not required if you are not
 * attempting to implement reconnect support on connection close.
 *
 * The handler here configured (the init_session_setup_ptr) is called just before calling
 * session setup to refresh internal pointer passed in into the \ref
 * MyQttSessionSetup so session setup receives a newly created object.
 *
 * @param opts The connection options to configure.
 *
 * @param init_session_setup_ptr Recover handler to call to refresh/regenerate user land pointer passed in into the session setup.
 *
 * @param user_data User defined pointer passed in into the recover handler.
 *
 * @param user_data2 Second user defined pointer passed in into the recover handler.
 *
 * @param user_data3 Third user defined pointer passed in into the recover handler.
 */
void                myqtt_conn_opts_set_init_session_setup_ptr (MyQttConnOpts               * opts,
								MyQttInitSessionSetupPtr      init_session_setup_ptr,
								axlPointer                    user_data,
								axlPointer                    user_data2,
								axlPointer                    user_data3)
{
	if (! opts)
		return;

	/* configure data */
	opts->init_session_setup_ptr   = init_session_setup_ptr;
	opts->init_user_data           = user_data;
	opts->init_user_data2          = user_data2;
	opts->init_user_data3          = user_data3;
	return;
}

/** 
 * @brief Releases the connection object created by \ref myqtt_conn_opts_new.
 *
 * In general you don't have to call this function unless you called
 * \ref myqtt_conn_opts_set_reuse to reuse the connection object
 * across calls.
 *
 * @param opts The object to release.
 *
 */
void                myqtt_conn_opts_free     (MyQttConnOpts  * opts)
{
	if (opts == NULL)
		return;

	/* release user and password (if present) */
	axl_free (opts->username);
	axl_free (opts->password);

	/* release will configuration (if present) */
	axl_free (opts->will_topic);
	axl_free (opts->will_message);

	/* release certificate info */
	axl_free (opts->certificate);
	axl_free (opts->private_key);
	axl_free (opts->chain_certificate);
	axl_free (opts->ca_certificate);
	axl_free (opts->serverName);

	axl_free (opts);
	return;
}


/** 
 * @internal Reference counting update implementation.
 */
axl_bool               myqtt_conn_ref_internal                    (MyQttConn * connection, 
								   const char       * who,
								   axl_bool           check_ref)
{
#if defined(ENABLE_MYQTT_LOG)
	MyQttCtx * ctx;
#endif
	int        ref_count;

	v_return_val_if_fail (connection, axl_false);
	if (check_ref)
		v_return_val_if_fail (myqtt_conn_is_ok (connection, axl_false), axl_false);

#if defined(ENABLE_MYQTT_LOG)
	/* get a reference to the ctx */
	ctx = connection->ctx;
#endif
	
	/* lock ref/unref operations over this connection */
	myqtt_mutex_lock   (&connection->ref_mutex);

	/* increase and log the connection increased */
	connection->ref_count++;

	ref_count = connection->ref_count;

	myqtt_log (MYQTT_LEVEL_DEBUG, "%d increased connection id=%d (%p) reference to %d by %s\n",
		   myqtt_getpid (),
		   connection->id, connection,
		   ref_count, who ? who : "??"); 

	/* unlock ref/unref options over this connection */
	myqtt_mutex_unlock (&connection->ref_mutex);

	return ref_count > 1;
}

/** 
 * @brief Increase internal myqtt connection reference counting.
 * 
 * Because MyQtt Library design, several on going threads shares
 * references to the same connection for several purposes. 
 * 
 * Conn reference counting allows to every on going thread to notify
 * the system that connection reference is no longer in use so, if the
 * reference counting reaches zero, connection resources will be
 * collected.
 *
 * Keep in mind that using this function also implies to use \ref
 * myqtt_conn_unref . For each call to \ref myqtt_conn_ref it should
 * exist a call to \ref myqtt_conn_unref. Failing to do so will cause
 * either memory leak or memory corruption because improper connection
 * collection.
 * 
 * The function return axl_true to signal that the connection reference
 * count was increased in one unit. If the function return axl_false, the
 * connection reference count wasn't increased and a call to
 * myqtt_conn_unref should not be done. Here is an example:
 * 
 * \code
 * // try to ref the connection
 * if (! myqtt_conn_ref (connection, "some known module or file")) {
 *    // unable to ref the connection
 *    return;
 * }
 *
 * // connection referenced, do work 
 *
 * // finally unref the connection
 * myqtt_conn_unref (connection, "some known module or file");
 * \endcode
 *
 * @param connection the connection to operate.
 * @param who who have increased the reference.
 *
 * @return axl_true if the connection reference was increased or axl_false if
 * an error was found.
 */
axl_bool               myqtt_conn_ref                    (MyQttConn * connection, 
								 const char       * who)
{
	/* checked ref */
	return myqtt_conn_ref_internal (connection, who, axl_true);
}

/** 
 * @brief Allows to perform a ref count operation on the connection
 * provided without checking if the connection is working (no call to
 * \ref myqtt_conn_is_ok).
 *
 * @param connection The connection to update.

 * @return axl_true if the reference update operation is completed,
 * otherwise axl_false is returned.
 */
axl_bool               myqtt_conn_uncheck_ref           (MyQttConn * connection)
{
	/* unchecked ref */
	return myqtt_conn_ref_internal (connection, "unchecked", axl_false);
}

/** 
 * @brief Decrease myqtt connection reference counting.
 *
 * Allows to decrease connection reference counting. If this reference
 * counting goes under 0 the connection resources will be de-allocated. 
 *
 * See also \ref myqtt_conn_ref
 * 
 * @param connection The connection to operate.
 * @param who        Who have decreased the reference. This is a string value used to log which entity have decreased the connection counting.
 */
void               myqtt_conn_unref                  (MyQttConn * connection, 
							     char const       * who)
{

#if defined(ENABLE_MYQTT_LOG)
	MyQttCtx  * ctx;
#endif
	int          count;

	/* do not operate if no reference is received */
	if (connection == NULL)
		return;

	/* lock the connection being unrefered */
	myqtt_mutex_lock     (&(connection->ref_mutex));

#if defined(ENABLE_MYQTT_LOG)
	/* get context */
	ctx = connection->ctx;
#endif

	/* decrease reference counting */
	connection->ref_count--;

	myqtt_log (MYQTT_LEVEL_DEBUG, "%d decreased connection id=%d (%p) reference count to %d decreased by %s\n", 
		myqtt_getpid (),
		connection->id, connection,
		connection->ref_count, who ? who : "??");  
		
	/* get current count */
	count = connection->ref_count;
	myqtt_mutex_unlock (&(connection->ref_mutex));

	/* if count is 0, free the connection */
	if (count == 0) {
	        /* printf ("**\n** Releasing conn-id=%d conn=%p (refs=0)\n**\n", connection->id, connection); */
		myqtt_conn_free (connection);
	} /* end if */

	return;
}


/** 
 * @brief Allows to get current reference count for the provided connection.
 *
 * See also the following functions:
 *  - \ref myqtt_conn_ref
 *  - \ref myqtt_conn_unref
 * 
 * @param connection The connection that is requested to return its
 * count.
 *
 * @return The function returns the reference count or -1 if it fails.
 */
int                 myqtt_conn_ref_count              (MyQttConn * connection)
{
	int result;

	/* check reference received */
	if (connection == NULL)
		return -1;

	/* return the reference count */
	myqtt_mutex_lock     (&(connection->ref_mutex));
	result = connection->ref_count;
	myqtt_mutex_unlock     (&(connection->ref_mutex));
	return result;
}

/** 
 * @brief Allows to configure myqtt internal timeouts for synchronous
 * operations.
 * 
 * This function allows to set the timeout to use on new
 * connection creation.  Default timeout is defined to 60 seconds (60
 * x 1000000 = 60000000 microseconds).
 *
 * If you call to create a new connection with \ref myqtt_conn_new
 * and remote peer doesn't responds within the period,
 * \ref myqtt_conn_new will return with a non-connected myqtt
 * connection.
 *
 * @param ctx The context where the operation will be performed.
 *
 * @param microseconds_to_wait Timeout value to be used. Providing a
 * value of 0, will reset the timeout to the default value.
 */
void               myqtt_conn_timeout (MyQttCtx * ctx,
				       long       microseconds_to_wait)
{
	/* get current context */
	char      * value;
	
	/* check ctx */
	if (ctx == NULL)
		return;
	
	/* clear previous value */
	if (microseconds_to_wait == 0) {
		/* reset value */
		myqtt_support_unsetenv ("MYQTT_SYNC_TIMEOUT");
	} else {
		/* set new value */
		value = axl_strdup_printf ("%ld", microseconds_to_wait);
		myqtt_support_setenv ("MYQTT_SYNC_TIMEOUT", value);
		axl_free (value);
	} /* end if */

	/* make the next call to myqtt_conn_get_timeout to
	 * recheck the value */
	ctx->connection_timeout_checked = axl_false;

	return;
}

/** 
 * @brief Allows to configure myqtt connect timeout.
 * 
 * This function allows to set the TCP connect timeout used by \ref
 * myqtt_conn_new and \ref myqtt_conn_new6
 *
 * If you call to create a new connection with \ref myqtt_conn_new
 * and connect does not succeed within the period
 * \ref myqtt_conn_new will return with a non-connected myqtt
 * connection.
 *
 * Value configured on this function, will be returned by \ref
 * myqtt_conn_get_connect_timeout. By default, no timeout is
 * configured.
 *
 * See also \ref myqtt_conn_timeout which implements the
 * general timeout used by myqtt engine for I/O operations.
 *
 * @param ctx The context where the operation will be performed.
 *
 * @param microseconds_to_wait Timeout value to be used. The value
 * provided is measured in microseconds (1 seg = 1000000 us). Use 0 to restore the connect
 * timeout to the default value.
 */
void               myqtt_conn_connect_timeout (MyQttCtx * ctx,
					       long        microseconds_to_wait)
{
	/* get current context */
	char      * value;

	/* check reference received */
	if (ctx == NULL)
		return;
	
	/* clear previous value */
	if (microseconds_to_wait == 0) {
		/* unset value */
		myqtt_support_unsetenv ("MYQTT_CONNECT_TIMEOUT");
	} else {
		/* set new value */
		value = axl_strdup_printf ("%ld", microseconds_to_wait);
		myqtt_support_setenv ("MYQTT_CONNECT_TIMEOUT", value);
		axl_free (value);
	} /* end if */

	/* make the next call to myqtt_conn_get_connect_timeout
	 * to recheck the value */
	ctx->connection_connect_timeout_checked = axl_false;

	return;
}

/** 
 * @brief Allows to get current timeout set for \ref MyQttConn
 * synchronous operations.
 *
 * @param ctx The context where the timeout configuration is requested
 *
 * See also \ref myqtt_conn_timeout.
 *
 * @return Current timeout configured. Returned value is measured in
 * microseconds (1 second = 1000000 microseconds).
 */
long                myqtt_conn_get_timeout (MyQttCtx * ctx)
{
	/* get current context */
	long       d_timeout   = ctx->connection_std_timeout;
	
	/* check reference */
	if (ctx == NULL) {
		/* this value must be synchronised by the value
		 * configured at myqtt_conn_init. */
		return (60000000);
	} /* end if */

	/* check if we have used the current environment variable */
	myqtt_mutex_lock  (&(ctx->ref_mutex));
	if (! ctx->connection_timeout_checked) {
		ctx->connection_timeout_checked = axl_true;
		d_timeout                       = myqtt_support_getenv_int ("MYQTT_SYNC_TIMEOUT");
	} /* end if */

	if (d_timeout == 0) {
		/* no timeout definition done using default timeout 10 seconds */
		d_timeout = ctx->connection_std_timeout;
	} else if (d_timeout != ctx->connection_std_timeout) {
		/* update current std timeout */
		ctx->connection_std_timeout = d_timeout;
	} /* end if */

	myqtt_mutex_unlock  (&(ctx->ref_mutex));

	/* return current value */
	return d_timeout;
}

/** 
 * @brief Allows to get current timeout set for \ref MyQttConn
 * connect operation.
 *
 * @param ctx The context where the connection timeout is being requested.
 *
 * See also \ref myqtt_conn_connect_timeout.
 *
 * @return Current timeout configured. Returned value is measured in
 * microseconds (1 second = 1000000 microseconds). If a null value is
 * received, 0 is return and no timeout is implemented.
 */
long              myqtt_conn_get_connect_timeout (MyQttCtx * ctx)
{
	/* check context recevied */
	if (ctx == NULL) {
		/* get the the default connect */
		return (0);
	} /* end if */
		
	/* check if we have used the current environment variable */
	if (! ctx->connection_connect_timeout_checked) {
		ctx->connection_connect_timeout_checked = axl_true;
		/* get value from environment */
		ctx->connection_connect_std_timeout  = myqtt_support_getenv_int ("MYQTT_CONNECT_TIMEOUT");
	} /* end if */

	/* return current value */
	return ctx->connection_connect_std_timeout;
}

/** 
 * @brief Allows to get current connection status
 *
 * This function will allow you to check if your myqtt connection is
 * actually connected. You must use this function before calling
 * myqtt_conn_new to check what have actually happen.
 *
 * You can also use myqtt_conn_get_message to check the message
 * returned by the myqtt layer. This may be useful on connection
 * errors.  The free_on_fail parameter can be use to free myqtt
 * connection resources if this myqtt connection is not
 * connected. This operation will be done by using \ref myqtt_conn_close.
 *
 * 
 * @param connection the connection to get current status.
 *
 * @param free_on_fail if axl_true the connection will be closed using
 * myqtt_conn_close on not connected status.
 * 
 * @return current connection status for the given connection
 */
axl_bool                myqtt_conn_is_ok (MyQttConn * connection, 
						axl_bool          free_on_fail)
{
	axl_bool  result = axl_false;

	/* check connection null referencing. */
	if  (connection == NULL) 
		return axl_false;

	/* check for the socket this connection has */
	myqtt_mutex_lock  (&(connection->ref_mutex));
	result = (connection->session < 0) || (! connection->is_connected);
	myqtt_mutex_unlock  (&(connection->ref_mutex));

	/* implement free_on_fail flag */
	if (free_on_fail && result) {
		myqtt_conn_close (connection);
		return axl_false;
	} /* end if */
	
	/* return current connection status. */
	return ! result;
}

/** 
 * @brief Frees myqtt connection resources
 * 
 * Free all resources allocated by the myqtt connection. You have to
 * keep in mind if <i>connection</i> is already connected,
 * myqtt_conn_free will close the connection.
 *
 * Generally is not a good a idea to call this function. This is
 * because every connection created using the myqtt API is registered
 * at some internal process (the myqtt reader, sequencer and writer)
 * so they have references to created connection to do its job. 
 *
 * To close a connection properly call \ref myqtt_conn_close.
 * 
 * @param connection the connection to free
 */
void               myqtt_conn_free (MyQttConn * connection)
{
#if defined(ENABLE_MYQTT_LOG)
	MyQttCtx          * ctx;
#endif
	
	if (connection == NULL)
		return;

#if defined(ENABLE_MYQTT_LOG)
	/* get a reference to the context reference */
	ctx = connection->ctx;
#endif

	myqtt_log (MYQTT_LEVEL_DEBUG, "freeing connection id=%d (%p)", connection->id, connection);

	myqtt_log (MYQTT_LEVEL_DEBUG, "freeing connection custom data holder id=%d", connection->id);
	/* free connection data hash */
	if (connection->data) {
		myqtt_hash_destroy (connection->data);
		connection->data = NULL;
	}

	myqtt_log (MYQTT_LEVEL_DEBUG, "freeing connection host id=%d", connection->id);

	/* free host and port */
	if (connection->host != NULL) {
		axl_free (connection->host);
		axl_free (connection->local_addr);
		connection->host       = NULL;
		connection->local_addr = NULL;
	}
	axl_free (connection->host_ip);

	myqtt_log (MYQTT_LEVEL_DEBUG, "freeing connection port id=%d", connection->id);

	if (connection->port != NULL) {
		axl_free (connection->port);
		axl_free (connection->local_port);
		connection->port       = NULL;
		connection->local_port = NULL;
	}

	/* free pending line */
	axl_free (connection->pending_line);
	connection->pending_line = NULL;

	myqtt_log (MYQTT_LEVEL_DEBUG, "freeing connection reference counting mutex id=%d", connection->id);

	/* release ref mutex */
	myqtt_mutex_destroy (&connection->ref_mutex);

	myqtt_log (MYQTT_LEVEL_DEBUG, "freeing connection operational mutex id=%d", connection->id);

	myqtt_mutex_destroy (&connection->op_mutex);

	myqtt_mutex_destroy (&connection->handlers_mutex);

	myqtt_log (MYQTT_LEVEL_DEBUG, "freeing/terminating connection id=%d", connection->id);

	/* close connection */
	if (( connection->close_session) && (connection->session != -1)) {
		/* it seems that this connection is  */
		shutdown (connection->session, SHUT_RDWR);
		myqtt_close_socket (connection->session);
		connection->session = -1;
		myqtt_log (MYQTT_LEVEL_DEBUG, "session socket closed");
	}


	/* release on close notification */
	axl_list_free (connection->on_close_full);
	connection->on_close_full = NULL;

	/* wait reply hash */
	axl_hash_free (connection->wait_replies);
	connection->wait_replies = NULL;

	/* peer wait reply hash */
	axl_hash_free (connection->peer_wait_replies);
	connection->peer_wait_replies = NULL;

	/* wild subs */
	axl_hash_free (connection->subs);
	connection->subs = NULL;
	axl_hash_free (connection->wild_subs);
	connection->wild_subs = NULL;

	/* free identifiers */
	axl_free (connection->client_identifier);
	axl_free (connection->username);
	axl_free (connection->password);
	connection->password = NULL;
	axl_free (connection->will_topic);
	axl_free (connection->will_msg);
	axl_free (connection->serverName);

	/* sending package ids */
	axl_list_free (connection->sent_pkgids);
	connection->sent_pkgids = NULL;

	/* free possible msg and buffer */
	axl_free (connection->buffer);

	/* release ping resp queue if defined */
	myqtt_async_queue_unref (connection->ping_resp_queue);
	connection->ping_resp_queue = NULL;

	/* release reference to context */
	myqtt_ctx_unref2 (&connection->ctx, "end connection");

	/* release master connection options */
	if (connection->opts) {
		/* if it is: 
		 *
		 *  a) master role connection release or 
		 *  b) it has reconnect enabled but without reuse on 
		 * 
		 * then, release myqtt_conn_opts_free (opts)
		 */
		if (connection->role == MyQttRoleMasterListener || (connection->opts->reconnect && ! connection->opts->reuse))
			myqtt_conn_opts_free (connection->opts);
	} /* end if */

	/* free connection */
	axl_free (connection);

	return;
}



/** 
 * @brief Returns the socket used by this MyQttConn object.
 * 
 * @param connection the connection to get the socket.
 * 
 * @return the socket used or -1 if fail
 */
MYQTT_SOCKET    myqtt_conn_get_socket           (MyQttConn * connection)
{
	/* check reference received */
	if (connection == NULL)
		return -1;

	return connection->session;
}

/** 
 * @brief Allows to configure what to do with the underlying socket
 * connection when the \ref MyQttConn is closed.
 *
 * This function could be useful if it is needed to prevent from
 * myqtt library to close the underlying socket.
 * 
 * @param connection The connection to configure.
 *
 * @param action axl_true if the given connection have to be close once a
 * MyQtt Conn is closed. axl_false to avoid the socket to be
 * closed.
 */
void                myqtt_conn_set_close_socket       (MyQttConn * connection, 
							      axl_bool           action)
{
	if (connection == NULL)
		return;
	connection->close_session = action;
	return;
}

/** 
 * @internal Function used by myqtt to notify that this connection
 * has either written or received content, setting a time stamp, which
 * allows checking if the connection was idle.
 *
 * @param conn The connection that received or produced content.
 * @param bytes Bytes received on this stamp.
 */
void                myqtt_conn_set_receive_stamp            (MyQttConn * conn, long bytes_received, long bytes_sent)
{
	/* set that content was received */
	myqtt_mutex_lock (&conn->ref_mutex);
	conn->last_idle_stamp  = (long) time (NULL);
	conn->bytes_received  += bytes_received;
	conn->bytes_sent      += bytes_sent;
	myqtt_mutex_unlock (&conn->ref_mutex);

	return;
}

/** 
 * @internal allows to get bytes received so far and last idle stamp
 * (idle since that stamp) on the provided connection.
 */ 
void                myqtt_conn_get_receive_stamp            (MyQttConn * conn, 
								   long             * bytes_received, 
								   long             * bytes_sent,
								   long             * last_idle_stamp)
{
	if (bytes_received != NULL)
		(*bytes_received) = 0;
	if (bytes_sent != NULL)
		(*bytes_sent) = 0;
	if (last_idle_stamp != NULL)
		(*last_idle_stamp) = 0;
	if (conn == NULL)
		return;
	myqtt_mutex_lock (&conn->ref_mutex);
	if (bytes_received != NULL)
		(*bytes_received) = conn->bytes_received;
	if (bytes_sent != NULL)
		(*bytes_sent) = conn->bytes_sent;
	if (last_idle_stamp != NULL)
		(*last_idle_stamp) = conn->last_idle_stamp;
	myqtt_mutex_unlock (&conn->ref_mutex);

	return;
}

/** 
 * @internal Function used to check idle status, calling the handler
 * defined if the idle status is reached. The function also resets the
 * idle status, so it can be called in the future. 
 */
void                myqtt_conn_check_idle_status            (MyQttConn * conn, MyQttCtx * ctx, long time_stamp)
{
	/* do not notify master listeners activity (they don't have) */
	if (conn->role == MyQttRoleMasterListener)
		return;

	/* check if the connection was never checked */
	if (conn->last_idle_stamp == 0) {
		myqtt_conn_set_receive_stamp (conn, 0, 0);
		return;
	} /* end if */

	/* check idle status */
	if ((time_stamp - conn->last_idle_stamp) > ctx->max_idle_period) {
		myqtt_log (MYQTT_LEVEL_DEBUG, "Found idle connection id=%d, notifying..", myqtt_conn_get_id (conn));
		/* notify idle ref */
		myqtt_ctx_notify_idle (ctx, conn);

		return;
	} /* end if */
	return;
}

/** 
 * @brief Returns the actual host this connection is connected to.
 *
 * In the case the connection you have provided have the \ref
 * MyQttRoleMasterListener role (\ref myqtt_conn_get_role),
 * that is, listener connections that are waiting for connections, the
 * function will return the actual host used by the listener.
 *
 * You must not free returned value.  If you do so, you will get
 * unexpected behaviours.
 * 
 * 
 * @param connection the connection to get host value.
 * 
 * @return the host the given connection is connected to or NULL if something fail.
 */
const char         * myqtt_conn_get_host             (MyQttConn * connection)
{
	if (connection == NULL)
		return NULL;

	return connection->host;
}

/** 
 * @internal Function used to setup manual values returned by \ref
 * myqtt_conn_get_host and \ref myqtt_conn_get_port.
 */
void                myqtt_conn_set_host_and_port      (MyQttConn * connection, 
							      const char       * host,
							      const char       * port,
							      const char       * host_ip)
{
	v_return_if_fail (connection && host && port);
	
	if (connection->host)
		axl_free (connection->host);
	if (connection->port)
		axl_free (connection->port);
	if (connection->host_ip)
		axl_free (connection->host_ip);
	
	/* set host, port and ip value */
	connection->host    = axl_strdup (host);
	connection->port    = axl_strdup (port);
	connection->host_ip = axl_strdup (host_ip);

	return;
}

/** 
 * @brief Allows to get the actual host ip this connection is
 * connected to.
 *
 * @param connection The connection where the host ip value is being requested.
 *
 * This function works like \ref myqtt_conn_get_host_ip but
 * returning the actual ip in the case a name was used.
 *
 * @return A reference to the IP or NULL if it fails.
 */
const char        * myqtt_conn_get_host_ip            (MyQttConn * connection)
{
	struct sockaddr_in     sin;
#if defined(AXL_OS_WIN32)
	/* windows flavors */
	int                    sin_size     = sizeof (sin);
#else
	/* unix flavors */
	socklen_t              sin_size     = sizeof (sin);
#endif
	/* acquire lock to check if host ip was defined previously */
	myqtt_mutex_lock (&connection->op_mutex);
	if (connection->host_ip) {
		myqtt_mutex_unlock (&connection->op_mutex);
		return connection->host_ip;
	} /* end if */

	/* get actual IP value */
	if (getpeername (connection->session, (struct sockaddr *) &sin, &sin_size) < 0) {
		myqtt_mutex_unlock (&connection->op_mutex);
		return NULL;
	} /* end if */

	/* set local addr and local port */
	connection->host_ip = myqtt_support_inet_ntoa (connection->ctx, &sin);

	/* unlock and return value created */
	myqtt_mutex_unlock (&connection->op_mutex);
	return connection->host_ip;
}

/** 
 * @brief  Returns the connection unique identifier.
 *
 * The connection identifier is a unique integer assigned to all
 * connection created under MyQtt Library. This allows MyQtt programmer to
 * use this identifier for its own application purposes
 *
 * @param connection The connection to get the the unique integer
 * identifier from.
 * 
 * @return the unique identifier.
 */
int                myqtt_conn_get_id               (MyQttConn * connection)
{
	if (connection == NULL)
		return -1;

	return connection->id;
}

/** 
 * @brief Allows to get the username used during CONNECT phase on the
 * provided connection.
 *
 * @param conn The connection where the operation takes place.
 *
 * @return The username used or NULL if the CONNECT phase didn't
 * provide a username. The function also reports NULL when conn
 * reference received is NULL.
 */
const char *        myqtt_conn_get_username           (MyQttConn * conn)
{
	if (conn == NULL)
		return NULL;
	return conn->username;
}

/** 
 * @brief Allows to get the password used during CONNECT phase on the
 * provided connection.
 *
 * @param conn The connection where the operation takes place.
 *
 * @return The password used or NULL if the CONNECT phase didn't
 * provide a password. 
 *
 * <b>SECURITY NOTE</b>: For security reasons, the function won't return
 * the password (if any) after the CONNECT method completes. The
 * function also reports NULL when conn reference received is NULL. 
 *
 * <b>IMPORTANT NOTE</b>: In the case the function returns a value, it
 * shouldn't be used after \ref MyQttOnConnectHandler is called (if
 * defined that handler) and/or after the connection is completely
 * accepted.
 */
const char *        myqtt_conn_get_password           (MyQttConn * conn)
{
	if (conn == NULL)
		return NULL;
	return conn->password;
}

/** 
 * @brief Allows to get the client id used during CONNECT phase on the
 * provided connection.
 *
 * @param conn The connection where the operation takes place.
 *
 * @return The client id used or NULL if the CONNECT phase didn't
 * provide a client id. The function also reports NULL when conn
 * reference received is NULL.
 */
const char *        myqtt_conn_get_client_id          (MyQttConn * conn)
{
	if (conn == NULL)
		return NULL;
	return conn->client_identifier;
}

/** 
 * @brief Returns the actual port this connection is connected to.
 *
 * In the case the connection you have provided have the \ref
 * MyQttRoleMasterListener role (\ref myqtt_conn_get_role),
 * that is, a listener that is waiting for connections, the
 * function will return the actual port used by the listener.
 *
 * @param connection the connection to get the port value.
 * 
 * @return the port or NULL if something fails.
 */
const char        * myqtt_conn_get_port             (MyQttConn * connection)
{
	
	/* check reference received */
	if (connection == NULL)
		return NULL;

	return connection->port;
}

/** 
 * @brief Allows to get local address used by the connection.
 * @param connection The connection to check.
 *
 * @return A reference to the local address used or NULL if it fails.
 */
const char        * myqtt_conn_get_local_addr         (MyQttConn * connection)
{
	/* check reference received */
	if (connection == NULL)
		return NULL;

	return connection->local_addr;
}

/** 
 * @brief Allows to get the local port used by the connection. 
 * @param connection The connection to check.
 * @return A reference to the local port used or NULL if it fails.
 */
const char        * myqtt_conn_get_local_port         (MyQttConn * connection)
{
	/* check reference received */
	if (connection == NULL)
		return NULL;

	return connection->local_port;
}

typedef struct __MyQttOnCloseNotify {
	MyQttConn              * conn;
	MyQttConnOnClose         handler;
	axlPointer               data;
} MyQttOnCloseNotify;

axlPointer __myqtt_conn_on_close_do_notify (MyQttOnCloseNotify * data)
{
#if defined(ENABLE_MYQTT_LOG) && ! defined(SHOW_FORMAT_BUGS)
	MyQttCtx * ctx = CONN_CTX (data->conn);
#endif
	/* do notification */
	data->handler (data->conn, data->data);

	/* terminate thread */
	myqtt_log (MYQTT_LEVEL_DEBUG, "async on close notification done for conn-id=%d..", myqtt_conn_get_id (data->conn));

	myqtt_conn_unref (data->conn, "on-close-notification");
	axl_free (data);
	return NULL;
}

void __myqtt_conn_invoke_on_close_do_notify (MyQttConn            * conn, 
					     MyQttConnOnClose       handler, 
					     axlPointer             user_data)
{
	MyQttOnCloseNotify * data;
#if defined(ENABLE_MYQTT_LOG) && ! defined(SHOW_FORMAT_BUGS)
	MyQttCtx           * ctx = CONN_CTX (conn);
#endif
	MyQttThread          thread_def;

	/* acquire data to launch the thread */
	data = axl_new (MyQttOnCloseNotify, 1);
	if (data == NULL)
		return;

	/* set all data */
	data->conn    = conn;
	data->handler = handler;
	data->data    = user_data;
	
	/* increase reference counting during thread activation */
	myqtt_conn_ref_internal (conn, "on-close-notification", axl_false);

	/* call to notify using a thread */
	myqtt_log (MYQTT_LEVEL_DEBUG, "calling to create thread to implement on close notification for=%d", myqtt_conn_get_id (conn));
	if (! myqtt_thread_create (&thread_def, (MyQttThreadFunc) __myqtt_conn_on_close_do_notify, data, 
				    MYQTT_THREAD_CONF_DETACHED, MYQTT_THREAD_CONF_END)) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "failed to start thread to do connection-id=%d close notify", myqtt_conn_get_id (conn));

		/* release data due to failure */
		myqtt_conn_unref (conn, "on-close-notification");
		axl_free (data);
		return;
	}

	return;
} 

/** 
 * @internal 
 *
 * Function used to perform on close invocation
 * 
 * @param connection The connection where all on close handlers will
 * be notified.
 */
void __myqtt_conn_invoke_on_close (MyQttConn * connection)
{
	MyQttConnOnCloseData  * handler;
	MyQttCtx              * ctx = connection->ctx;

	/* check if context is in process of finishing */
	if (ctx->myqtt_exit) 
		return;

	/* lock now the op mutex is not blocked */
	myqtt_mutex_lock (&connection->handlers_mutex);

	/* invoke full */
	/* iterate over all full handlers and invoke them */
	while (axl_list_length (connection->on_close_full) > 0) {
		
		/* get a reference to the handler */
		handler = axl_list_get_first (connection->on_close_full);
		
		myqtt_log (MYQTT_LEVEL_DEBUG, "running on close full handler %p conn-id=%d, remaining handlers: %d",
			   handler, connection->id, axl_list_length (connection->on_close_full));
		
		/* remove the handler from the list */
		axl_list_unlink_first (connection->on_close_full);
		
		/* unlock now the op mutex is not blocked */
		myqtt_mutex_unlock (&connection->handlers_mutex);
		
		/* invoke */
		__myqtt_conn_invoke_on_close_do_notify (connection, handler->handler, handler->data);
		
		/* free handler node */
		axl_free (handler);
		
		/* reacquire the mutex */
		myqtt_mutex_lock (&connection->handlers_mutex);

	} /* end while */

	/* unlock now the op mutex is not blocked */
	myqtt_mutex_unlock (&connection->handlers_mutex);

	return;
} 

/** 
 * @brief Allows to shutdown the connection without cleanly closing like using \ref myqtt_conn_close.
 *
 * @param connection The connection to be closed.
 */
void    myqtt_conn_shutdown           (MyQttConn * connection)
{
	/* call to internal set not connected */
	__myqtt_conn_set_not_connected (connection, "(myqtt connection shutdown)", MyQttConnectionForcedClose);
	return;
	
} /* end myqtt_conn_shutdown */

/** 
 * @brief Function used to just shutdown socket associated to the
 * connection.
 *
 * This function differs from \ref myqtt_conn_shutdown in the
 * sense this one do not triggers all the internal connection close
 * (like calling connection close handlers, dropping msgs associated
 * to this connection at the sequencer). 
 *
 * @param connection The connection where the socket will be closed.
 */
void    myqtt_conn_shutdown_socket      (MyQttConn * connection)
{
	if (connection == NULL)
		return;
	myqtt_close_socket (connection->session);
	/* do not set connection->session to -1 */
	return;
}



void                myqtt_conn_block                        (MyQttConn * conn,
								    axl_bool           enable)
{
	v_return_if_fail (conn);
	
	/* set blocking state */
	conn->is_blocked = enable;

	/* call to restart the reader */
	myqtt_reader_restart (myqtt_conn_get_ctx (conn));

	return;
}

/** 
 * @brief Allows to check if the connection provided is blocked.
 * 
 * @param conn The connection to check for its associated blocking
 * state.
 * 
 * @return axl_true if the connection is blocked (no read I/O operation
 * available), otherwise axl_false is returned, signalling the connection
 * is fully I/O operational. Keep in mind the function returns axl_false
 * if the reference provided is NULL.
 */
axl_bool             myqtt_conn_is_blocked                   (MyQttConn  * conn)
{
	v_return_val_if_fail (conn, axl_false);

	/* set blocking state */
	return conn->is_blocked;
}

/** 
 * @brief Allows to check if the provided connection is still in
 * transit of being accepted.
 *
 * @param conn The connection to be checked. In the case the function
 * returns axl_true it means the connection is still not fully
 * opened/accepted (still greetings phase wasn't finished).
 *
 * @return axl_true in the case the connection is half opened (MQTT
 * greetings not yet not completed) otherwise axl_false is
 * returned. The function also returns axl_false in the case or NULL
 * pointer received.
 */
axl_bool            myqtt_conn_half_opened                  (MyQttConn  * conn)
{
	if (conn == NULL)
		return axl_false;
	return conn->initial_accept;
}

axl_bool __myqtt_conn_do_reconnect_third_step (MyQttCtx * ctx, axlPointer _conn, axlPointer user_data)
{
	MyQttConn     * conn = _conn;
	axl_bool        stop_event = axl_false;
	MyQttConnOpts * opts;

	myqtt_log (MYQTT_LEVEL_DEBUG, "..reconnect: attempting to reconnect ctx=%p, conn=%p, socket=%d", ctx, conn, conn->session);

	/* check if we have a init session setup pointer handler and
	   call it */
	if (conn && conn->opts) {
		/* define opts pointer */
		opts = conn->opts;

		if (opts && opts->init_session_setup_ptr) {
			/* call to create the session setup pointer */
			conn->setup_user_data = opts->init_session_setup_ptr (ctx, conn, 
									      opts->init_user_data, 
									      opts->init_user_data2,
									      opts->init_user_data3);
		} /* end if */

		myqtt_log (MYQTT_LEVEL_DEBUG, "..reconnect: conn->setup_user_data=%p", conn->setup_user_data);

	} /* end if */

	/* ok, now call to reconnect in this independent thread */
	__myqtt_conn_new_internal (conn->ctx, conn, conn->opts);

	myqtt_log (MYQTT_LEVEL_DEBUG, "..reconnect: after connect ctx=%p, conn=%p, socket=%d, myqtt_conn_is_ok (conn,axl_false)=%d", 
		   ctx, conn, conn->session, myqtt_conn_is_ok (conn, axl_false));

	/* now check if the connection is ok */
	if (myqtt_conn_is_ok (conn, axl_false)) {

		/* ok, register connection into the reader again */
		myqtt_reader_watch_connection (conn->ctx, conn);

		/* stop the event because the connection is ok */
		stop_event = axl_true;

		/* notify if handler is defined */
		if (conn->on_reconnect)
			conn->on_reconnect (conn, conn->on_reconnect_data);

		/* release reference */
		myqtt_conn_unref (conn, "myqtt-conn-reconnect");

	} /* end if */

	/* return stop_event=axl_true to remove this event, otherwise, axl_false to keep on trying */
	return stop_event;
}


void __myqtt_conn_do_reconnect_second_step (MyQttCtx * ctx, MyQttConn * conn, axlPointer user_data)
{
	/* now launch a threaded process to reconnect */
	myqtt_thread_pool_new_event (ctx, 1000000, __myqtt_conn_do_reconnect_third_step, conn, NULL);
	return;
}

void __myqtt_conn_do_reconnect (MyQttConn * conn)
{
	/* acquire a reference to the connection during the process */
	if (! myqtt_conn_ref_internal (conn, "myqtt-conn-reconnect", axl_false))
		return; /* if we fail to acquire a reference, just return */

	/* remove connection from the reader while we are working */
	myqtt_reader_unwatch_connection (conn->ctx, conn, __myqtt_conn_do_reconnect_second_step, NULL);
	return;
}

/** 
 * @internal
 * @brief Sets the given connection the not connected status.
 *
 * This internal myqtt function allows library to set connection
 * status to axl_false for a given connection. This function should not be
 * used by myqtt library consumers.
 *
 * This function can be called any number of times on the same
 * connection. The first time the function is called sets to axl_false
 * connection state and its error message. It also close the MQTT
 * session and sets its socket to -1 in order to make it easily to
 * recognise for other functions.
 *
 * It doesn't make a connection unref. This actually done by the
 * MyQtt Library internal process once they detect that the given
 * connection is not connected. 
 *
 * NOTE: next calls to function will lose error message.
 * 
 * @param connection the connection to set as.
 * @param message the new message to set.
 */
void           __myqtt_conn_set_not_connected (MyQttConn * connection, 
						      const char       * message,
						      MyQttStatus       status)
{
#if defined(ENABLE_MYQTT_LOG)
	MyQttCtx * ctx;
#endif

	/* check reference received */
	if (connection == NULL || message == NULL)
		return;

#if defined(ENABLE_MYQTT_LOG)
	/* get a reference to the context */
	ctx = connection->ctx;
#endif

	/* set connection status to axl_false if weren't */
	myqtt_mutex_lock (&connection->op_mutex);

	if (connection->is_connected) {
		/* acquire a reference to the connection during the
		   shutdown to avoid race conditions with listener
		   connections and the myqtt reader loop */
		myqtt_conn_ref_internal (connection, "set-not-connected", axl_false);

		myqtt_log (MYQTT_LEVEL_DEBUG, "flagging the connection as non-connected: %s (close-session?: %d)", message ? message : "",
			    connection->close_session);

		/* set value consistently */
		myqtt_mutex_lock  (&(connection->ref_mutex));
		connection->is_connected = axl_false;
		myqtt_mutex_unlock  (&(connection->ref_mutex));

		/* unlock now the op mutex is not blocked */
		myqtt_mutex_unlock (&connection->op_mutex);

		/* check for the close handler full definition */
		if (connection->on_close_full != NULL) {
			myqtt_log (MYQTT_LEVEL_DEBUG, "notifying connection-id=%d close handlers", connection->id);
			/* invoking on close handler full */ 
			__myqtt_conn_invoke_on_close (connection);
		}

		/* close socket connection if weren't  */
		if (( connection->close_session) && (connection->session != -1)) {
			myqtt_log (MYQTT_LEVEL_DEBUG, "closing connection id=%d to %s:%s (socket: %d)", 
				    connection->id,
				    axl_check_undef (connection->host), 
				    axl_check_undef (connection->port), connection->session);
			shutdown (connection->session, SHUT_RDWR); 
			myqtt_close_socket (connection->session);  

			/* set session value */
			myqtt_mutex_lock (&(connection->ref_mutex));
			connection->session      = -1;
			myqtt_mutex_unlock (&(connection->ref_mutex));

 	        } /* end if */

		/* implement automatic reconnect after all have been
		   closed and notified */
		if (connection->opts && connection->opts->reconnect) 
			__myqtt_conn_do_reconnect (connection);

		/* finish reference acquired after unlocking and doing
		   all close stuff */
		myqtt_conn_unref (connection, "set-not-connected");

		return;
	}

	myqtt_mutex_unlock (&connection->op_mutex);

	return;
}


/** 
 * @brief Sets user defined data associated with the given connection.
 *
 * The function allows to store arbitrary data associated to the given
 * connection. Data stored will be indexed by the provided key,
 * allowing to retrieve the information using: \ref
 * myqtt_conn_get_data.
 *
 * If the value provided is NULL, this will be considered as a
 * removing request for the given key and its associated data.
 * 
 * See also \ref myqtt_conn_set_data_full function. It is an
 * alternative API that allows configuring a set of destroy handlers
 * for key and data stored.
 *
 * @param connection The connection where data will be associated.
 *
 * @param key The string key indexing the data stored associated to
 * the given key.
 *
 * @param value The value to be stored. NULL to remove previous data
 * stored.
 */
void               myqtt_conn_set_data               (MyQttConn * connection,
							     const char       * key,
							     axlPointer         value)
{
	/* use the full version so all source code is supported in one
	 * function. */
	myqtt_conn_set_data_full (connection, (axlPointer) key, value, NULL, NULL);
	return;
}

/** 
 * @brief Allows to remove a key/value pair installed by
 * myqtt_conn_set_data and myqtt_conn_set_data_full
 * without calling destroy functions associated.
 *
 * @param connection The connection where the key/value entry will be
 * removed without calling destroy function associated (if any) to
 * both (key and value).
 *
 * @param key The key that identifies the entry to be deleted.
 * 
 */
void                myqtt_conn_delete_key_data        (MyQttConn * connection,
							      const char       * key)
{
	if (connection == NULL || connection->data == NULL)
		return;
	myqtt_hash_delete (connection->data, (axlPointer) key);
	return;
}
/** 
 * @brief Allows to store user space data into the connection like
 * \ref myqtt_conn_set_data does but configuring functions to
 * be called once required to de-allocate data stored.
 *
 * While storing user defined data into the connection it could be
 * necessary to also define destroy functions for the value stored and
 * the key stored. This allows to not worry about to free those data
 * (including the key) once the connection is dropped.
 *
 * This function allows to store data into the given connection
 * defining destroy functions for the key and the value stored, per item.
 * 
 * \code
 * [...]
 * void destroy_data_1 (axlPointer data) 
 * {
 *     // perform a memory de-allocation for data1
 * }
 * 
 * void destroy_data_2 (axlPointer data) 
 * {
 *     // perform a memory de-allocation for data2
 * }
 * [...]
 * // store data 1 providing a destroy value function
 * myqtt_conn_set_data_full (connection, "some:data:1", 
 *                                  data_1, NULL, destroy_data_1);
 *
 * // store data 2 providing a destroy value function
 * myqtt_conn_set_data_full (connection, "some:data:2",
 *                                  data_2, NULL, destroy_data_2);
 * [...]
 * \endcode
 * 
 *
 * @param connection    The connection where the data will be stored.
 * @param key           The unique string key value.
 * @param value         The value to be stored associated to the given key.
 * @param key_destroy   An optional key destroy function used to destroy (de-allocate) memory used by the key.
 * @param value_destroy An optional value destroy function used to destroy (de-allocate) memory used by the value.
 */
void                myqtt_conn_set_data_full          (MyQttConn * connection,
							      char             * key,
							      axlPointer         value,
							      axlDestroyFunc     key_destroy,
							      axlDestroyFunc     value_destroy)
{

	/* check reference */
	if (connection == NULL || key == NULL)
		return;

	/* check if the value is not null. It it is null, remove the
	 * value. */
	if (value == NULL) {
		myqtt_hash_remove (connection->data, key);
		return;
	}

	/* store the data selected replacing previous one */
	myqtt_hash_replace_full (connection->data, 
				  key, key_destroy, 
				  value, value_destroy);
	
	/* return from setting the value */
	return;
}

/** 
 * @brief Allows to set a commonly used user land pointer associated
 * to the provided connection.
 *
 * Though you can use \ref myqtt_conn_set_data_full or \ref myqtt_conn_set_data, this function allows to set a pointer
 * that can be retrieved by using \ref myqtt_conn_get_hook with a low cpu usage.
 *
 * @param connection The connection where the user land pointer is associated.
 *
 * @param ptr The pointer that will be associated to the connection.
 */
void                myqtt_conn_set_hook               (MyQttConn * connection,
							      axlPointer         ptr)
{
	if (connection == NULL)
		return;
	connection->hook = ptr;
	return;
}

/** 
 * @brief Allows to get the user land pointer configured by \ref myqtt_conn_set_hook.
 *
 * @param connection The connection where the user land pointer is
 * being queried.
 *
 * @return The pointer stored.
 */
axlPointer          myqtt_conn_get_hook               (MyQttConn * connection)
{
	if (connection == NULL)
		return NULL;
	return connection->hook;
}


/** 
 * @brief Gets stored value indexed by the given key inside the given connection.
 *
 * The function returns stored data using \ref
 * myqtt_conn_set_data or \ref myqtt_conn_set_data_full.
 * 
 * @param connection the connection where the value will be looked up.
 * @param key the key to look up.
 * 
 * @return the value or NULL if fails.
 */
axlPointer         myqtt_conn_get_data               (MyQttConn * connection,
							     const char       * key)
{
 	v_return_val_if_fail (connection,       NULL);
 	v_return_val_if_fail (key,              NULL);
 	v_return_val_if_fail (connection->data, NULL);

	return myqtt_hash_lookup (connection->data, (axlPointer) key);
}

/** 
 * @brief Allows to get current data hash object used by the provided
 * connection. This way you can use this hash object directly with
 * \ref myqtt_hash "myqtt hash API".
 *
 * @param connection The connection where the hash has been requested..
 *
 * @return A reference to the hash or NULL if it fails.
 */
MyQttHash        * myqtt_conn_get_data_hash          (MyQttConn * connection)
{
	v_return_val_if_fail (connection, NULL);
	return connection->data;
}



/** 
 * @brief Allows to get current connection role.
 * 
 * @param connection The MyQttConn to get the current role from.
 * 
 * @return Current role represented by \ref MyQttPeerRole. If the
 * function receives a NULL reference it will return \ref
 * MyQttRoleUnknown.
 */
MyQttPeerRole      myqtt_conn_get_role               (MyQttConn * connection)
{
	/* if null reference received, return unknown role */
	v_return_val_if_fail (connection, MyQttRoleUnknown);

	return connection->role;
}


/** 
 * @brief The function returns the specific MQTT listener that accepted the provided connection.
 *
 * This function allows to get a reference to the listener that
 * accepted and created the current connection. In the case this
 * function is called over a client connection NULL will be returned.
 * 
 * @param connection The connection that is required to return the
 * master connection associated (the master connection will have the
 * role: \ref MyQttRoleMasterListener).
 * 
 * @return The reference or NULL if it fails. 
 */
MyQttConn  * myqtt_conn_get_listener           (MyQttConn * connection)
{
	/* check reference received */
	if (connection == NULL)
		return NULL;
	
	/* returns current master connection associated */
	return myqtt_conn_get_data (connection, "_vo:li:master");
}

/** 
 * @brief Allows to get the server name that was used on this
 * connection by indicating the host to connect to.
 *
 * @param conn The connection where the operation is taking place.
 *
 * @return A reference to the connection where the serverName is stored.
 */
const char        * myqtt_conn_get_server_name        (MyQttConn * conn)
{
	/* check input connection */
	if (! conn)
		return NULL;

	/* still not implemented */
	return conn->serverName;
}

/** 
 * @internal Allows to configure the serverName under which this
 * connection is working.
 *
 * @param conn The connection to be configured.
 *
 * @param serverName the server name to be configured. 
 */
void                myqtt_conn_set_server_name        (MyQttConn * conn, const char * serverName)
{
	char * temp;
	if (conn == NULL || serverName == NULL)
		return;

	/* save serverName */
	temp = conn->serverName;
	conn->serverName = axl_strdup (serverName);
	axl_free (temp);
	return;
}

/** 
 * @brief Allows to get the context under which the connection was
 * created. 
 * 
 * @param connection The connection that is required to return the context under
 * which it was created.
 * 
 * @return A reference to the context associated to the connection or
 * NULL if it fails.
 */
MyQttCtx         * myqtt_conn_get_ctx                (MyQttConn * connection)
{
	/* check value received */
	if (connection == NULL)
		return NULL;

	/* reference to the connection */
	return connection->ctx;
}

/** 
 * @brief Allows to configure the send handler used to actually
 * perform sending operations over the underlying connection.
 *
 * 
 * @param connection The connection where the send handler will be set.
 * @param send_handler The send handler to be set.
 * 
 * @return Returns the previous send handler defined or NULL if fails.
 */
MyQttSend      myqtt_conn_set_send_handler    (MyQttConn * connection,
							      MyQttSend  send_handler)
{
	MyQttSend previous_handler;

	/* check parameters received */
	if (connection == NULL || send_handler == NULL)
		return NULL;

	/* save previous handler defined */
	previous_handler = connection->send;

	/* set the new send handler to be used. */
	connection->send = send_handler;

	/* returns previous handler */
	return previous_handler;
 
	
}

/** 
 * @brief Allows to configure receive handler use to actually receive
 * data from remote peer. 
 * 
 * @param connection The connection where the receive handler will be set.
 * @param receive_handler The receive handler to be set.
 * 
 * @return Returns current receive handler set or NULL if it fails.
 */
MyQttReceive   myqtt_conn_set_receive_handler (MyQttConn     * connection,
							      MyQttReceive   receive_handler)
{
	MyQttReceive previous_handler;

	/* check parameters received */
	if (connection == NULL || receive_handler == NULL)
		return NULL;

	/* save previous handler defined */
	previous_handler    = connection->receive;

	/* set the new send handler to be used. */
	connection->receive = receive_handler;

	/* returns previous handler */
	return previous_handler;
}

/** 
 * @brief Set default IO handlers to be used while sending and
 * receiving data for the given connection.
 * 
 * MyQtt Library allows to configure which are the function to be
 * used while sending and receiving data from the underlying
 * transport. This functions are usually not required API consumers.
 *
 * This function allows to configure the default handlers to be used
 * by the MyQtt Library. This useful to implement handler restoring.
 * 
 * @param connection The connection where the handler restoring will
 * take place.
 *
 */
void                   myqtt_conn_set_default_io_handler (MyQttConn * connection)
{
#if defined(ENABLE_MYQTT_LOG)
	MyQttCtx * ctx;
#endif

	/* check reference */
	if (connection == NULL)
		return;

#if defined(ENABLE_MYQTT_LOG)
	/* get a reference */
	ctx = connection->ctx;
#endif

	/* set default send and receive handlers */
	connection->send       = myqtt_conn_default_send;
	connection->receive    = myqtt_conn_default_receive;
	myqtt_log (MYQTT_LEVEL_DEBUG, "restoring default IO handlers for connection id=%d", 
		    connection->id);

	return;
}

/** 
 * @brief Allows to configure the on message handler for the provided connection.
 *
 * This handler will be called each time a PUBLISH message is received
 * on the provided connection (when acting as a client).  See \ref
 * MyQttOnMsgReceived for more information about the handler and the
 * signature you have to provide.
 *
 * If you want to set a handler to get a notification at the server
 * side (acting as a broker) for every message received see \ref
 * myqtt_ctx_set_on_publish
 *
 * The handler configured on this function will take precedence over
 * that configured using myqtt_ctx_set_on_msg (which applies to all
 * connections that do not have a handler configured myqtt_conn_set_on_msg).
 *
 * You can only configure one handler at time. Calling to configure a
 * handler twice will replace previous one.
 *
 * See also \ref myqtt_ctx_set_on_msg
 *
 * @param conn The connection that is going to be configured with the
 * handler. Only one handler can be configured. Every call to this
 * function will replace previous one.
 *
 * @param on_msg The handler to be configured.
 *
 * @param on_msg_data User defined pointer to be passed in to on_msg handler every time it is called.
 *
 */
void                   myqtt_conn_set_on_msg         (MyQttConn          * conn,
						      MyQttOnMsgReceived   on_msg,
						      axlPointer           on_msg_data)
{
	if (conn == NULL)
		return;

	/* configure handler */
	myqtt_mutex_lock (&conn->op_mutex);
	conn->on_msg_data = on_msg_data;
	conn->on_msg      = on_msg;
	myqtt_mutex_unlock (&conn->op_mutex);

	return;
}

void __myqtt_conn_get_next_on_close (MyQttConn * conn, axlPointer data)
{
	MyQttAsyncQueue * queue = data;

	/* notify connection close */
	myqtt_async_queue_push (queue, INT_TO_PTR (-1));

	return;
}

void __myqtt_conn_queue_msgs (MyQttCtx * ctx, MyQttConn * conn, MyQttMsg * msg, axlPointer data)
{
	MyQttAsyncQueue * queue = data;

	/* notify message received */
	myqtt_msg_ref (msg);
	myqtt_async_queue_push (queue, msg);

	return;
}

/** 
 * @brief Allows to implement a simple synchronous wait for the next message (PUBLISH) received on the provided connection limiting that wait to the provided timeout (in milliseconds).
 *
 * @param conn The connection where the wait operation for the next (\ref MYQTT_PUBLISH) message will take place.
 *
 * @param timeout The timeout to limit the operation. If it is set to 0 it will wait for ever. This value is provided in milliseconds (1000 ms = 1sec)
 *
 * @return A reference to the message received or NULL if it fails or
 * timeout was reached. The function also returns NULL when the
 * connection is closed during the wait operation. In the case the
 * function returns a \ref MyQttMsg reference you must call to \ref
 * myqtt_msg_unref after finishing with it.
 *
 * NOTE: the function will replace the \ref MyQttOnMsgReceived handler
 * that was previously configured by \ref myqtt_conn_set_on_msg but it
 * will be restored once the function finishes.
 *
 * <b>Notes about using this function to receive a particular message</b>
 *
 * This function is useful for simple generic blocking wait
 * operation. However, it is not recommended when expected to receive
 * a particular message because a particular \ref myqtt_conn_pub done
 * before. Here is why:
 *
 * If you \ref myqtt_conn_pub to the server, and then call to this
 * function (expecting to receive a reply), you may lose it because at
 * the time you are about to call to \ref myqtt_conn_get_next, the
 * thread planner may give priority to notify incoming message and,
 * because you didn't set any on message handler, that message is lost
 * and hence, calling to \ref myqtt_conn_get_next will not return it.
 *
 * So, the recommended solution to work out this situation is to call
 * first to setup a \ref MyQttOnMsgReceived to ensure that reception
 * mechanism is already in place when you issue the PUB (\ref
 * myqtt_conn_pub) message. 
 *
 * Here is a short example:
 *
 * \code
 * // create queue 
 * queue = myqtt_async_queue_new ();
 * myqtt_conn_set_on_msg (conn, test_03_on_message, queue);
 *
 * // call to get client identifier 
 * if (! myqtt_conn_pub (conn, "myqtt/admin/get-tls-status", "", 0, MYQTT_QOS_0, axl_false, 0)) {
 *     printf ("ERROR: unable to publish message to get client identifier..\n");
 *     return axl_false;
 * } 
 *
 * // push a message to ask for client id identifier 
 * msg   = myqtt_async_queue_timedpop (queue, 10000000);
 * myqtt_async_queue_unref (queue);
 * \endcode
 * 
 *
 */
MyQttMsg *             myqtt_conn_get_next (MyQttConn * conn, long timeout)
{
	MyQttOnMsgReceived    old_handler;
	axlPointer            old_data;
	MyQttAsyncQueue     * queue;
	MyQttMsg            * msg;

	if (! myqtt_conn_is_ok (conn, axl_false))
		return NULL;

	/* have a reference to previous handlers */
	old_handler = conn->on_msg;
	old_data    = conn->on_msg_data;

	/* install connection close */
	queue             = myqtt_async_queue_new ();
	myqtt_conn_set_on_close (conn, axl_false, __myqtt_conn_get_next_on_close, queue);

	/* now install new handlers */
	myqtt_mutex_lock (&conn->op_mutex);
	conn->on_msg_data = queue;
	conn->on_msg      = __myqtt_conn_queue_msgs;
	myqtt_mutex_unlock (&conn->op_mutex);

	/* now wait */
	if (timeout > 0)
		msg = myqtt_async_queue_timedpop (queue, timeout * 1000);
	else
		msg = myqtt_async_queue_pop (queue);

	/* remove on close handler */
	myqtt_conn_remove_on_close (conn, __myqtt_conn_get_next_on_close, queue);

	/* detect values reported by connection close */
	if (PTR_TO_INT (msg) == -1)
		msg = NULL;

	/* restore handlers */
	myqtt_mutex_lock (&conn->op_mutex);
	conn->on_msg_data = old_data;
	conn->on_msg      = old_handler;
	myqtt_mutex_unlock (&conn->op_mutex);

	/* release queue */
	myqtt_async_queue_unref (queue);

	return msg;
}


/** 
 * @brief Allows to set a new on close handler to be executed only
 * once the connection is being closed.
 *
 * The handler provided on this function is called once the connection
 * provided is closed. This is useful to detect connection broken or "broken pipe".
 *
 * @param connection The connection being closed
 *
 * @param on_close_handler The handler that will be executed.
 *
 * @param data User defined data to be passed to the handler.
 *
 * @param insert_last Allows to configure if the handler should be inserted at the last position.
 *
 * NOTE: handler configured will be skipped in the case \ref MyQttCtx
 * hosting the provided connection is being closed (a call to \ref
 * myqtt_exit_ctx was done). 
 * 
 */
void myqtt_conn_set_on_close       (MyQttConn         * connection,
				    axl_bool            insert_last,
				    MyQttConnOnClose    on_close_handler,
				    axlPointer          data)
{

	MyQttConnOnCloseData * handler;
#if defined(ENABLE_MYQTT_LOG)
	MyQttCtx                   * ctx;
#endif

	/* check reference received */
	if (connection == NULL || on_close_handler == NULL)
		return;

	/* lock during the operation */
	myqtt_mutex_lock (&connection->handlers_mutex);

	/* init on close list on demand */
	if (connection->on_close_full == NULL) {
		connection->on_close_full  = axl_list_new (axl_list_always_return_1, axl_free);
		if (connection->on_close_full == NULL) {
			myqtt_mutex_unlock (&connection->handlers_mutex);
			return;
		} /* end if */
	}

	/* save handler defined */
	handler          = axl_new (MyQttConnOnCloseData, 1);
	handler->handler = on_close_handler;
	handler->data    = data;

	/* store the handler */
	if (insert_last)
		axl_list_append (connection->on_close_full, handler);
	else
		axl_list_prepend (connection->on_close_full, handler);
#if defined(ENABLE_MYQTT_LOG)
	ctx = connection->ctx;
	myqtt_log (MYQTT_LEVEL_DEBUG, "on close full handlers %d on conn-id=%d, handler added %p (added %s)",
		    axl_list_length (connection->on_close_full), connection->id, handler, insert_last ? "last" : "first");
#endif

	/* unlock now it is done */
	myqtt_mutex_unlock (&connection->handlers_mutex);
	/* returns previous handler */
	return;
}

/** 
 * @brief Allows to setup a handler that is called when a message is
 * completely sent by the engine (myqtt sequencer).
 *
 * @param conn The connection where the operation will take place.
 *
 * @param on_msg_sent The handler to call
 *
 * @param on_msg_sent_data A user defined pointer to be passed in into
 * on_msg_sent handler.
 */
void                   myqtt_conn_set_on_msg_sent    (MyQttConn         * conn,
						      MyQttOnMsgSent      on_msg_sent,
						      axlPointer          on_msg_sent_data)
{
	if (conn == NULL)
		return;

	/* configure handlers */
	conn->on_msg_sent      = on_msg_sent;
	conn->on_msg_sent_data = on_msg_sent_data;

	return;
}

/** 
 * @brief Allows to configure a handler that is called in the case a
 * reconnect operation took place and finished without error.
 *
 * @param conn The connection that was reconnected.
 *
 * @param on_reconnect The handler that will be called on reconnect.
 *
 * @param on_reconnect_data A user defined pointer that is passed in
 * into the handler configured when it is called.
 */
void                   myqtt_conn_set_on_reconnect   (MyQttConn         * conn,
						      MyQttOnReconnect    on_reconnect,
						      axlPointer          on_reconnect_data)
{
	if (conn == NULL)
		return;
	
	/* set on reconnect handler */
	conn->on_reconnect      = on_reconnect;
	conn->on_reconnect_data = on_reconnect_data;

	return;
}



/** 
 * @brief Extended version for \ref myqtt_conn_set_on_close
 * handler which also support receiving a user data pointer.
 *
 * See \ref myqtt_conn_set_on_close for more details. This
 * function could be called several times to install several handlers.
 *
 * Once a handler is installed, you can use \ref
 * myqtt_conn_remove_on_close to uninstall the handler if
 * it is required to avoid getting more notifications.
 * 
 * @param connection The connection that is required to get close
 * notifications.
 *
 * @param on_close_handler The handler to be executed once the event
 * is produced.
 *
 * @param insert_last Allows to configure if the handler should be inserted at the last position.
 *
 * @param data User defined data to be passed to the handler.
 * 
 */
void                    myqtt_conn_set_on_close_full2  (MyQttConn          * connection,
							MyQttConnOnClose     on_close_handler,
							axl_bool             insert_last,
							axlPointer           data)
{
	MyQttConnOnCloseData * handler;
#if defined(ENABLE_MYQTT_LOG)
	MyQttCtx                   * ctx;
#endif

	/* check reference received */
	if (connection == NULL || on_close_handler == NULL)
		return;

	/* lock during the operation */
	myqtt_mutex_lock (&connection->handlers_mutex);

	/* init on close list on demand */
	if (connection->on_close_full == NULL) {
		connection->on_close_full  = axl_list_new (axl_list_always_return_1, axl_free);
		if (connection->on_close_full == NULL) {
			myqtt_mutex_unlock (&connection->handlers_mutex);
			return;
		} /* end if */
	}

	/* save handler defined */
	handler          = axl_new (MyQttConnOnCloseData, 1);
	handler->handler = on_close_handler;
	handler->data    = data;

	/* store the handler */
	if (insert_last)
		axl_list_append (connection->on_close_full, handler);
	else
		axl_list_prepend (connection->on_close_full, handler);
#if defined(ENABLE_MYQTT_LOG)
	ctx = connection->ctx;
	myqtt_log (MYQTT_LEVEL_DEBUG, "on close full handlers %d on conn-id=%d, handler added %p (added %s)",
		    axl_list_length (connection->on_close_full), connection->id, handler, insert_last ? "last" : "first");
#endif

	/* unlock now it is done */
	myqtt_mutex_unlock (&connection->handlers_mutex);

	/* finish */
	return;
}

/** 
 * @brief Allows to uninstall a particular handler installed to get
 * notifications about the connection close.
 *
 * If the handler is found, it will be uninstalled and the data
 * associated to the handler will be returned.
 * 
 * @param connection The connection where the uninstall operation will
 * be performed.
 *
 * @param on_close_handler The handler to uninstall.
 *
 * @param data In order to remove the handler you must provide
 * the same pointer data as provided on handler installation. This is
 * done to avoid removing a handler that was installed twice but with
 * different contexts. Think about a function that install the same
 * handler but with different data. The appropriate handler to
 * uninstall and its associated data can only be matched this way.
 * 
 * @return axl_true if the function uninstalled the handler otherwise
 * axl_false is returned.
 */
axl_bool   myqtt_conn_remove_on_close (MyQttConn          * connection, 
				       MyQttConnOnClose     on_close_handler,
				       axlPointer           data)
{
	int                           iterator;
	MyQttConnOnCloseData * handler;

	/* check reference received */
	v_return_val_if_fail (connection, axl_false);
	v_return_val_if_fail (on_close_handler, axl_false);

	/* look during the operation */
	myqtt_mutex_lock (&connection->handlers_mutex);

	/* remove by pointer */
	iterator = 0;
	while (iterator < axl_list_length (connection->on_close_full)) {

		/* get a reference to the handler */
		handler = axl_list_get_nth (connection->on_close_full, iterator);
		
		if ((on_close_handler == handler->handler) && (data == handler->data)) {

			/* remove by pointer */
			axl_list_remove_ptr (connection->on_close_full, handler);

			/* unlock */
			myqtt_mutex_unlock (&connection->handlers_mutex);

			return axl_true;
			
		} /* end if */

		/* update the iterator */
		iterator++;
		
	} /* end while */
	
	/* unlock */
	myqtt_mutex_unlock (&connection->handlers_mutex);

	return axl_false;
}

/** 
 * @internal
 * @brief Allows to invoke current receive handler defined by \ref MyQttReceive.
 * 
 * This function is actually no useful for MyQtt Library consumer. It
 * is used by the library to actually perform the invocation in a
 * transparent way.
 *
 * @param connection The connection where the receive handler will be invoked.
 * @param buffer     The buffer to be received.
 * @param buffer_len The buffer size to be received.
 * 
 * @return How many data was actually received or -1 if it fails. The
 * function returns -2 if the connection isn't still prepared to read
 * data.
 */
int                 myqtt_conn_invoke_receive         (MyQttConn        * connection,
						       unsigned char    * buffer,
						       int                buffer_len)
{
	/* return -1 */
	if (connection == NULL || buffer == NULL || connection->receive == NULL)
		return -1;

	return connection->receive (connection, buffer, buffer_len);
}

/** 
 * @internal
 * @brief Allows to invoke current send handler defined by \ref MyQttSend.
 * 
 * This function is actually not useful for MyQtt Library
 * consumers. It is used by the library to perform the invocation of the send handler.
 * 
 * @param connection The connection where the invocation of the send handler will be performed.
 * @param buffer     The buffer data to be sent.
 * @param buffer_len The buffer data size.
 * 
 * @return How many data was actually sent or -1 if it fails. -2 is
 * returned if the connection isn't still prepared to write or send
 * the data.
 */
int                 myqtt_conn_invoke_send            (MyQttConn           * connection,
						       const unsigned char * buffer,
						       int                   buffer_len)
{
	if (connection == NULL || buffer == NULL || ! myqtt_conn_is_ok (connection, axl_false))
		return -1;

	return connection->send (connection, buffer, buffer_len);
}

/** 
 * @brief Allows to disable sanity socket check, by default enabled.
 *
 * While allocating underlying socket descriptors, at the connection
 * creation using (<b>socket</b> system call) \ref
 * myqtt_conn_new, it could happen that the OS assign a socket
 * descriptor used for standard task like: stdout(0), stdin(1),
 * stderr(2).
 * 
 * This happens when the user app issue a <b>close() </b> call over
 * the previous standard descriptors (<b>0,1,2</b>) causing myqtt to
 * allocate a reserved descriptor with some funny behaviours.
 * 
 * Starting from previous context, any user app call issuing a console
 * print will cause to automatically send to the remote site the
 * message printed, bypassing all myqtt mechanisms.
 * 
 * Obviously, this is an odd situation and it is not desirable. By
 * default, MyQtt Library includes a sanity check to just reject
 * creating a MyQtt Conn with an underlying socket descriptor
 * which could cause applications problems.
 *
 * You can disable this sanity check using this function.
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @param enable According to the value, the sanity check will be
 * enabled or disabled.
 * 
 */
void                myqtt_conn_sanity_socket_check (MyQttCtx * ctx, axl_bool      enable)
{
	/* do not perform any operation if a null context is
	 * received */
	if (ctx == NULL)
		return;

	ctx->connection_enable_sanity_check  = enable;
	return;
}


/** 
 * @internal Allows to init the connection module using the provided
 * context (\ref MyQttCtx).
 * 
 * @param ctx The myqtt context that is going to be initialised (only the connection module)
 */
void                myqtt_conn_init                   (MyQttCtx        * ctx)
{

	v_return_if_fail (ctx);

	/**** myqtt_conn.c: init connection module */
	if (ctx->connection_id == 0) {
		/* only init connection id in case it is 0 (clean
		 * start) leaving the value as is if we are in a fork
		 * operation, so child must keep connection ids as
		 * they were created at the parent before forking */
		ctx->connection_id                = 1;
	}
	ctx->connection_enable_sanity_check       = axl_true;
	ctx->connection_std_timeout               = 60000000;

	myqtt_mutex_create (&ctx->connection_hostname_mutex);

	if (ctx->connection_hostname == NULL)
		ctx->connection_hostname  = axl_hash_new (axl_hash_string, axl_hash_equal_string);
	return;
}

/** 
 * @internal Allows to terminate the myqtt connection module context
 * on the provided myqtt context.
 * 
 * @param ctx The \ref MyQttCtx to cleanup
 */
void                myqtt_conn_cleanup                (MyQttCtx        * ctx)
{
	v_return_if_fail (ctx);

	/**** myqtt_conn.c: cleanup ****/
	myqtt_mutex_destroy (&ctx->connection_hostname_mutex);

	axl_hash_free (ctx->connection_hostname);
	ctx->connection_hostname = NULL;

	return;
}

/** 
 * @brief Allows to get maximum segment size negotiated.
 * 
 * @param connection The connection where to get maximum segment size.
 * 
 * @return The maximum segment size or -1 if fails. The function is
 * still not portable since the Microsoft Windows API do not allow
 * getting TCP maximum segment size.
 */
int                myqtt_conn_get_mss                (MyQttConn * connection)
{
#if defined(AXL_OS_WIN32)
	/* no support */
	return -1;
#else
	/* unix flavors */
	socklen_t            optlen;
	long              max_seg_size;

	v_return_val_if_fail (connection, -1);

	/* clear values */
	optlen       = sizeof (long);
	max_seg_size = 0;
	
	/* call to get socket option */
	if (getsockopt (connection->session, IPPROTO_TCP, TCP_MAXSEG, &max_seg_size, &optlen) == 0) {
		return max_seg_size;
	}

	/* return value found */
	return -1;
#endif
}

/** 
 * @internal Function used to check if we have reached our socket
 * creation limit to avoid exhausting it. The idea is that we need to
 * have at least one bucket free before limit is reached so we can
 * still empty the listeners backlog to close them (accept ()).
 *
 * @return axl_true in the case the limit is not reached, otherwise
 * axl_false is returned.
 */
axl_bool myqtt_conn_check_socket_limit (MyQttCtx * ctx, MYQTT_SOCKET socket_to_check)
{
	int   soft_limit, hard_limit;
	MYQTT_SOCKET temp;

	/* create a temporal socket */
	temp = socket (AF_INET, SOCK_STREAM, 0);
	if (temp == MYQTT_INVALID_SOCKET) {
		/* uhmmn.. seems we reached our socket limit, we have
		 * to close the connection to avoid keep on iterating
		 * over the listener connection because its backlog
		 * could be filled with sockets we can't accept */
		shutdown (socket_to_check, SHUT_RDWR);
		myqtt_close_socket (socket_to_check);

		/* get values */
		myqtt_conf_get (ctx, MYQTT_SOFT_SOCK_LIMIT, &soft_limit);
		myqtt_conf_get (ctx, MYQTT_HARD_SOCK_LIMIT, &hard_limit);
		
		myqtt_log (MYQTT_LEVEL_CRITICAL, 
			    "drooping socket connection, reached process limit: soft-limit=%d, hard-limit=%d\n",
			    soft_limit, hard_limit);
		return axl_false; /* limit reached */

	} /* end if */
	
	/* close temporal socket */
	myqtt_close_socket (temp);

	return axl_true; /* connection check ok */
}

/**
 * @internal Function used to record and error and then shutdown the
 * connection in the same step.
 *
 * @param conn The connection where the error was detected.
 * @param message The message to report
 * 
 */
void                __myqtt_conn_shutdown_and_record_error (MyQttConn     * conn,
							    MyQttStatus     status,
							    const char    * message,
							    ...)
{
	va_list     args;
	char      * _msg;
	char      * aux;
#if defined(ENABLE_MYQTT_LOG)
	MyQttCtx * ctx;
#endif
	if (conn == NULL)
		return;

#if defined(ENABLE_MYQTT_LOG)
	/* get reference */
	ctx = conn->ctx;
#endif

	/* log error */
	if (status != MyQttOk && status != MyQttConnectionCloseCalled) {

		/* prepare message */
		va_start (args, message);
		_msg = axl_strdup_printfv (message, args);
		va_end (args);

		/* create message to be logged with additional information */
		aux = axl_strdup_printf ("%s conn-id=%d from=%s:%s username=%s client-id=%s",
					 _msg,
					 myqtt_conn_get_id (conn),
					 conn->host, conn->port, 
					 conn->username ? conn->username : "",
					 conn->client_identifier ? conn->client_identifier : "");
		axl_free (_msg);
		_msg = aux;

		myqtt_log (MYQTT_LEVEL_CRITICAL, _msg);
		
		/* release message */
		axl_free (_msg);
	} 
	
	/* shutdown connection */
	myqtt_conn_shutdown (conn);

	return;
}

/* @} */

