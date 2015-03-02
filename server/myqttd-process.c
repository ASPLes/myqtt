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

#include <myqttd-process.h>

/* include private headers */
#include <myqttd-ctx-private.h>
#include <myqtt-conn-private.h>

/* include signal handler: SIGCHLD */
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/un.h>


/** 
 * @internal Path to the myqttd binary path that used to load current
 * instance.
 */
const char * myqttd_bin_path         = NULL;

/** 
 * @internal Optional command prefix used to create child process in
 * myqttd.
 */
const char * myqttd_child_cmd_prefix = NULL;

extern axl_bool __myqttd_module_no_unmap;

/** 
 * @internal Macro used to block sigchild and lock the mutex
 * associated to child processes.
 */
#define MYQTTD_PROCESS_LOCK_CHILD() do {                  \
	myqttd_signal_block (ctx, SIGCHLD);        \
	myqtt_mutex_lock (&ctx->child_process_mutex); \
} while (0)

/** 
 * @internal Macro used to block sigchild and lock the mutex
 * associated to child processes.
 */
#define MYQTTD_PROCESS_UNLOCK_CHILD() do {                  \
	myqtt_mutex_unlock (&ctx->child_process_mutex); \
	myqttd_signal_unblock (ctx, SIGCHLD);        \
} while (0)

/** 
 * @internal Function used to init process module (for its internal
 * handling).
 *
 * @param ctx The myqttd context where the initialization will
 * take place.
 */
void myqttd_process_init         (MyQttdCtx * ctx, axl_bool reinit)
{
	/* check for reinit operation */
	if (reinit && ctx->child_process) {
		axl_hash_free (ctx->child_process);
		ctx->child_process = NULL;
	}

	/* create the list of childs opened */
	if (ctx->child_process == NULL)
		ctx->child_process = axl_hash_new (axl_hash_int, axl_hash_equal_int);

	/* init mutex */
	myqtt_mutex_create (&ctx->child_process_mutex);
	return;
}


axlPointer __myqttd_process_finished (MyQttdCtx * ctx)
{
	MyQttCtx        * myqtt_ctx = ctx->myqtt_ctx;
	int                delay      = 300000; /* 300ms */
	int                tries      = 30; 

	while (tries > 0) {

		/* check if the conn manager has connections watched
		 * at this time */
		myqtt_mutex_lock (&ctx->conn_mgr_mutex);
		if (axl_hash_items (ctx->conn_mgr_hash) > 0) {
			msg ("CHILD: cancelled child process termination because new connections are now handled, tries=%d, delay=%d, reader connections=%d, tbc conn mgr=%d",
			      tries, delay, 
			      myqtt_reader_connections_watched (myqtt_ctx), 
			      axl_hash_items (ctx->conn_mgr_hash));
			myqtt_mutex_unlock (&ctx->conn_mgr_mutex);
			return NULL;
		} /* end if */
		myqtt_mutex_unlock (&ctx->conn_mgr_mutex);
		
		/* finish current thread if myqttd is existing */
		if (ctx->is_exiting) {
			msg ("CHILD: Found myqttd existing, finishing child process termination thread signaled due to myqtt reader stop");
			return NULL;
		}

		if (tries < 10) {
			/* reduce tries, wait */
			msg ("CHILD: delay child process termination to ensure the parent has no pending connections, tries=%d, delay=%d, reader connections=%d, tbc conn mgr=%d, is-existing=%d",
			     tries, delay,
			     myqtt_reader_connections_watched (myqtt_ctx), 
			     axl_hash_items (ctx->conn_mgr_hash),
			     ctx->is_exiting);
		} /* end if */

		tries--;
		myqttd_sleep (ctx, delay);
	}

	/* unlock waiting child */
	msg ("CHILD: calling to unlock due to myqtt reader stoped (no more connections to be watched): %p (myqtt refs: %d)..",
	     ctx, myqtt_ctx_ref_count (ctx->myqtt_ctx));
	/* unlock the current listener */
	myqtt_listener_unlock (MYQTTD_MYQTT_CTX (ctx));
	return NULL;
}


void myqttd_process_check_for_finish (MyQttdCtx * ctx)
{
	MyQttThread                thread;

	/* do not implement any notification if the myqttd process
	   is itself finishing */
	if (ctx->is_exiting) {
	        msg ("CHILD: process already called to finish myqttd..");
		return;
	}

	/* check if the child process was configured with a reuse
	   flag, if not, notify exist right now */
	/* if (! ctx->child->ppath->reuse) { */
	/*	        msg ("CHILD: unlocking listener to finish: %p..", ctx->myqtt_ctx); */
		/* so it is a child process without reuse flag
		   activated, exit now */
	/*		myqtt_listener_unlock (MYQTTD_MYQTT_CTX (ctx)); */
	/* return; */
	/* } */

	/* reached this point, the child process has reuse = yes which
	   means we have to ensure we don't miss any connection which
	   is already accepted on the parent but still not reached the
	   child: the following creates a thread to unlock the myqtt
	   reader, so he can listen for new registration while we do
	   the wait code for new connections... */

	/* create data to be notified */
	msg ("CHILD: initiated child termination (checking for rogue connections..");
	if (! myqtt_thread_create (&thread,
				    (MyQttThreadFunc) __myqttd_process_finished,
				    ctx, MYQTT_THREAD_CONF_DETACHED, MYQTT_THREAD_CONF_END)) {
		error ("Failed to create thread to watch reader state and notify process termination (code %d) %s",
		       errno, myqtt_errno_get_last_error ());
	}
	return;
}

#if defined(AXL_OS_UNIX)
int __myqttd_process_local_unix_fd (const char *path, axl_bool is_parent, MyQttdCtx * ctx)
{
	int                  _socket     = -1;
	int                  _aux_socket = -1;
	struct sockaddr_un   socket_name = {0};
	int                  tries       = 10;
	int                  delay       = 100;

	/* configure socket name to connect to (or to bind to) */
	memset (&socket_name, 0, sizeof (struct sockaddr_un));
	strcpy (socket_name.sun_path, path);
	socket_name.sun_family = AF_UNIX;

	/* create socket and check result */
	_socket = socket (AF_UNIX, SOCK_STREAM, 0);
	if (_socket == -1) {
	        error ("%s: Failed to create local socket to hold connection, reached limit?, errno: %d, %s", 
		       is_parent ? "PARENT" : "CHILD", errno, myqtt_errno_get_error (errno));
	        return -1;
	}

	/* if child, wait until it connects to the child */
	if (is_parent) {
		while (tries > 0) {
			if (connect (_socket, (struct sockaddr *)&socket_name, sizeof (socket_name))) {
				if (errno == 107 || errno == 111 || errno == 2) {
					/* implement a wait operation */
					myqttd_sleep (ctx, delay);
				} else {
					error ("%s: Unexpected error found while creating child control connection: (errno: %d) %s", 
					       is_parent ? "PARENT" : "CHILD", errno, myqtt_errno_get_last_error ());
					break;
				} /* end if */
			} else {
				/* connect == 0 : connected */
				break;
			}

			/* reduce tries */
			tries--;
			delay = delay * 2;

		} /* end if */

		/* drop an ok log */
		msg ("%s: local socket (%s) = %d created OK", is_parent ? "PARENT" : "CHILD", path, _socket);

		return _socket;
	}

	/* child handling */
	/* unlink for previously created file */
	unlink (path);

	/* check path */
	if (strlen (path) >= sizeof (socket_name.sun_path)) {
	        myqtt_close_socket (_socket);
	        error ("%s: Failed to create local socket to hold connection, path is bigger that limit (%d >= %d), path: %s", 
		       is_parent ? "PARENT" : "CHILD", (int) strlen (path), (int) sizeof (socket_name.sun_path), path);
		return -1;
	}
	umask (0077);
	if (bind (_socket, (struct sockaddr *) &socket_name, sizeof(socket_name))) {
	        error ("%s: Failed to create local socket to hold conection, bind function failed with error: %d, %s", 
		       is_parent ? "PARENT" : "CHILD", errno, myqtt_errno_get_last_error ());
		myqtt_close_socket (_socket);
		return -1;
	} /* end if */

	/* listen */
	if (listen (_socket, 1) < 0) {
	        myqtt_close_socket (_socket);
		error ("%s: Failed to listen on socket created, error was (code: %d) %s", 
		       is_parent ? "PARENT" : "CHILD", errno, myqtt_errno_get_last_error ());
		return -1;
	}

	/* now call to accept connection from parent */
	_aux_socket = myqtt_listener_accept (_socket);
	if (_aux_socket < 0) 
	         error ("%s: Failed to create local socket to hold conection, listener function failed with error: %d, %s (socket value is %d)", 
			is_parent ? "PARENT" : "CHILD", errno, myqtt_errno_get_last_error (), _aux_socket);
	myqtt_close_socket (_socket);

	/* unix semantic applies, now remove the file socket because
	 * now having opened a socket, the reference will be removed
	 * after this is closed. */
	unlink (path);

	/* drop an ok log */
	msg ("%s: local socket (%s) = %d created OK", is_parent ? "PARENT" : "CHILD", path, _socket);	
	
	return _aux_socket;
}
#endif

axl_bool __myqttd_process_create_child_connection (MyQttdChild * child)
{
	MyQttdCtx    * ctx = child->ctx;
	struct sockaddr_un socket_name = {0};
	int    iterator = 0;

	/* configure socket name to connect to (or to bind to) */
	memset (&socket_name, 0, sizeof (struct sockaddr_un));
	strcpy (socket_name.sun_path, child->socket_control_path);
	socket_name.sun_family = AF_UNIX;

	/* create the client connection making the child to do the
	   bind (creating local file socket using child process
	   permissions)  */
	myqttd_sleep (ctx, 1000);
	while (iterator < 25) {
		/* check if the file exists ... */
		if (myqtt_support_file_test (child->socket_control_path, FILE_EXISTS)) {
			/* create child connection */
			child->child_connection = __myqttd_process_local_unix_fd (child->socket_control_path, axl_true, ctx);
			/* check child creation status */
			if (child->child_connection > 0)
				return axl_true;
		} /* end if */
			
		/* check if the file exists before returning error returned
		   by previous function */
		if (! myqtt_support_file_test (child->socket_control_path, FILE_EXISTS)) 
			wrn ("PARENT: child still not ready, socket control path isn't found: %s", child->socket_control_path);

		/* next position but wait a bit */
		iterator++;
		
		wrn ("PARENT: child is still not ready, waiting a bit...");
		myqttd_sleep (ctx, 200000);
	}
	  

	return (child->child_connection > 0);
}

axl_bool __myqttd_process_create_parent_connection (MyQttdChild * child)
{
	/* create the client connection making the child to do the
	   bind (creating local file socket using child process
	   permissions)  */
#if ! defined(SHOW_FORMAT_BUGS)
	MyQttdCtx * ctx = child->ctx;
#endif

	msg ("CHILD: process creating control connection: %s..", child->socket_control_path);
	child->child_connection = __myqttd_process_local_unix_fd (child->socket_control_path, axl_false, child->ctx);
	msg ("CHILD: child control connection status: %d", child->child_connection);
	return (child->child_connection > 0);
}

/** 
 * @internal Function used to send the provided socket to the provided
 * child.
 *
 * @param socket The socket to be send.
 *
 * @param child The child child where to send the new socket.
 *
 * @param ancillary_data Data to be sent along with the
 * socket. Standard data is "s" to close that socket in the child
 * process and "n" to notify a new connection.
 *
 * @return If the socket was sent.
 */
axl_bool myqttd_process_send_socket (MYQTT_SOCKET     socket, 
					 MyQttdChild * child, 
					 const char      * ancillary_data, 
					 int               size)
{
	struct msghdr        msg;
	int                * int_ptr;
	char                 ccmsg[CMSG_SPACE(sizeof(socket))];

	struct cmsghdr     * cmsg;
	struct iovec         vec; 

	/* send at least one byte */
	const char         * str = ancillary_data ? ancillary_data : "#"; 
	int                  rv;
#if ! defined(SHOW_FORMAT_BUGS)
	MyQttdCtx      * ctx = child->ctx;
#endif

	/* clear structures */
	memset (&msg, 0, sizeof (struct msghdr));

	/* configure destination */
	msg.msg_namelen        = 0;

	vec.iov_base   = (char *) str;
	vec.iov_len    = ancillary_data == NULL ? 1 : size;
	msg.msg_iov    = &vec;
	msg.msg_iovlen = 1;

	msg.msg_control        = ccmsg;
	msg.msg_controllen     = sizeof(ccmsg);

	cmsg = CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_level       = SOL_SOCKET;
	cmsg->cmsg_type        = SCM_RIGHTS;
	cmsg->cmsg_len         = CMSG_LEN(sizeof(socket));
	int_ptr                = (int*)CMSG_DATA(cmsg);
	(*int_ptr)             = socket;

	msg.msg_controllen     = cmsg->cmsg_len;
	msg.msg_flags = 0;
	
	rv = (sendmsg (child->child_connection, &msg, 0) != -1);
	if (rv)  {
		msg ("PARENT: Socket %d sent to child via %d (ancillary data: %s), closing (status: %d)..", 
		     socket, child->child_connection, ancillary_data, rv);
		/* close the socket  */
		myqtt_close_socket (socket); 
	} else {
		error ("PARENT: Failed to send socket, error code %d, textual was: %s", rv, myqtt_errno_get_error (errno));
	}
	
	return rv;
}

/** 
 * @brief Allows to receive a socket from the parent on the child
 * provided. In the case the function works, the socket references is
 * updated with the socket descriptor.
 *
 * @param socket A reference where to set socket received. It cannot be NULL.
 * @param child Child receiving the socket. 
 *
 * @return The function returns axl_false in the case of failure,
 * otherwise axl_true is returned.
 */
axl_bool myqttd_process_receive_socket (MYQTT_SOCKET    * _socket, 
					    MyQttdChild  * child, 
					    char            ** ancillary_data, 
					    int              * size)
{
	struct msghdr    msg;
	struct iovec     iov;
	char             buf[1024];
	int              status;
	char             ccmsg[CMSG_SPACE(sizeof(int))];
	struct           cmsghdr *cmsg;
	MyQttdCtx  * ctx;

	/* variables for error reporting */
	MYQTT_SOCKET    temp;
	int              soft_limit, hard_limit;

	int            * int_ptr;
	
	v_return_val_if_fail (_socket && child, axl_false);

	/* get context reference */
	ctx = child->ctx;
	
	iov.iov_base = buf;
	iov.iov_len = 1023;

	memset (&msg, 0, sizeof (struct msghdr));	
	msg.msg_name       = 0;
	msg.msg_namelen    = 0;
	msg.msg_iov        = &iov;
	msg.msg_iovlen     = 1;
	msg.msg_control    = ccmsg;
	msg.msg_controllen = sizeof(ccmsg); 
	
	status = recvmsg (child->child_connection, &msg, 0);
	if (status == -1) {
		error ("Failed to receive socket, recvmsg failed, error was: (code %d) %s",
		       errno, myqtt_errno_get_last_error ());
		(*_socket) = -1;
		return axl_false;
	} /* end if */

	cmsg = CMSG_FIRSTHDR(&msg);
	if (cmsg == NULL) {
		/* check first if we have support to create more sockets */
		temp = socket (AF_INET, SOCK_STREAM, 0);
		if (temp == MYQTT_INVALID_SOCKET) {
			/* we have received our socket limit */
			myqtt_conf_get (MYQTTD_MYQTT_CTX(ctx), MYQTT_SOFT_SOCK_LIMIT, &soft_limit);
			myqtt_conf_get (MYQTTD_MYQTT_CTX(ctx), MYQTT_HARD_SOCK_LIMIT, &hard_limit);
		
			error ("Unable to receive socket from parent, droping socket connection, reached process limit: soft-limit=%d, hard-limit=%d\n",
			       soft_limit, hard_limit);
		} else {
			error ("Received empty control message from parent (status: %d), unable to receive socket (code %d): %s",
			       status, errno, myqtt_errno_get_last_error ());
			error ("Reached socket process limit?");
		} /* end if */

		(*_socket) = -1;
		if (ancillary_data)
			(*ancillary_data) = NULL;
		if (size)
			(*size) = 0;
		return axl_false;
	}

	if (!cmsg->cmsg_type == SCM_RIGHTS) {
		error ("Unexpected control message of unknown type %d, failed to receive socket", 
		       cmsg->cmsg_type);
		(*_socket) = -1;
		return axl_false;
	}

	/* report data received on the message */
	if (size)
		(*size)   = status;
	if (ancillary_data) {
		(*ancillary_data) = axl_new (char, status + 1);
		memcpy (*ancillary_data, iov.iov_base, status);
	}

	/* set socket received */
	int_ptr    = (int *) CMSG_DATA(cmsg);
	(*_socket) = (*int_ptr);

	msg ("Process received socket %d, checking to send received signal...", (*_socket));
	return axl_true;
}

/** 
 * @internal Function used to create an string that represents the
 * status of the MQTT session so the receiving process can reconstruct
 * the connection.
 */
char * myqttd_process_connection_status_string (axl_bool          handle_reply,
						const char      * serverName,
						int               has_tls,
						int               fix_server_name,
						const char      * remote_host,
						const char      * remote_port,
						const char      * remote_host_ip,
						/* the following must be the last parameter */
						int               skip_conn_recover)
{
	return axl_strdup_printf ("n%d;-;%d;-;%s;-;%s;-;%d;-;%s;-;%d;-;%d;-;%d;-;%d;-;%d;-;%d;-;%s;-;%s;-;%s;-;%d",
				  handle_reply,
				  serverName ? serverName : "",
				  has_tls, 
				  /* indicate the child that the serverName we are passing is the
				   * serverName that must be setup and do not accept any change about
				   * this serverName */
				  fix_server_name,
				  /* indication about the current
				   * remote host and current remote
				   * port to be restored on the parent. */
				  remote_host, 
				  remote_port,
				  remote_host_ip,
				  /* this must be the last always! */
				  skip_conn_recover);
}

void myqttd_process_send_connection_to_child (MyQttdCtx    * ctx, 
					      MyQttdChild  * child, 
					      MyQttConn    * conn, 
					      axl_bool       handle_reply, 
					      const char   * serverName, 
					      MyQttMsg     * msg)
{
	MYQTT_SOCKET        client_socket;
	char              * conn_status;

	/* build connection status string */
	conn_status = myqttd_process_connection_status_string (handle_reply, 
							       serverName,
							       conn->tls_on,
							       /* notify if we have to fix the serverName */
							       axl_cmp (serverName, myqtt_conn_get_server_name (conn)),
							       /* get host, port and host ip */
							       myqtt_conn_get_host (conn), myqtt_conn_get_port (conn), 
							       myqtt_conn_get_host_ip (conn),
							       /* if proxied, skip recover on child */
							       myqttd_conn_mgr_proxy_on_parent (conn));
	
	msg ("Sending connection to child already created, ancillary data ('%s') size: %d", conn_status, (int) strlen (conn_status));

	/* socket that is know handled by the child process */
	client_socket = myqtt_conn_get_socket (conn);
	myqtt_conn_set_close_socket (conn, axl_false);

	/* unwatch the connection from the parent to avoid receiving
	   more content which now handled by the child and unregister
	   from connection manager */
	myqtt_reader_unwatch_connection (CONN_CTX (conn), conn);

	/* send the socket descriptor to the child to avoid holding a
	   bucket in the parent */
	if (! myqttd_process_send_socket (client_socket, child, conn_status, strlen (conn_status))) {
		error ("PARENT: Something failed while sending socket (%d) to child pid %d already created, error (code %d): %s",
		       client_socket, child->pid, errno, myqtt_errno_get_last_error ());

		/* release ancillary data */
		axl_free (conn_status);

		/* close connection */
		myqtt_conn_shutdown (conn);
		return;
	}

	/* release ancillary data */
	axl_free (conn_status);
	
	/* terminate the connection */
	myqtt_conn_shutdown (conn);

	return;
}

axl_bool myqttd_process_send_proxy_connection_to_child (MyQttdCtx        * ctx, 
							MyQttdChild      * child, 
							MyQttConn        * conn, 
							MYQTT_SOCKET       client_socket,
							axl_bool           handle_reply, 
							const char       * serverName, 
							MyQttMsg         * msg)
{
	char    * conn_status;

	/* build connection status string */
	conn_status = myqttd_process_connection_status_string (handle_reply, 
							       serverName,
							       conn->tls_on,
							       /* notify if we have to fix the serverName */
							       axl_cmp (serverName, myqtt_conn_get_server_name (conn)),
							       /* host and port */
							       myqtt_conn_get_host (conn), myqtt_conn_get_port (conn),
							       myqtt_conn_get_host_ip (conn),
							       /* if proxied, skip recover on child */
							       myqttd_conn_mgr_proxy_on_parent (conn));
	
	msg ("PARENT: (PROXY) Sending connection to child already created, ancillary data ('%s') size: %d", conn_status, (int) strlen (conn_status));

	/* send the socket descriptor to the child to avoid holding a
	   bucket in the parent */
	if (! myqttd_process_send_socket (client_socket, child, conn_status, strlen (conn_status))) {
		error ("PARENT: Something failed while sending socket (%d) to child pid %d already created, error (code %d): %s",
		       client_socket, child->pid, errno, myqtt_errno_get_last_error ());

		/* release ancillary data */
		axl_free (conn_status);

		/* close connection */
		myqtt_conn_shutdown (conn);
		myqtt_close_socket (client_socket);
		return axl_false;
	}

	/* release ancillary data */
	axl_free (conn_status);

	/* report ok operation */
	return axl_true;
}

/** 
 * @internal Function used to implement common functions for a new
 * connection that is accepted into a child process.
 */
axl_bool __myqttd_process_common_new_connection (MyQttdCtx      * ctx,
						 MyQttConn      * conn,
						 axl_bool         handle_reply,
						 const char     * serverName,
						 MyQttMsg       * msg)
{
	axl_bool        result = axl_true;

	/* avoid handling any connection if myqttd is finishing */
	if (ctx->is_exiting) {
		error ("CHILD: Dropping connection=%d, myqttd is exiting..", myqtt_conn_get_id (conn));
		myqtt_conn_shutdown (conn);
		myqtt_conn_close (conn);
		return axl_false;
	}

	if (! myqtt_conn_is_ok (conn, axl_false)) {
		error ("CHILD: Connection id=%d is not working after notifying ppath selected handler..", myqtt_conn_get_id (conn));
		myqtt_conn_close (conn);
		return axl_false;
	}

	/* check if the connection was received during on connect
	   phase (myqtt_listener_set_on_accept_handler) or because a
	   channel start */
	if (! handle_reply) {
		/* set the connection to still finish MQTT greetings
		   negotiation phase */
		/* myqtt_conn_set_initial_accept (conn, axl_true); */
	}

	/* now finish and register the connection */
	myqtt_conn_set_close_socket (conn, axl_true);
	myqtt_reader_watch_connection (MYQTTD_MYQTT_CTX (ctx), conn);

	/* because the conn manager module has been initialized again,
	   register the connection handling by this process */
	/* myqttd_conn_mgr_register (ctx, conn); */

	/* check to handle start reply message */
	msg ("CHILD: Checking to handle start serverName=%s reply=%d at child=%d", serverName ? serverName : "", handle_reply, getpid ());
	if (handle_reply) {
		
	} /* end if */

	/* unref connection since it is registered */
	myqtt_conn_unref (conn, "myqttd process, conn registered");

	return result;
}

int __get_next_field (char * conn_status, int _iterator)
{
	int iterator = _iterator;

	while (conn_status[iterator]     != 0 && 
	       conn_status[iterator + 1] != 0 && 
	       conn_status[iterator + 2] != 0) {

		/* check if the separator was found (;-;) */
		if (conn_status [iterator] == ';' &&
		    conn_status [iterator + 1] == '-' &&
		    conn_status [iterator + 2] == ';') {
			conn_status[iterator] = 0;
			return iterator + 3;
		} /* end if */

		/* separator not found */
		iterator++;
	}

	return _iterator;
}

/** 
 * @internal Function that recovers the data to reconstruct a
 * connection state from the provided string.
 */
void     myqttd_process_connection_recover_status (char            * conn_status,
						   axl_bool        * handle_reply,
						   const char     ** serverName,
						   int             * fix_server_name,
						   const char     ** remote_host,
						   const char     ** remote_port,
						   const char     ** remote_host_ip,
						   int             * has_tls)
{
	int iterator = 0;
	int next;

	/* parse ancillary data */
	conn_status[1]  = 0;
	(*handle_reply) = atoi (conn_status);

	/* get next position */
	next               = __get_next_field (conn_status, iterator);
	(*serverName)      = conn_status + iterator;	
	if (strlen (*serverName) == 0)
		*serverName = 0;

	/* get next position */
	iterator           = next;
	next               = __get_next_field (conn_status, iterator);
	(*has_tls)         = atoi (conn_status + iterator);

	/* get next position: fix_server_name */
	iterator           = next;
	next               = __get_next_field (conn_status, iterator);
	(*fix_server_name) = atoi (conn_status + iterator);

	/* get next position: remote_host */
	iterator           = next;
	next               = __get_next_field (conn_status, iterator);
	(*remote_host)     = conn_status + iterator;
	if (strlen (*remote_host) == 0)
		*remote_host = 0;

	/* get next position: remote_port */
	iterator           = next;
	next               = __get_next_field (conn_status, iterator);
	(*remote_port)     = conn_status + iterator;
	if (strlen (*remote_port) == 0)
		*remote_port = 0;

	/* get next position: remote_port */
	iterator           = next;
	next               = __get_next_field (conn_status, iterator);
	(*remote_host_ip)  = conn_status + iterator;
	if (strlen (*remote_host_ip) == 0)
		*remote_host_ip = 0;
 
	return;
}

MyQttConn * __myqttd_process_handle_connection_received (MyQttdCtx      * ctx, 
							 MyQttdChild    * child,
							 MYQTT_SOCKET     _socket, 
							 char           * conn_status)
{
	axl_bool           handle_reply       = axl_false;
	const char       * serverName         = NULL;
	MyQttConn        * conn               = NULL;
	int                has_tls            = 0;
	MyQttMsg         * msg                = NULL;
	int                fix_server_name    = 0;
	const char       * remote_host        = NULL;
	const char       * remote_port        = NULL;
	const char       * remote_host_ip     = NULL;

	/* check connection status after continue */
	if (conn_status == NULL || strlen (conn_status) == 0) {
		error ("CHILD: internal server error, received conn_status string NULL or empty, socket=%d, unable to initialize connection on child",
		       _socket);
		myqtt_close_socket (_socket);
		return NULL;
	} /* end if */

	/* call to recover data from string */
	msg ("CHILD: processing conn_status received: [%s]", conn_status);
	myqttd_process_connection_recover_status (conn_status, 
						  &handle_reply,
						  &serverName,
						  &has_tls,
						  &remote_host,
						  &remote_port,
						  &remote_host_ip,
						  &fix_server_name);

	msg ("CHILD: Received conn_status: handle_reply=%d, serverName=%s, has_tls=%d, fix_server_name=%d, remote_host=%s, remote_port=%s, remote_host_ip=%s",
	     handle_reply,
	     serverName ? serverName : "",
	     has_tls, fix_server_name,
	     remote_host ? remote_host : "", 
	     remote_port ? remote_port : "",
	     remote_host_ip ? remote_host_ip : "");

	/* create a connection and register it on local myqtt
	   reader */
	conn = myqtt_conn_new_empty (MYQTTD_MYQTT_CTX (ctx), _socket, MyQttRoleListener);
	if (conn == NULL) {
		error ("CHILD: internal server error, unable to create connection object at child (socket=%d, role=listener)", _socket);
		return NULL;
	}

	/* setup host and port manually */
	if (remote_host && remote_port) {
		/* call to setup host and port */
		myqtt_conn_set_host_and_port (conn, remote_host, remote_port, remote_host_ip);
	}

	/* notify about the connection received and setup serverName
	 * if required by the parent server */
	msg ("CHILD: New connection id=%d (%s:%s) accepted on child pid=%d", 
	     myqtt_conn_get_id (conn), myqtt_conn_get_host (conn), myqtt_conn_get_port (conn), getpid ());
	if (fix_server_name && serverName) {
		msg ("CHILD: setting connection-id=%d serverName=%s as indicated by parent process", 
		     myqtt_conn_get_id (conn), serverName);
		/* setup server name */
		myqtt_conn_set_server_name (conn, serverName);
	} /* end if */

	/* set TLS status */
	if (has_tls > 0) {
		conn->tls_on = axl_true;
		msg ("CHILD: flagging the connection to have tls enabled (for profile path activation, fake TLS socket), conn-id=%d (%d)",
		     myqtt_conn_get_id (conn), conn->tls_on);
	} 

	if (handle_reply) {
		/* handle reply here */

	}

	/* call to register */
	if (! __myqttd_process_common_new_connection (ctx, conn, handle_reply, serverName, msg)) {
		/* nullify conn on error */
		conn = NULL;
	}
	    
	/* unref message */
	myqtt_msg_unref (msg);
	return conn;
}

/** 
 * @internal Function called each time a notification from the parent
 * is received on the child.
 */
axl_bool myqttd_process_parent_notify (MyQttdLoop * loop, 
					   MyQttdCtx  * ctx,
					   int              descriptor, 
					   axlPointer       ptr, 
					   axlPointer       ptr2)
{
	int                _socket         = -1;
	MyQttdChild  * child          = (MyQttdChild *) ptr;
	char             * ancillary_data = NULL;
	const char       * label          = ctx->child ? "CHILD" : "PARENT";
	
	msg ("%s: notification on control connection, read content", label);

	/* receive socket */
	if (! myqttd_process_receive_socket (&_socket, child, &ancillary_data, NULL)) {
		error ("%s: Failed to received socket..", label);
		return axl_false; /* close parent notification socket */
	}

	/* check content received */
	if (ancillary_data == NULL || strlen (ancillary_data) == 0 || _socket <= 0) {
		error ("%s: Ancillary data is null (%p) or empty or socket returned is not valid (%d)",
		       label, ancillary_data, _socket);
		goto release_content;
	}

	/* process commands received from the parent */
	if (ancillary_data[0] == 's') {
		/* close the connection received. This command signals
		   that the socket notified is already owned by the
		   current process (due to fork call) but the parent
		   still send us this socket to avoid having a file
		   descriptor used bucket. */
		wrn ("%s: closing socket=%d because it is already owned by the child process (due to fork call)", label, _socket);
	} else if (ancillary_data[0] == 'n') {
		/* received notification if a new, unknown connection,
		   register it */
		msg ("%s: Received socket %d, and ancillary_data[0]='%c' (processing)", 
		     label, _socket, ancillary_data[0]);
		__myqttd_process_handle_connection_received (ctx, child, _socket, ancillary_data + 1);
		_socket = -1; /* avoid socket be closed */
	} else {
		msg ("%s: Unknown command, socket received (%d), ancillary data: %s", 
		     label, _socket, ancillary_data);
	}

	/* release data received */
 release_content:
	axl_free (ancillary_data);
	myqtt_close_socket (_socket);

	return axl_true; /* don't close descriptor */
}

void __myqttd_process_release_parent_connections_foreach  (MyQttConn * conn, axlPointer user_data, axlPointer user_data2, axlPointer user_data3) 
{
#if ! defined(SHOW_FORMAT_BUGS)
	MyQttdCtx          * ctx           = user_data;
#endif
	MyQttConn       * child_conn    = user_data2;
	int                      client_socket = myqtt_conn_get_socket (conn);

	/* don't close/send the socket that the child must handle */
	if (child_conn && myqtt_conn_get_id (child_conn) == 
	    myqtt_conn_get_id (conn)) {
		msg ("CHILD: NOT Sending connection id=%d (socket: %d) to parent (now handled by child) (pointer: %p, role: %d)", 
		     myqtt_conn_get_id (conn), client_socket,
		     conn, myqtt_conn_get_role (conn));
		return;
	} /* end if */

	/* setting close on exec flag for all sockets the child will not handle */
	msg ("CHILD: Setting close on exec on socket: %d (conn id: %d, role: %d)", client_socket, 
	     myqtt_conn_get_id (conn), myqtt_conn_get_role (conn));
	fcntl (client_socket, F_SETFD, fcntl(client_socket, F_GETFD) | FD_CLOEXEC);

	return;
}

/** 
 * @internal This function ensures that all connections that the
 * parent handles but the child must't are closed properly so the
 * child process only have access to connections associated to its
 * profile path.
 *
 * The idea is that the parent (main process) may have a number of
 * running connections serving certain profile paths...but when it is
 * required to create a child, a fork is done (unix) and all sockets
 * associated running at the parent, are available and the child so it
 * is required to release all this stuff.
 *
 * The function has two steps: first check all connections in the
 * connection manager hash closing all of them but skipping the
 * connection that must handle the child (that is, the connection that
 * triggered the fork) and the second step to initialize the
 * connection manager hash. Later the connection that must handle the
 * child is added to the newly initialized connection manager hash by
 * issuing a myqttd_conn_mgr_register.
 */
int __myqttd_process_release_parent_connections (MyQttdCtx    * ctx, 
						     MyQttConn * child_conn, 
						     MyQttdChild  * child)
{
	/* set close on exec flag for all sockets but the socket the
	 * child will handle */
	myqtt_reader_foreach_offline (ctx->myqtt_ctx, __myqttd_process_release_parent_connections_foreach, 
				       ctx, child_conn, NULL);
	return 0;
}

void __myqttd_process_prepare_logging (MyQttdCtx * ctx, axl_bool is_parent, int * general_log, int * error_log, int * access_log, int * myqtt_log)
{
	/* check if log is enabled or not */
	if (! myqttd_log_is_enabled (ctx))
		return;

	if (is_parent) {

		/* support for general-log */
		myqttd_log_manager_register (ctx, LOG_REPORT_GENERAL, general_log[0]); /* register read end */
		myqtt_close_socket (general_log[1]);                                      /* close write end */
		
		/* support for error-log */
		myqttd_log_manager_register (ctx, LOG_REPORT_ERROR, error_log[0]); /* register read end */
		myqtt_close_socket (error_log[1]);                                      /* close write end */
			
		/* support for access-log */
		myqttd_log_manager_register (ctx, LOG_REPORT_ACCESS, access_log[0]); /* register read end */
		myqtt_close_socket (access_log[1]);                                      /* close write end */
		
		/* support for myqtt-log */
		myqttd_log_manager_register (ctx, LOG_REPORT_MYQTT, myqtt_log[0]); /* register read end */
		myqtt_close_socket (myqtt_log[1]);                                      /* close write end */

		return;
	} /* end if */

	return;
}

/** 
 * @internal Allows to check child limit (global and profile path
 * limits).
 *
 * @param ctx The context where the limit is being checked.
 *
 * @param conn The connection that triggered the child creation
 * process.
 *
 * @param def The profile path def selected for this connection.
 *
 * @return axl_true in the case the limit was reched and the
 * connection was closed, otherwise axl_false is returned.
 */
axl_bool myqttd_process_check_child_limit (MyQttdCtx      * ctx,
					   MyQttConn      * conn)
{
	msg ("Checking global child limit: %d before creating process.", ctx->global_child_limit);

	if (axl_hash_items (ctx->child_process) >= ctx->global_child_limit) {
		error ("Child limit reached (%d), unable to accept connection on child process, closing conn-id=%d", 
		       ctx->global_child_limit, myqtt_conn_get_id (conn));

		myqtt_conn_shutdown (conn);
		return axl_true; /* limit reached */
	} /* end if */	

	/* now check limits */

	return axl_false; /* limit NOT reached */
}

/** 
 * @internal Function used to send child init string to the child
 * process.
 */
axl_bool __myqttd_process_send_child_init_string (MyQttdCtx       * ctx,
						  MyQttdChild     * child,
						  MyQttConn       * conn,
						  int               client_socket,
						  axl_bool          handle_reply,
						  char            * serverName,
						  MyQttMsg        * msg,
						  int             * general_log,
						  int             * error_log,
						  int             * access_log,
						  int             * myqtt_log)
{
	char          * conn_status;
	char          * child_init_string;
	int             length;
	int             written;
	char          * aux;
	int             tries = 0;

#if defined(__MYQTTD_ENABLE_DEBUG_CODE__)
	if (serverName && axl_cmp (serverName, "kd7tkgnm3.fail.aspl.es")) {
		error ("PARENT: found serverName=%s that must trigger failure..", serverName);
		return axl_false;
	} /* end if */
#endif

	/* build connection status string */
	conn_status = myqttd_process_connection_status_string (handle_reply, 
							       serverName,
							       conn->tls_on,
							       /* notify if we have to fix the serverName */
							       axl_cmp (serverName, myqtt_conn_get_server_name (conn)),
							       /* provide host and port */
							       myqtt_conn_get_host (conn), myqtt_conn_get_port (conn),
							       myqtt_conn_get_host_ip (conn),
							       /* if proxied, skip recover on child */
							       myqttd_conn_mgr_proxy_on_parent (conn));
	if (conn_status == NULL) {
		error ("PARENT: failled to create child, unable to allocate conn status string");
		return axl_false;
	}
	msg ("PARENT: conn_status value: %s", conn_status);

	/* prepare child init string: 
	 *
	 * 0) conn socket : the connection socket that will handle the child 
	 * 1) general_log[0] : read end for general log 
	 * 2) general_log[1] : write end for general log 
	 * 3) error_log[0] : read end for error log 
	 * 4) error_log[1] : write end for error log 
	 * 5) access_log[0] : read end for access log 
	 * 6) access_log[1] : write end for access log 
	 * 7) myqtt_log[0] : read end for myqtt log 
	 * 8) myqtt_log[1] : write end for myqtt log 
	 * 9) child->socket_control_path : path to the socket_control_path
	 * 10) conn_status : connection status description to recover it at the child
	 * POSITION INDEX:                       0    1    2    3    4    5    6    7    8    9   10*/
 	child_init_string = axl_strdup_printf ("%d;_;%d;_;%d;_;%d;_;%d;_;%d;_;%d;_;%d;_;%d;_;%s;_;%d",
					       /* 0  */ client_socket,
					       /* 1  */ general_log[0],
					       /* 2  */ general_log[1],
					       /* 3  */ error_log[0],
					       /* 4  */ error_log[1],
					       /* 5  */ access_log[0],
					       /* 6  */ access_log[1],
					       /* 7  */ myqtt_log[0],
					       /* 8  */ myqtt_log[1],
					       /* 9  */ child->socket_control_path,
					       /* 10 */ conn_status);
	axl_free (conn_status);
	if (child_init_string == NULL) {
		error ("PARENT: failled to create child, unable to allocate memory for child init string");
		return axl_false;
	} /* end if */

	msg ("PARENT: created child init string: %s, handle_reply=%d", child_init_string, handle_reply);

	/* get child init string length */
	length = strlen (child_init_string);

	/* first send bytes to read */
	aux     = axl_strdup_printf ("%d\n", length);
	length  = strlen (aux);

	while (tries < 100) {
		written = write (child->child_connection, aux, length);
		if (written == length)
			break;
		tries++;
		if (errno == 107) {
			wrn ("PARENT: child still not ready, waiting 10ms (socket: %d), reconnecting..", child->child_connection);
			myqttd_sleep (ctx, 10000);
			myqtt_close_socket (child->child_connection);
			if (! __myqttd_process_create_child_connection (child)) {
				error ("PARENT: error after reconnecting to child process, errno: %d:%s", errno, myqtt_errno_get_last_error ());
				break;
			} /* end if */
		}

		/* if fails, check if the parent process is finishing to avoid waiting */
		if (ctx->is_exiting) {
			axl_free (aux);
			return axl_false;
		}
	} /* end while */
	axl_free (aux);

	/* check errors */
	if (written != length) {
		error ("PARENT: failed to send init child string, expected to write %d bytes but %d were written, errno: %d:%s",
		       length, written, errno, myqtt_errno_get_last_error ());
		return axl_false;
	}

	/* get child init string length (again) */
	length = strlen (child_init_string);
	/* msg ("PARENT: sending init child string length %d: %s", length, child_init_string); */

	/* send content */
	written = write (child->child_connection, child_init_string, length);
	axl_free (child_init_string);

	if (length != written) {
		error ("PARENT: failed to send init child string, expected to write %d bytes but %d were written",
		       length, written);
		return axl_false;
	}
	
	msg ("PARENT: child init string sent to child");
	return axl_true;
}

/** 
 * @internal Allows to create a child process running listener connection
 * provided.
 */
void myqttd_process_create_child (MyQttdCtx           * ctx, 
				  MyQttConn           * conn, 
				  axl_bool              handle_reply,
				  char                * serverName,
				  MyQttMsg            * msg)
{
	int                pid;
	MyQttdChild      * child = NULL;
	int                client_socket;
	
	/* pipes to communicate logs from child to parent */
	int                general_log[2] = {-1, -1};
	int                error_log[2]   = {-1, -1};
	int                access_log[2]  = {-1, -1};
	int                myqtt_log[2]  = {-1, -1};
	/* const char    * ppath_name;*/
	int                error_code;
	char            ** cmds;
	int                iterator = 0;
	axl_bool           skip_thread_pool_wait = axl_false;
	axl_bool           enable_debug          = axl_false;
	/* get current proxy on parent setting */
	axl_bool           proxy_on_parent = myqttd_conn_mgr_proxy_on_parent (conn);

	if (ctx->is_exiting) {
		error ("Unable to create child process, myqttd is finishing..");
		myqtt_conn_shutdown (conn);
		return;
	} /* end if */

	/* check if we are main process (only main process can create
	 * childs, at least for now) */
	if (ctx->child) {
		error ("Internal runtime error, child process %d is trying to create another subchild, closing conn-id=%d", 
		       ctx->pid, myqtt_conn_get_id (conn));
		myqtt_conn_shutdown (conn);
		return;
	} /* end if */

	/* get profile path name */
	/* ppath_name     = myqttd_ppath_get_name (def) ? myqttd_ppath_get_name (def) : "";
	   msg2 ("Calling to create child process to handle profile path: %s..", ppath_name); */

	MYQTTD_PROCESS_LOCK_CHILD ();

	/* recheck again if we are exiting */
	if (ctx->is_exiting) {
		/* unlock */
		MYQTTD_PROCESS_UNLOCK_CHILD ();

		error ("Unable to create child process, myqttd is finishing..");
		myqtt_conn_shutdown (conn);
		return;
	} /* end if */

	/* enable SIGCHLD handling */
	myqttd_signal_sigchld (ctx, axl_true);

	/* msg2 ("LOCK acquired: calling to create child process to handle profile path: %s..", ppath_name); */
	
	/* check if child associated to the given profile path is
	   defined and if reuse flag is enabled */
	/* child = myqttd_process_get_child_from_ppath (ctx, def, axl_false); */
	if (/* def->reuse && */ child) {
		msg ("Found child process reuse flag and child already created (%p), sending connection id=%d",
		     child, myqtt_conn_get_id (conn));

		if (proxy_on_parent) {
			/* setup the proxy on parent code creating a
			 * new client socket */
			client_socket = myqttd_conn_mgr_setup_proxy_on_parent (ctx, conn);

			/* send the connection to the child process */
			myqttd_process_send_proxy_connection_to_child (
			       ctx, child, conn, client_socket, handle_reply,
			       serverName, msg);
		} else {
		     
			/* reuse profile path */
			myqttd_process_send_connection_to_child (
			       ctx, child, conn, 
			       handle_reply, serverName, msg);
		} /* end if */
		MYQTTD_PROCESS_UNLOCK_CHILD ();
		return;
	}

	/* check limits here before continue */
	if (myqttd_process_check_child_limit (ctx, conn)) {
		/* unlock before shutting down connection */
		MYQTTD_PROCESS_UNLOCK_CHILD ();
		return;
	} /* end if */

	/* show some logs about what will happen */
	if (child == NULL) 
		msg ("PARENT: Creating a child process (first instance), proxy_on_parent=%d, conn-id=%d", 
		     proxy_on_parent, myqtt_conn_get_id (conn));
	else
 		msg ("PARENT: Child defined, but not reusing child processes (reuse=no flag), proxy_on_parent=%d, conn-id=%d", 
		     proxy_on_parent, myqtt_conn_get_id (conn));

	if (myqttd_log_is_enabled (ctx)) {
		if (pipe (general_log) != 0)
			error ("unable to create pipe to transport general log, this will cause these logs to be lost");
		if (pipe (error_log) != 0)
			error ("unable to create pipe to transport error log, this will cause these logs to be lost");
		if (pipe (access_log) != 0)
			error ("unable to create pipe to transport access log, this will cause these logs to be lost");
		if (pipe (myqtt_log) != 0)
			error ("unable to create pipe to transport myqtt log, this will cause these logs to be lost");
	} /* end if */

	/* create control socket path */
	child        = myqttd_child_new (ctx);
	if (child == NULL) {
		/* unlock child process mutex */
		MYQTTD_PROCESS_UNLOCK_CHILD ();

		/* close connection that would handle child process */
		myqtt_conn_shutdown (conn);
		return;
	} /* end if */

	/* call to fork */
	pid = fork ();
	if (pid != 0) {
		msg ("PARENT: child process created pid=%d", pid);

		if (proxy_on_parent) {
			/* setup the proxy on parent code creating a
			 * new client socket */
			client_socket = myqttd_conn_mgr_setup_proxy_on_parent (ctx, conn);
		} else {
			/* socket that is know handled by the child
			 * process */
			client_socket = myqtt_conn_get_socket (conn);

			/* unwatch the connection from the parent to
			   avoid receiving more content which now
			   handled by the child */
			myqtt_reader_unwatch_connection (CONN_CTX (conn), conn);
		} /* end if */

		/* update child pid and additional data */
		child->pid = pid;
		child->ctx = ctx;

		/* create child connection socket */
		if (! __myqttd_process_create_child_connection (child)) {
			error ("Unable to create child process connection to pass sockets for pid=%d", pid);
		
			MYQTTD_PROCESS_UNLOCK_CHILD ();

			myqtt_conn_shutdown (conn);
			myqttd_child_unref (child);
			return;
		}

		/* send child init string through the control socket */
		if (! __myqttd_process_send_child_init_string (ctx, child, conn, client_socket,
							       handle_reply, serverName, msg,
							       general_log, error_log, access_log, myqtt_log)) {
			MYQTTD_PROCESS_UNLOCK_CHILD ();

			myqtt_conn_shutdown (conn);
			myqttd_child_unref (child);
			return;
		} /* end if */

		/** 
		 * If we are proxying the connection send it to the
		 * child to handle it.
		 */
		if (proxy_on_parent && ! myqttd_process_send_proxy_connection_to_child (
			    ctx, child, conn, client_socket, handle_reply, serverName, msg)) {
			error ("PARENT: Unable to send socket associated to the connection that originated the child process (proxied)");
			MYQTTD_PROCESS_UNLOCK_CHILD ();

			myqtt_conn_shutdown (conn);
			myqttd_child_unref (child);
			return;
		} /* end if */

		/** 
		 * Send the socket descriptor to the child to avoid
		 * holding a bucket in the parent.
		 */
		if (! proxy_on_parent && ! myqttd_process_send_socket (client_socket, child, "s", 1)) {
			error ("PARENT: Unable to send socket associated to the connection that originated the child process");
			MYQTTD_PROCESS_UNLOCK_CHILD ();

			myqtt_conn_shutdown (conn);
			myqttd_child_unref (child);
			return;
		}

		/* terminate the connection */
		if (! proxy_on_parent) {
			myqtt_conn_set_close_socket (conn, axl_false);
			myqtt_conn_shutdown (conn);
		} /* end if */

		/* register pipes to receive child logs */
		__myqttd_process_prepare_logging (ctx, axl_true, general_log, error_log, access_log, myqtt_log);

		/* register the child process identifier */
		axl_hash_insert_full (ctx->child_process,
				      /* store child pid */
				      INT_TO_PTR (pid), NULL,
				      /* data and destroy func */
				      child, (axlDestroyFunc) myqttd_child_unref);

		/* update number of childs running this profile path */
		/* def->childs_running++; */

		MYQTTD_PROCESS_UNLOCK_CHILD ();

		/* record child */
		msg ("PARENT=%d: Created child process pid=%d (childs: %d)", getpid (), pid, myqttd_process_child_count (ctx));
		return;
	} /* end if */

	/**** CHILD CODE ****/

	/* reconfigure pids */
	ctx->pid = getpid ();

	/* release connections received from parent (including
	   sockets) */
	msg ("CHILD: calling to release all (parent) connections but conn-id=%d", 
	     myqtt_conn_get_id (conn));
	__myqttd_process_release_parent_connections (ctx, proxy_on_parent ? NULL : conn, child);   

	/* call to start myqttd process */
	msg ("CHILD: Starting child process with: %s %s --child %s --config %s", 
	     myqttd_child_cmd_prefix ? myqttd_child_cmd_prefix : "", 
	     myqttd_bin_path ? myqttd_bin_path : "", child->socket_control_path, ctx->config_path);

	/* get debug was requested */
	enable_debug = ! PTR_TO_INT (myqttd_ctx_get_data (ctx, "debug-was-not-requested"));

	/* prepare child cmd prefix if defined */
	if (myqttd_child_cmd_prefix) {
		msg ("CHILD: found child cmd prefix: '%s', processing..", myqttd_child_cmd_prefix);
		cmds = axl_split (myqttd_child_cmd_prefix, 1, " ");
		if (cmds == NULL) {
			error ("CHILD: failed to allocate memory for commands");
			exit (-1);
		} /* end if */

		/* count positions */
		while (cmds[iterator] != 0)
			iterator++;

		/* expand to include additional commands */
		cmds = axl_realloc (cmds, sizeof (char*) * (iterator + 14 + 1));

		cmds[iterator] = (char *) myqttd_bin_path;
		iterator++;
		cmds[iterator] = "--child";
		iterator++;
		cmds[iterator] = child->socket_control_path;
		iterator++;
		cmds[iterator] = "--config";
		iterator++;
		cmds[iterator] = ctx->config_path;
		iterator++;
		if (myqttd_log_enabled (ctx)) {
			cmds[iterator] = "--debug";
			iterator++;
		}
		if (myqttd_log2_enabled (ctx)) {
			cmds[iterator] = "--debug2";
			iterator++;
		} 
		if (myqttd_log3_enabled (ctx)) {
			cmds[iterator] = "--debug3";
			iterator++;
		}
		if (ctx->console_color_debug) {
			cmds[iterator] = "--color-debug";
			iterator++;
		}
		if (__myqttd_module_no_unmap) {
			cmds[iterator] = "--no-unmap-modules";
			iterator++;
		}
		/* get skip thread pool wait */
		myqtt_conf_get (MYQTTD_MYQTT_CTX(ctx), MYQTT_SKIP_THREAD_POOL_WAIT, &skip_thread_pool_wait);
		if (! skip_thread_pool_wait) {
			cmds[iterator] = "--wait-thread-pool";
			iterator++;
		}

		if (enable_debug && myqtt_log_is_enabled (ctx->myqtt_ctx)) {
			cmds[iterator] = "--myqtt-debug";
			iterator++;
		}
		if (enable_debug && myqtt_log2_is_enabled (ctx->myqtt_ctx)) {
			cmds[iterator] = "--myqtt-debug2";
			iterator++;
		}
		if (enable_debug && myqtt_color_log_is_enabled (ctx->myqtt_ctx)) {
			cmds[iterator] = "--myqtt-debug-color";
			iterator++;
		}

		cmds[iterator] = NULL;

		/* run command with prefix */
		error_code = execvp (cmds[0], cmds);
	} else {

		/* run command without prefixes */
		error_code = execlp (
			/* configure command to run myqttd */
			myqttd_bin_path, "myqttd", "--child", child->socket_control_path,
			/* pass configuration file used */
			"--config", ctx->config_path,
			/* pass debug options to child */
			myqttd_log_enabled (ctx) ? "--debug" : "", 
			myqttd_log2_enabled (ctx) ? "--debug2" : "", 
			myqttd_log3_enabled (ctx) ? "--debug3" : "", 
			ctx->console_color_debug ? "--color-debug" : "",
			/* pass myqtt debug options */
			(enable_debug && myqtt_log_is_enabled (ctx->myqtt_ctx)) ? "--myqtt-debug" : "",
			(enable_debug && myqtt_log2_is_enabled (ctx->myqtt_ctx)) ? "--myqtt-debug2" : "",
			(enable_debug && myqtt_color_log_is_enabled (ctx->myqtt_ctx)) ? "--myqtt-debug-color" : "",
			/* unmap modules support */
			__myqttd_module_no_unmap ? "--no-unmap-modules" : "",
			/* always last parameter */
			NULL);
	}
	
	error ("CHILD: unable to create child process, error found was: %d: errno: %s", error_code, myqtt_errno_get_last_error ());
	exit (-1);

	/**** CHILD PROCESS CREATION FINISHED ****/
	return;
}

#if defined(DEFINE_KILL_PROTO)
int kill (int pid, int signal);
#endif

axl_bool __terminate_child (axlPointer key, axlPointer data, axlPointer user_data)
{
#if ! defined(SHOW_FORMAT_BUGS)
	MyQttdCtx   * ctx   = user_data;
#endif
	MyQttdChild * child = data;

	/* send term signal */
	if (kill (child->pid, SIGTERM) != 0)
		error ("failed to kill child (%d) error was: %d:%s",
		       child->pid, errno, myqtt_errno_get_last_error ());
	return axl_false; /* keep on iterating */
}

/** 
 * @internal Function that allows to check and kill childs started by
 * myqttd acording to user configuration.
 *
 * @param ctx The context where the child stop operation will take
 * place.
 */ 
void myqttd_process_kill_childs  (MyQttdCtx * ctx)
{
	axlNode * node;
	int       pid;
	int       status;
	int       childs;

	/* get user doc */
	node = axl_doc_get (ctx->config, "/myqtt/global-settings/kill-childs-on-exit");
	if (node == NULL) {
		error ("Unable to find kill-childs-on-exit node, doing nothing..");
		return;
	}

	/* check if we have to kill childs */
	if (! HAS_ATTR_VALUE (node, "value", "yes")) {
		error ("leaving childs running (kill-childs-on-exit not enabled)..");
		return;
	} /* end if */

	/* disable signal handling because we are the parent and we
	   are killing childs (we know childs are stopping). The
	   following is to avoid races with
	   myqttd_signal_received for SIGCHLD */
	signal (SIGCHLD, NULL);

	/* send a kill operation to all childs */
	childs = axl_hash_items (ctx->child_process);
	if (childs > 0) {
		/* lock child to get first element and remove it */
		MYQTTD_PROCESS_LOCK_CHILD ();

		/* reacquire current childs */
		childs = axl_hash_items (ctx->child_process);

		/* notify all childs they will be closed */
		axl_hash_foreach (ctx->child_process, __terminate_child, ctx);


		/* unlock the list during the kill and wait */
		MYQTTD_PROCESS_UNLOCK_CHILD ();

		while (childs > 0) {
			/* wait childs to finish */
			msg ("waiting childs (%d) to finish...", childs);
			status = 0;
			pid = wait (&status);
			msg ("...child %d finish, exit status: %d", pid, status);
			
			childs--;
		} /* end while */
	} /* end while */


	return;
}

/** 
 * @brief Allows to return the number of child processes  created. 
 *
 * @param ctx The context where the operation will take place.
 *
 * @return Number of child process created or -1 it if fails.
 */
int      myqttd_process_child_count  (MyQttdCtx * ctx)
{
	int count;

	/* check context received */
	if (ctx == NULL)
		return -1;

	MYQTTD_PROCESS_LOCK_CHILD ();
	count = axl_hash_items (ctx->child_process);
	MYQTTD_PROCESS_UNLOCK_CHILD ();

	/* msg ("child process count: %d..", count); */
	return count;
}

axl_bool myqttd_process_child_list_build (axlPointer _pid, axlPointer _child, axlPointer _result)
{
	MyQttdChild * child = _child;

	/* acquire a reference */
	if (myqttd_child_ref (child)) {
		/* add the child */
		axl_list_append (_result, child);
	} /* end if */
	
	return axl_false; /* do not stop foreach process */
}

/** 
 * @brief Allows to get the list of childs created at the time the
 * call happend.
 *
 * @param ctx The context where the childs were created. The context
 * must represents a master process (myqttd_ctx_is_child (ctx) ==
 * axl_false).
 *
 * @return A list (axlList) where each position contains a \ref
 * MyQttdChild reference. Use axl_list_free to finish the list
 * returned. The function returns NULL if wrong reference received (it
 * is null or it does not represent a master process).
 */
axlList         * myqttd_process_child_list (MyQttdCtx * ctx)
{
	axlList * result;

	if (ctx == NULL)
		return NULL;
	if (myqttd_ctx_is_child (ctx))
		return NULL;

	/* build the list */
	result = axl_list_new (axl_list_always_return_1, (axlDestroyFunc) myqttd_child_unref);
	if (result == NULL)
		return NULL;

	/* now add childs */
	MYQTTD_PROCESS_LOCK_CHILD ();
	axl_hash_foreach (ctx->child_process, myqttd_process_child_list_build, result);
	MYQTTD_PROCESS_UNLOCK_CHILD ();

	return result;
}

/** 
 * @brief Allows to check if the provided process Id belongs to a
 * child process currently running.
 *
 * @param ctx The context where the operation will take place.
 *
 * @param pid The process identifier.
 *
 * @return axl_true if the process exists, otherwise axl_false is
 * returned.
 */
axl_bool myqttd_process_child_exists  (MyQttdCtx * ctx, int pid)
{
	/* check context received */
	if (ctx == NULL || pid < 0)
		return axl_false;

	return axl_false; /* (myqttd_process_find_pid_from_ppath_id (ctx, pid) != -1); */
}

/** 
 * @brief Allows to get a reference to the child object identified
 * with the provided pid.
 *
 * @param ctx The context where the child process will be searched.
 *
 * @param pid The child pid process searched.
 *
 * @return A reference to the myqttd child process or NULL if it
 * fails. The caller must release the reference when no longer
 * required calling to \ref myqttd_child_unref.
 */
MyQttdChild * myqttd_process_child_by_id (MyQttdCtx * ctx, int pid)
{
	MyQttdChild * child;

	/* check ctx reference */
	if (ctx == NULL)
		return NULL;

	/* lock */
	MYQTTD_PROCESS_LOCK_CHILD ();

	/* get child */
	child = axl_hash_get (ctx->child_process, INT_TO_PTR (pid));

	/* unlock */
	MYQTTD_PROCESS_UNLOCK_CHILD ();

	return child; /* no child found */
}

/** 
 * @internal Function used to cleanup the process module.
 */
void myqttd_process_cleanup      (MyQttdCtx * ctx)
{
	myqtt_mutex_destroy (&ctx->child_process_mutex);
	axl_hash_free (ctx->child_process);
	return;
			      
}

/** 
 * @internal Set current myqttd binary path that was used to load
 * current myqttd process. You must pass an static value.
 */
void              myqttd_process_set_file_path (const char * path)
{
	/* set binary path */
	myqttd_bin_path = path;
	return;
}

/** 
 * @internal Set current myqttd child cmd prefix to be added to
 * child startup command. This may be used to add debugger
 * commands. You must pass an static value.
 */
void              myqttd_process_set_child_cmd_prefix (const char * cmd_prefix)
{
	/* set child cmd prefix */
	myqttd_child_cmd_prefix = cmd_prefix;
	return;
}
